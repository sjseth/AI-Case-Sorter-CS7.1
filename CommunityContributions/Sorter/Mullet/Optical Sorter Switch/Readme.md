# Optical Sorter Switch Mod

![](./images/image.png)

### Requirements:
The Sorter optics homing sensor mod requires the following items for the mod to work. 

1. New sorter tube support with the homing point repositioned on the support to allow for new position of switch. I have designed this to have a slide on lock ring locking the sorter tube in position. 

2. The components of the optical switch, the housing, the lid and the plunger. Which will all be 3D printed with the STL file provided and attached.

3. You will also require an optical IR Sensor H206 Module type. M3 Screws and 2 X Compression Spring 0.3 X 4mm OD X15mm long.

<br/>



### Wiring Changes Required
Correct connection of power, ground and signal is required as per the optical IR sensor WYC­H206.

![alt text](./images/image-1.png)

### The Main component ready to assemble
![alt text](./images/image-2.png)

### Assembly of switch 
![](./images/image-3.png)

 

### Interrupter for IR 
The next Picture shows the interrupt for IR sensor. This can be fine tuned by simply sanding the length down to allow for correct homing location.

![alt text](./images/image-4.png)




### Fully assembled 

![alt text](./images/image-5.png)

### Arduino Code Changes may be required

Depending on the type of the optical switch, you may need to make a change to the arduino code. If the optical switch is a normally open (NO), then there are no changes required. 

However, if the optical switch is an normally closed switch (NC), we have to make small change in the arduino code. You may need to get the latest arduino code from the repo as this modification was added in version 7.1.20250109.1

![alt text](./images/image-6.png)

You need to change the **SORT_HOMING_SENSOR_TYPE** from **1** to **0** and then redeploy the code to your arduino. 