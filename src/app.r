#include "Dialogs.r"
#include "Menus.r"
#include "Processes.r"

resource 'DLOG' (128) {
    {40, 66, 364, 446}, noGrowDocProc, visible, goAway, 0, 128,
    "MacADTPro", centerMainScreen
};

resource 'DITL' (128) {
    {
        {0, 0, 0, 0}, StaticText {disabled, ""};
        {0, 0, 0, 0}, StaticText {disabled, ""};
        {0, 0, 0, 0}, StaticText {disabled, ""};
        {0, 0, 0, 0}, StaticText {disabled, ""};
        {0, 0, 0, 0}, StaticText {disabled, ""};
        {0, 0, 0, 0}, StaticText {disabled, ""};
        {0, 0, 0, 0}, StaticText {disabled, ""};
        {0, 0, 0, 0}, StaticText {disabled, ""};
        {0, 0, 0, 0}, StaticText {disabled, ""};
        {0, 0, 0, 0}, StaticText {disabled, ""};
        {0, 0, 0, 0}, StaticText {disabled, ""};
        {206, 20, 238, 254}, StaticText {disabled, "Ready."};
        {290, 270, 312, 364}, Button {enabled, "Connect"};
        {28, 20, 40, 360}, StaticText {disabled, "Locating image folder..."};
        {242, 20, 254, 254}, StaticText {disabled, "RX: 0 bytes   TX: 0 bytes"};
        /* LNew draws its 15-pixel vertical scroll bar to the right of rView. */
        {46, 20, 200, 345}, StaticText {disabled, ""};
        {8, 20, 26, 360}, StaticText {disabled, "Disk images in the selected folder:"};
        {0, 0, 0, 0}, StaticText {disabled, ""};
    }
};

resource 'MENU' (128) {
    128, textMenuProc;
    allEnabled, enabled;
    apple;
    {
        "About MacADT Pro...", noIcon, noKey, noMark, plain;
    }
};

resource 'MENU' (129) {
    129, textMenuProc;
    allEnabled, enabled;
    "File";
    {
        "Preferences...", noIcon, ".", noMark, plain;
        "-", noIcon, noKey, noMark, plain;
        "Quit", noIcon, "Q", noMark, plain;
    }
};

resource 'MBAR' (128) {
    { 128, 129 };
};

resource 'DLOG' (129) {
    {80, 86, 246, 406}, dBoxProc, visible, noGoAway, 0, 129,
    "About MacADT Pro", centerMainScreen
};

resource 'DITL' (129) {
    {
        {130, 224, 152, 296}, Button {enabled, "OK"};
        /* Replaced with mixed-font drawing code after GetNewDialog. */
        {18, 20, 118, 300}, StaticText {disabled, ""};
    }
};

resource 'DLOG' (130) {
    {62, 76, 306, 436}, noGrowDocProc, visible, goAway, 0, 130,
    "Preferences", centerMainScreen
};

resource 'DITL' (130) {
    {
        {204, 264, 226, 344}, Button {enabled, "OK"};
        {204, 168, 226, 248}, Button {enabled, "Cancel"};
        {12, 16, 30, 104}, StaticText {disabled, "Serial port"};
        {12, 120, 30, 224}, RadioButton {enabled, "Modem"};
        {12, 244, 30, 344}, RadioButton {enabled, "Printer"};
        {44, 16, 62, 344}, StaticText {disabled, "Transfer speed (baud)"};
        {68, 16, 86, 104}, RadioButton {enabled, "300"};
        {68, 120, 86, 208}, RadioButton {enabled, "1200"};
        {68, 224, 86, 312}, RadioButton {enabled, "2400"};
        {92, 16, 110, 104}, RadioButton {enabled, "9600"};
        {92, 120, 110, 224}, RadioButton {enabled, "19200"};
        {124, 16, 144, 344}, CheckBox {enabled, "ImageWriter II cable"};
        {154, 16, 194, 344}, StaticText {disabled,
            "Choose the port and speed used by the Apple II. The cable option enables hardware handshaking."};
        {0, 0, 0, 0}, StaticText {disabled, ""};
    }
};

resource 'SIZE' (-1) {
    reserved, acceptSuspendResumeEvents, reserved, canBackground,
    doesActivateOnFGSwitch, backgroundAndForeground, dontGetFrontClicks,
    ignoreChildDiedEvents, is32BitCompatible, notHighLevelEventAware,
    onlyLocalHLEvents, notStationeryAware, dontUseTextEditServices,
    reserved, reserved, reserved, 768 * 1024, 512 * 1024
};

data 'PICT' (128) {
    $$read("rainbow.pict")
};
