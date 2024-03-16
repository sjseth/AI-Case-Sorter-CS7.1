# Software controlled Camera LEDs

This change removes the external dimmer from the electronics and replaces it with software control via the Arduino and ultimately from the sorter software (when/if Seth adds this change to a release).

## Install Process

This change uses the hardware PWM control from the Arduino UNO.  There are only a few PINs that support PWM and we need to move the Feed Sensor (brass proximity sensor) from PIN 9 (x+) to PIN 13 (Spin DIR) on the CNC Shield.

DO NOT upload the firmware until you have made the hardware change!

### Parts Required

1.  DC 5V-36V 15A(Max 30A) 400W Dual High-Power MOSFET Trigger Switch Drive Module 0-20KHz.  This is the recommended way to drive the air solenoid for the AirDrop mod.  Refer to the parts list for the AirDrop for a link.

Note: If you built the AirDrop mod, you would have purchases a 5 pack of these MOSFET switches so you likely don't need any new hardware.

### Wiring Changes

1.  Either add a 4 pin header to the MOSFET switch, or solder a pair of wires with Molex connectors to the Trigger and Gnd pins on the MOSFET switch. 

2.  Move the black wire from the Proximity Sensor from the X+ pin to Spin DIR on the CNC Shield.

3.  Connect the Trigger and Gnd wires from the Camera LED MOSFET switch to the X+ and GND pins in the shield (or wire the Grd to your ground bus)

4.  Remove the +5V and Gnd from the Dimmer and connect them to the VIN+ and VIN- of the MOSFET switch.

5.  Remove the two output wires from the Dimmer module to the camera LEDs. 
	One of these should have a resistor - KEEP the resistor.  If you followed the original instructions, the resistor is in series with the Gnd (-ve) connection to the LEDs.  Connect this wire to the Vout- on the MOSFET switch.  Connect the +5V wire to the Vout+ on the MOSFET switch.

3.  Remove the dimmer (no longer required).

### Arduino Code

1.  Compile/Verify the Arduino file from this folder.

2.  Edit any of the config changes you had on your previous code.  

3.  There is a new config option CAMERA_LED_LEVEL.  This is the default brighthness of camera LEDs.  0 turns the light off.  255 gives 100% brightness.  With a 1k resistor, a value of 78 (about 30%) is a good place to start.  If you have a larger (1.2k or 1.5k) resistor you may need a higher value.

4.  Upload the Sketch to the UNO and do the normal verifications via the serial console.

### New Serial Commands

1.  getconfig 
    Will return all the configuration parameters, including the new cameraLEDLevel.

2.  cameraledlevel:x
	Sets the camera brightness to x, where x is 0-255.
	The console will reply with the percentage level set.
	
## Debugging

1.  No LEDs.  Check the polarity of the LED connection to the MOSFET switch and the +5v and Gnd connections to the MOSFET switch.  The LEDs polarity is likely reversed.

2.  The MOSFET switch has a red LED that lights up with the camera on (and appears dimmer/brighter as you adjust the settings).  If the LED isn't on, check you have the Trigger to the X+ PIN and the Ground to ground.

3.  Too dim/bright.  Open the Serial Monitor in the Arduino IDE and wait for the "Ready" reply.  Use cameraledlevel:255.  This should turn the LEDs to full on.  Open the Windows Camera App and check the image as you tune.  In my experience the AI Sorter Software will appear a little dimmer than the Camera app, so you might want to get 5-8 higher.

4.  Lost settings.  On poweroff/reboot the Arduino will go back to the #define CAMERA_LED_LEVEL value.  Update this value to your preferred setting and compile/upload the changes to keep them. 