#pragma once

#define JOYSTICK_BUTTON_HIGH (400)
#define JOYSTICK_BUTTON_LOW  (-JOYSTICK_BUTTON_HIGH)

static const uint8_t hid_gamepad_report_desc[] =
{
   0x05, 0x01,          // UsagePage(Generic Desktop[0x0001])
    0x09, 0x05,          // UsageId(Gamepad[0x0005])
    0xA1, 0x01,          // Collection(Application)
    0x85, 0x01,          //     ReportId(1)
    0x05, 0x09,          //     UsagePage(Button[0x0009])
    0x19, 0x01,          //     UsageIdMin(Button 1[0x0001])
    0x29, 0x10,          //     UsageIdMax(Button 16[0x0010])
    0x15, 0x00,          //     LogicalMinimum(0)
    0x25, 0x01,          //     LogicalMaximum(1)
    0x95, 0x10,          //     ReportCount(16)
    0x75, 0x01,          //     ReportSize(1)
    0x81, 0x02,          //     Input(Data, Variable, Absolute, NoWrap, Linear, PreferredState, NoNullPosition, BitField)
    0x05, 0x01,          //     UsagePage(Generic Desktop[0x0001])
    0x09, 0x30,          //     UsageId(X[0x0030])
    0x09, 0x31,          //     UsageId(Y[0x0031])
    0x09, 0x32,          //     UsageId(Z[0x0032])
    0x09, 0x33,          //     UsageId(Rx[0x0033])
    0x09, 0x34,          //     UsageId(Ry[0x0034])
    0x09, 0x35,          //     UsageId(Rz[0x0035])
    0x09, 0x36,          //     UsageId(Slider[0x0036])
    0x09, 0x37,          //     UsageId(Dial[0x0037])
    0x16, 0x00, 0xFE,    //     LogicalMinimum(-512)
    0x26, 0xFF, 0x01,    //     LogicalMaximum(511)
    0x95, 0x08,          //     ReportCount(8)
    0x75, 0x0A,          //     ReportSize(10)
    0x81, 0x02,          //     Input(Data, Variable, Absolute, NoWrap, Linear, PreferredState, NoNullPosition, BitField)
    0xC0,                // EndCollection()
};

#pragma pack(push,1)

#define HID_REPORT_INPUT1_ID (1)
struct HidReportInput1
{
    uint8_t ReportId = HID_REPORT_INPUT1_ID;
    uint8_t buttons[2];
    int16_t ch1:10;
    int16_t ch2:10;
    int16_t ch3:10;
    int16_t ch4:10;
    int16_t ch5:10;
    int16_t ch6:10;
    int16_t ch7:10;
    int16_t ch8:10;
} __packed;

#pragma pack(pop)

// Bytes 1 - ID
// Bytes 2 - Buttons 1-8
// Bytes 10 - Channels 1-8 (10 bits each)  = 80 bits = 10 bytes
BUILD_ASSERT(sizeof(HidReportInput1) == 13, "HID Report wrong size is not packed correctly");

void buildJoystickHIDReport(struct HidReportInput1 &report, uint16_t chans[16]);
void joystick_init(void);
void set_JoystickChannels(uint16_t chans[16]);