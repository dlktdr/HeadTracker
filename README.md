## RC HeadTracker
If your an RC enthusiast like myself with a FPV headset at some point you thought wouldn't it be cool if I could turn my head and the camera would follow. That is the intention of this project. It is a continuation of the work started by Dennis Frie, Mark Mansur and others.

You can sense the orientation of your FPV headset by using an IMU (Inertial Measurement Unit) which is a chip that can sense which way it's pointing relative to the earth. There are many such IC's available on the market today. This software only supports two at the moment. For new users I would recommend purchasing a Arduino Nano 33 BLE. The other one supported is the Arduino Nano with a BNO055 sensor.

The Arduino Nano 33 BLE is an all in one solution, it has a processor and the sensor already to go.

The Arduino Nano and BNO055 requires soldering the sensor to an arduino nano. This solution is less expensive and works equally well. I'm not recommending this at this time as the extra work to source and assemble extra components doesn't justify the minor extra cost of the BLE33.

Once you have your board mounted to your headset you have to zero it out so your camera and the way your facing line up. There are two ways to do this. 
1) With a switch 
 A switch requires you to solder a small push button to the board.
2) Gesutre sensor
 The gesture sensor requires no soldering and you can just wave over the board to reset. ONLY the **Nano33BLE Sense** board has this sensor on board. I feel for the extra few dollars again it's worth it for simplicity.

Once you have a sensor mounted to your headset you have to get that signal up to your RC craft. (Plane, Drone, Etc.)
 If you own a FRSky Remote with the PARA trainer functionality this is now extreamly easy as this NanoBLE33 code can directly communicate with the transmitter.
 If you have any other transmitter they almost all support trainer input with a cable. The cable contains a PPM signal which most remotes can receive. You will need a MONO 3.5mm cable. Two wires will need to be soldered to the Nano33BLE and plugged into your transmitter.

!!IF YOUR READING THIS IT'S LATE AND I"LL FINISH LATER.!!

OLD DOCS BELOW

## Whats needed to Build tracker with a Arduino Nano with a BNO055 sensor

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
5) A window will open showing the status of the programming. It should show "avrdude.exe: 15662 bytes of flash verified" near the bottom if successful.
6) Close the firmware and log windows

## Usage
* Choose the correct Com Port from the list and click connect. After a second you should see **Settings Retrieved!** in the log.
* If you only see "TX: $GSET" and nothing else hit the reset button on the Nano to see if there was an Error on Bootup.
* Once Connected Click **Start** to show the realtime status.
* Right click on the range settings and choose default values for all three tilt, pan, roll if needed
* Adjust these sliders so when you pan your head the servo's move where you want them to.
* Adjust the gain settings to make the outputs more or less sensitive.
* The yellow triangle on the range sliders show the actual servo output
* The channel options allow you to choose what PPM output each is going to on your radio.

## Errors
If you get,
"Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!" - Check you have the proper firmware loaded with the correct BNO055 Address, if you use the board listed here you want the 0x29 address. If this is correct check your SCL and SDA wires are correct. Also check that both PS0 and PS1 are tied to ground.
If still nothing make sure you have the proper voltage going to the board.

## Calibration
Once your connected follow click Calibrate. This will guide you through a calibration procedure. Although this procedure isn't really required it does reduce the time at startup for the sensor to find magnetic north.

## Alternate orientations
You can mount the board in many orientations 48 to be exact. I don't have an easy way to find the correct settings at the moment. You will have to cycle through the Axis Sign and Remapping choices. Be sure to have "Show Raw Data" on when trying to find the correct one. This will remove any offsets from pressing the zero button. Hold your headset in a level orientation. Cycle through the choices until the tilt and roll on the graph are near zero. Ignore the pan until you have both tilt and roll at or near zero. Then check pan, remeber you will have to always manually zero this one.

## Note
The GUI is not compatible with non-BNO055 firmware's i've made quite a few changes and it currently won't work properly. I just recently made it so you can easily pick a firmware now so the next step is making this work with various boards and IC's.

![alt text](https://github.com/dlktdr/HeadTracker/raw/master/gui/src/ScreenShot.png)

## To Do

I've added board orientation choices. Needs work tho, haven't added too many choices. I also have to figure out a better way to do this tho. Everyone isn't going to buy the same board or solder it on in the same orientation.

# HeadTracker
New Release of the Headtracker Gui and 



## Whats needed
1) Arduino Nano 33 BLE (or Sense) - Digikey/Newark many others..
3) Soldering Iron
5) 3.5mm mono audio cable. I got a right angle one
6) Small Push Button Switch, for resetting zero. Note: If you buy BLE 33 Sense you can now wave over the gesture sensor to reset. No button required.

## To Do
- [x] Choose a fusion algorithm, went with Madgwick
- [x] Add calibration to the GUI
- [X] Save config to flash
- [ ] HM10+Nano Wireless remote PPM Board?
- [ ] FRsky trainer connection
- [X] Find solution to PPM output gitter from interrupt timing issues
- [ ] PPM Input Functionality and Testing
- [X] Add ability to program from the firmware window, sample firmware in master
- [ ] Create binaries for gui
- [ ] Combine with master

## Screenshot
![alt text](https://github.com/dlktdr/HeadTracker/raw/Nano-33-BLE/gui/src/ScreenCapture1007.png)
