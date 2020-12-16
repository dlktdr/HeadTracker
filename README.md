# HeadTracker
Re-work of Dennis Frie & Mark Mansur headtracker code for use with a BNO055 Sensor

I've modified the code for the head tracker and wrote a new GUI for it. Based on the BNO055 sensor. This sensor self calibrates once powered up. No need to calibrate connected to the PC.

## Whats needed

1) Arduino Nano - Amazon/Ebay/Many Places.
2) BNO055 Sensor Board - Ebay/Adafruit, https://www.adafruit.com/product/4646, I got mine on ebay, photo below search for BNO055
3) Soldering Iron
4) Short pieces of wire
5) 3.5mm mono stereo cable
6) Small Push Button Switch, for resetting zero

![alt text](https://github.com/dlktdr/HeadTracker/blob/master/Doc/BNO055.jpg?raw=true)

## Assembly
1) With the nano you will get some pin headers. Cut one of these so you have two pins. Solder it into A4(SDA)+A5(SCL), this is the I2C communication pins.
2) Solder two wires one into 3v3 pin and the other in a gnd pin. Leave the other end loose, it's difficult to get at once the board is on.
*** This is a 3.3v IC. Some boards have a voltage regulator on board so you can power them from 5V if desired. Hook up the nano to 5v in this case.
3) The BNO055 has two pins PS0 and PS1. These pins need to be connected to ground to place the chip in I2C mode so the arduino will see it. On my ebay board you make a solder bridge from the S0 to - and from S1 to -. On other brands of boards you may need to connect wires from PS0+PS1 to a ground pin.
4) Solder on the crystal to the back of the board and trim the wires excess wire, I mounted mine flat on the board and used some super glue to hold in place.
5) Solder the board onto the two headers sticking up from the nano. Making sure SDA connects to A4 and SCL connects to A5
6) Cut the two wires sticking up to the proper length and attach them to the 3.3 and gnd pins on the BNO055 board.
7) Cut off one end of your mono 3.5mm cable and strip off the insulation at the end. The outer copper braid connects to a gnd pin. The Center wire connects to D9. This is the cable you will plug into your transmitter to transmit your head orientation
8) Connect the push switch from D11 to GND. This will be used to reset zero when your head is level. I bent one of the pins on the switch and put it into GND, then used a wire to D11 on the other side. 
![alt text](https://github.com/dlktdr/HeadTracker/blob/master/Doc/Hookup.png?raw=true)

## Programming
1) Download the current release from https://github.com/dlktdr/HeadTracker and extract the zip file to a folder on your computer.
2) Choose the correct Com Port from the list. Hit refresh icon to scan again if neeeded.
3) Choose **Firmware->Upload Firmware** from the menu.
4) Click to **Fetch Firmwares Online**, a list of current available firmwares is presented. Highlight the one you want and click **Upload Selected**.
5) A window will open showing the status of the programming. You should get a programming success. If not check your com port and cables
6) Close the firmware window

## Usage
* Choose the correct Com Port from the list and click connect. After a second you should see **Settings Retrieved!** in the log.
* If you only see "TX: $GSET" and nothing else hit the reset button on the Nano to see if there was an Error on Bootup.
* Once Connected Click **Start** to show the realtime status.
* Right click on the range settings and choose default values for all three tilt, pan, roll if needed
* Adjust the range sliders so when you pan your head the servo's move where you want them to.
  * Setting The blue bars are the min/max ranges. 
  * Red is where you want center output. (Right click and choose re-center if you want it perfectly the middle of the min/max)
* Adjust the gain settings to make the outputs more or less sensitive.
* The yellow triangle on the range sliders show the actual servo output
* The channel options allow you to choose what PPM output each is going to on your radio.
* The low pass settings allow filtering out rapid motion. Set to 1% for maximum filtering. Set to 100% for very little.
* **Reset Center** does the same thing as pressing the physical button

## Errors
If you get,
"Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!" - Check you have the proper firmware loaded with the correct BNO055 Address, if you use the board listed here you want the 0x29 address. If this is correct check your SCL and SDA wires are correct. Also check that both PS0 and PS1 are tied to ground.
If still nothing make sure you have the proper voltage going to the board.

## Calibration
**Version 0.3** - To calibrate connect to the device and click the calibrate button. This will start the calibration. The goal is to have all bar graphs at a maximum at which time it will save the data. It will guide you through each step as you do them. Once all bars hit go max the data is saved and you're done.

**Versions < 0.3** - The BNO055 calibration data is lost on every reboot. All I've found you need to do is slowly pan your headseat 180. Set it down and not moving for 5seconds. 
After you put your headset on hold your head level and straight and hit the reset button. Thats it.
Don't worry about trying to get all the Gyro,Accel,Mag,System Bar graphs to their max levels. We don't need that accuracy for what this is.

## Alternate orientations
You can mount the board in many orientations 48 to be exact. I don't have an easy way to find the correct settings at the moment. You will have to cycle through the Axis Sign and Remapping choices. Be sure to have "Show Raw Data" on when trying to find the correct one. This will remove any offsets from pressing the zero button. Hold your headset in a level orientation. Cycle through the choices until the tilt and roll on the graph are near zero. Ignore the pan until you have both tilt and roll at or near zero. Then check pan, remeber you will have to always manually zero this one.

## Note
The GUI is not compatible with non-BNO055 firmware's i've made quite a few changes and it currently won't work properly. I just recently made it so you can easily pick a firmware now so the next step is making this work with various boards and IC's.

![alt text](https://github.com/dlktdr/HeadTracker/raw/master/gui/src/ScreenShot.png)

## To Do

Make it so you can mount in any position!
