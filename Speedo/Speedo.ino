// imports
#include <mcp2515.h>
#include <SPI.h>
#include <SevSeg.h>

// define canMsg variable
struct can_frame canMsg;
// define send canMsg
struct can_frame sendMsg;
// define mcp2515 variable and set pin
MCP2515 mcp2515(10);
// define sevseg variable
SevSeg sevseg;
// startup animation tracking
bool startupFinished = false;
// speed variable
float speedMph = 0.0;
// rpm variable
float revs = 0.0;
// coolant temp variable
int temp = 0;
// speedLimiter variable
bool isSpeedLimited = false;
// variables for tracking 0 - 62 mph
signed long lastZeroTime = -1;
unsigned long timeTo62 = 0;
bool flashAccelTime = false;
// display state tracker
int displayState = 0;

void setup() {
  // init serial
  Serial.begin(115200);

  // init canbus communication
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  // log message
  Serial.println("Initialized mcp2515");

  // define config for SevSeg
  byte numDigits = 4;
  byte digitPins[] = {6, 7, 8, 9};
  byte segmentPins[] = {14, 3, 16, 17, 18, 19, 4, 15};
  bool resistorsOnSegments = true;
  byte hardwareConfig = COMMON_ANODE;

  // init sevseg
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments);
  sevseg.setBrightness(100);
  sevseg.refreshDisplay();

  // log message
  Serial.println("Initialized SevSeg");
}

void loop() {
  // must be ran every loop so that the sevseg display can be seen
  sevseg.refreshDisplay();

  // Show the startup animation
  if(!startupFinished) {
    // switch the state every 150 milliseconds
    static unsigned long lastTime = millis();
    static uint8_t index = 0;
    if(millis() - lastTime >= 150) {
      lastTime = millis();
      setState(index);
      index += 1;
    }
    if(index == 17) {
      startupFinished = true;
    }
    return;
  }

  /* Section for requesting data from canbus */
  static unsigned long lastrpm = millis();
  static unsigned long lastoil = millis();
  if(millis() - lastrpm >= 100) {
    lastrpm = millis();
    sendMsg.can_id = 0x7DF;
    sendMsg.can_dlc = 8;
    sendMsg.data[0] = 0x02;
    sendMsg.data[1] = 0x01;
    sendMsg.data[2] = 0x0C;
    sendMsg.data[3] = 0x0;
    sendMsg.data[4] = 0x0;
    sendMsg.data[5] = 0x0;
    sendMsg.data[6] = 0x0;
    sendMsg.data[7] = 0x0;
    mcp2515.sendMessage(&sendMsg);
  }
  if(millis() - lastoil >= 500) {
    lastoil = millis();
    sendMsg.can_id = 0x7DF;
    sendMsg.can_dlc = 8;
    sendMsg.data[0] = 0x02;
    sendMsg.data[1] = 0x01;
    sendMsg.data[2] = 0x05;
    sendMsg.data[3] = 0x0;
    sendMsg.data[4] = 0x0;
    sendMsg.data[5] = 0x0;
    sendMsg.data[6] = 0x0;
    sendMsg.data[7] = 0x0;
    mcp2515.sendMessage(&sendMsg);
  }

  /* Section for reading data from canbus */

  MCP2515::ERROR result = mcp2515.readMessage(&canMsg);
  if (result == MCP2515::ERROR_OK) {
    // handle speed packet
    if (canMsg.can_id == 0x0b4) {
      // extract speed values from the packet
      speedMph = ((float) ((canMsg.data[5] << 8) | canMsg.data[6]) / 100) * 0.621371;

    // handle diagnostic packets
    }else if (canMsg.can_id == 0x7E8) {
      if(canMsg.data[2] == 0x0C) {
        revs = ((canMsg.data[3] << 8) + canMsg.data[4]) / 4.0;
      }else if (canMsg.data[2] == 0x05) {
        temp = canMsg.data[3]-40;
      }
    }else if (canMsg.can_id == 0x399) {
      // extract speed limiter status from packet
      isSpeedLimited = (canMsg.data[6] & 0b1) == 0b1;
      // handle checking for if we should change mode
      static uint8_t oneCount = 0;
      if(isSpeedLimited) {
        oneCount += 1;
      }else if(!isSpeedLimited && oneCount == 1) {
        oneCount = 0;
        displayState += 1;
        static unsigned long start = millis();
        start = millis();
        while(millis() - start <= 250) {
          sevseg.refreshDisplay();
          sevseg.blank();
        }

      }else if(oneCount >= 2 && !isSpeedLimited) {
        oneCount = 0;
      }
    }
  }

  /* Section for handling other calculations */

  // handle recording 0-62
  if(speedMph <= 0) {
    lastZeroTime = millis();
  }else if(speedMph >= 62 && lastZeroTime > 0) {
    timeTo62 = millis() - lastZeroTime;
    lastZeroTime = -1;
    // do not show very long time values
    if(timeTo62 > 100000) {
      flashAccelTime = false;
    }
  }

  /* Section for updating the display */

  static unsigned long timer = millis();
  // display 0-62 time
  if(flashAccelTime) {
    // tracker for the start of the animation
    static unsigned long startAnim = millis();
    if(millis() - startAnim > 5000) {
      startAnim = millis();
    }
    // display order
    // blank - 200ms
    // time - 1500ms
    // blank - 200ms
    if (millis() - startAnim >= 1800) {
      flashAccelTime = false;
    }else if (millis() - startAnim >= 1700) {
      sevseg.blank();
    }else if(millis() - startAnim >= 200) {
      float time = timeTo62 / 1000.0;
      if(time < 10) {
        sevseg.setNumberF(time, 3);
      }else{
        sevseg.setNumberF(time, 2);
      }
    }else {
      sevseg.blank();
    }
    return;
  // flash the display if over 5500 rpm
  }else if (revs >= 5500) {
    static boolean isBlank = true;
    if(millis() < timer) {
      return;
    }
    timer += 75;
    if(isBlank) {
      uint8_t segs[4] = {0xFF, 0xFF, 0xFF, 0xFF};
      sevseg.setSegments(segs);
      isBlank = false;
    }else {
      sevseg.blank();
      isBlank = true;
    }
  }else {
    switch(displayState){
      // reset state
      case 3:
        displayState = 0;
        break;
      // display oil temp
      case 2:
        if(millis() >= timer) {
          timer += 100;
          sevseg.setNumber(temp, -1);
        }
        break;
      // display revs
      case 1:
        if(millis() >= timer) {
          timer += 100;
          sevseg.setNumberF(revs);
        }
        break;
      // display speed
      case 0:
        if (millis() >= timer) {
          timer += 100;
          sevseg.setNumberF(speedMph, -1);
        }
        break;
    }
  }
}

void setState(uint8_t index) {
    switch (index) {
        case 0:
        {
            sevseg.blank();
            break;
        }
        case 1:
        {
            uint8_t segs[4] = {0b00100001, 0b00000000, 0b00000000, 0b00000000};
            sevseg.setSegments(segs);
            break;
        }
        case 2:
        {
            uint8_t segs[4] = {0b01110011, 0b00000000, 0b00000000, 0b00000000};
            sevseg.setSegments(segs);
            break;
        }
        case 3:
        {
            uint8_t segs[4] = {0b01011110, 0b00100001, 0b00000000, 0b00000000};
            sevseg.setSegments(segs);
            break;
        }
        case 4:
        {
            uint8_t segs[4] = {0b10001100, 0b01110011, 0b00000000, 0b00000000};
            sevseg.setSegments(segs);
            break;
        }
        case 5:
        {
            uint8_t segs[4] = {0b10000000, 0b01011110, 0b00100001, 0b00000000};
            sevseg.setSegments(segs);
            break;
        }
        case 6:
        {
            uint8_t segs[4] = {0b00000000, 0b10001100, 0b01110011, 0b00000000};
            sevseg.setSegments(segs);
            break;
        }
        case 7:
        {
            uint8_t segs[4] = {0b00000000, 0b10000000, 0b01011110, 0b00100001};
            sevseg.setSegments(segs);
            break;
        }
        case 8:
        {
            uint8_t segs[4] = {0b00000000, 0b00000000, 0b10001100, 0b01110011};
            sevseg.setSegments(segs);
            break;
        }
        case 9:
        {
            uint8_t segs[4] = {0b00000000, 0b00000000, 0b10000000, 0b01011110};
            sevseg.setSegments(segs);
            break;
        }
        case 10:
        {
            uint8_t segs[4] = {0b00000000, 0b00000000, 0b00000000, 0b10001100};
            sevseg.setSegments(segs);
            break;
        }
        case 11:
        {
            uint8_t segs[4] = {0b00000000, 0b00000000, 0b00000000, 0b10000000};
            sevseg.setSegments(segs);
            break;
        }
        case 12:
        {
            sevseg.blank();
            break;
        }
        default:
            break;
    }
}
