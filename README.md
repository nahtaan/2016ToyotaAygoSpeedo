# 2016 Toyota Aygo Speedo
An Arduino-based digital dashboard that reads vehicle data directly from the car's CAN bus network and displays it on a 4-digit 7 segment display.

Currently read data:
- Vehicle Speed
- Engine RPM
- Coolant Temperature

Additional Features:
- 0-62 mph timer
- Shift light (Whole display flashes when above 5500 rpm)

There is also a file under CanInspecting which contains the code used for reverse engineering the different canbus codes.
