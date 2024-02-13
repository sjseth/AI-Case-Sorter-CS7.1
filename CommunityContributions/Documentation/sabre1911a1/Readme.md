BTT SKR PICO MOD
----------------

**I was elated to discover Seth's CS7.1 case sorter videos on YouTube. But, because I can never leave things well-enough alone, several thoughts entered my mind while watching the videos:**

1. The BigtreeTech SKR Pico, a controller board for the Voron 0 3D printer, would potentially make a perfect single-board alternative to the Arduino Uno used in this project, eliminating the need for: 
      - the motor shield
      - the 2209 drivers and heatsinks
      - the PWM dimmer for the LED
      - the voltage rails and associated wiring
      - the external relay controlling the AirDrop solenoid (not 100% sure of this one since I can't test it, but I THINK the heat bed and/or hotend outputs on the board should be able to run a 12v air solenoid - an experiment for another day)

2. The Raspberry Pi RP2040 microcontroller can be programmed in Arduino (support for it right in the Arduino IDE board manager!) The BTT SKR Pico's MCU is a Raspberry Pi RP2040, therefore it can be programmed with Arduino code, therefore Seth's existing code should work with it with minor modifications.

3. Running this project with an SKR Pico could simplify the wiring and the electronics package as a whole.

4. There may be some minor cost savings (Arduino + motor shield + two drivers + the other stuff is a little more expensive if you add it all up in your Amazon cart than the SKR Pico alone).

5. I would love to contribute to this project in some way, because this project is awesome!

**All that said there are currently a few potential downsides to this mod:**

1. You will need to be (or get) comfortable with crimping JST connectors, although you will only have to make 1 2-pin connector, 1 3-pin connector, and maybe 1 5-pin (with a single wire) if you need to break out the accessory signal.

2. Making modifications to the motor driver run current now has to be done in software via an Arduino library and UART instead of with physical potentiometers. I haven't got that working at the time of this writing, but everything has worked fine in testing, regardless.

3. This is obviously a work in progress. 
