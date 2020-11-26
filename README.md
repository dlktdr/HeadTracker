# HeadTracker
Re-work of Dennis Frie's  work for use with a BNO055 Sensor

I've modified the code for the head tracker and started working on a new GUi for it. Based on the BNO055 sensor. This sensor self calibrates once powered up. No need to calibrate connected to the PC.

## Whats needed

1) Arduino Nano - Amazon/Ebay/Many Places
2) BNO055 Sensor Board - Ebay/Adafruit, https://www.adafruit.com/product/4646, I got mine on ebay, photo below search for BNO055
3) Soldering Iron
4) Short pieces of wire
5) 2.5mm mono plug or cord

![alt text](https://github.com/dlktdr/HeadTracker/blob/master/Doc/BNO055.jpg?raw=true)

## Assembly

1) With the nano you will get some pin headers. Cut one of these so you have two pins. Solder it into A4(SDA)+A5(SCL), this is the I2C communication pins.
2) Solder two wires one into 3v3 pin and the other in a gnd pin. Leave the other end loose, it's difficult to get at once the board is on.
*** This is a 3.3v IC. Some boards have a voltage regulator on board so you can power them from 5V if desired
3) The BNO055 has two pins PS0 and PS1. These pins need to be connected to ground to place the chip in I2C mode so the arduino will see it. On my ebay board you make a solder bridge from the S0 to - and from S1 to -. On other brands of boards you may need to connect wires from PS0+PS1 to a ground pin.
4) Solder on the crystal to the back of the board and trim the wires excess wire, I mounted mine flat on the board and used some super glue to hold in place.
5) Solder the board onto the two headers sticking up from the nano. Making sure SDA connects to A4 and SCL connects to A5
6) Cut the two wires sticking up to the proper length and attach them to the 3.3 and gnd pins on the BNO055 board.
7) Program and Test.

I was going to make the GUI compatible with the old firmware too but It's currently broken for that. I will work on it later if there is any requests to do so.

![alt text](https://github.com/dlktdr/HeadTracker/blob/master/ScreenShot.png?raw=true)
