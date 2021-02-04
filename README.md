# RC HeadTracker
If your an RC enthusiast like myself with a FPV headset at some point you thought wouldn't it be cool if I could turn my head and the camera would follow. That is the intention of this project. It is a continuation of the work started by Dennis Frie, Mark Mansur and others in an effort to make it a more simple project.

I would like to throw out thanks to Yuri for the BLE Para work. https://github.com/ysoldak/HeadTracker

## [Wiki](https://github.com/dlktdr/HeadTracker/wiki)
* Please visit the  for the hardware required and how to build one.

## [Nano33BLE Build Link](https://github.com/dlktdr/HeadTracker/wiki/Nano33BLE-Build-Instructions)
* How to for the new BLE33 board

## [Download](https://github.com/dlktdr/HeadTracker/releases)
* Download the latest version from here if your running Windows 7,8,10. 
* The GUI should compile on Linux and MacOS but I have not tested it (*Firmware upload will not work).

### To Do
- [x] Choose a fusion algorithm, went with Madgwick
- [x] Add calibration to the GUI
- [X] Save config to flash
- [X] Wireless PPM using a second Nano33BLE
- [X] FRsky trainer connection
- [X] Find solution to PPM output jitter from interrupt timing issues
- [ ] PPM Input Functionality and testing on BLE33
- [X] Add ability to program from the firmware window, sample firmware in master
- [X] Create binaries for Gui
- [ ] Add rotation to mount board in other orientations BLE33

### Screenshot
![alt text](https://github.com/dlktdr/HeadTracker/raw/master/docs/ScreenCapture1006.png)
