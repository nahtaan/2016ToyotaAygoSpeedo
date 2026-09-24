#include <SPI.h>
#include <mcp2515.h>

struct can_frame canMsg;
struct can_frame sendMsg;
MCP2515 mcp2515(10);

uint16_t ids[66] = {0x1AA,
0x1C4,
0x1C6,
0x20,
0x220,
0x221,
0x226,
0x228,
0x24,
0x25,
0x260,
0x2C1,
0x320,
0x351,
0x3B7,
0x3D3,
0x413,
0x4C1,
0x4C6,
0xAA,
0xB4,
0xBA,
0x38A,
0x394,
0x399,
0x3B1,
0x3BB,
0x3C3,
0x3D0,
0x3F9,
0x423,
0x434,
0x436,
0x437,
0x442,
0x443,
0x4A6,
0x4C8,
0x4DD,
0x610,
0x611,
0x620,
0x622,
0x6E1,
0x384,
0x386,
0x387,
0x389,
0x38E,
0x38F,
0x3A5,
0x3BC,
0x3E6,
0x3E7,
0x3E8,
0x3E9,
0x420,
0x435,
0x4C3,
0x618,
0x640,
0x614,
0x615,
0x61A,
0x61c,
0x619};
uint8_t currentIndex = 0;
void setup() {
  sendMsg.can_id = 0x622;
  sendMsg.can_dlc = 8;
  sendMsg.data[0] = 0x12;
  sendMsg.data[1] = 0x80;
  sendMsg.data[2] = 0x0;
  sendMsg.data[3] = 0x40;
  sendMsg.data[4] = 0x0;
  sendMsg.data[5] = 0x0;
  sendMsg.data[6] = 0x0;
  sendMsg.data[7] = 0x0;

  Serial.begin(115200);
  
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();
}

void loop() {
  // static unsigned long time = millis();
  // if(millis() - time >= 25) {
    // mcp2515.sendMessage(&sendMsg);
    // time = millis();
  // }
  if(Serial.available() > 0) {
    Serial.read();
    mcp2515.sendMessage(&sendMsg);
    Serial.println("Sent message.");
  }

  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    // if(canMsg.can_id != ids[currentIndex]) {
    if(canMsg.can_id != 0x622) {
      return;
    }
    Serial.print(canMsg.can_id, HEX);
    Serial.print(" ");
    for(int i = 0; i<8; i++) {
      Serial.print(canMsg.data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
}