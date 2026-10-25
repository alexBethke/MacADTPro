#include "Dialogs.r"
#include "Menus.r"
#include "Processes.r"
#include "Types.r"
#include "SysTypes.r"

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
        {206, 20, 238, 304}, StaticText {disabled, "Ready."};
        {290, 270, 312, 364}, Button {enabled, "Connect"};
        {8, 20, 20, 360}, StaticText {disabled, "Locating image folder..."};
        {242, 20, 254, 254}, StaticText {disabled, "RX: 0 bytes   TX: 0 bytes"};
        /* LNew draws its 15-pixel vertical scroll bar to the right of rView. */
        {46, 20, 200, 345}, StaticText {disabled, ""};
        {28, 20, 46, 360}, StaticText {disabled, "Disk images in the selected folder:"};
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

resource 'MENU' (130) {
    130, textMenuProc;
    allEnabled, enabled;
    "Bootstrap";
    {
        "Load ADTPro 2.1.0 into memory...", noIcon, noKey, noMark, plain;
    }
};

resource 'MBAR' (128) {
    { 128, 129, 130 };
};

/* Finder bundle mapping. The signature matches the MADP creator code passed
   to Retro68's add_application() in CMakeLists.txt. */
type 'MADP' as 'STR ';
resource 'MADP' (0) { "MacADTPro" };

resource 'BNDL' (128) {
    'MADP', 0,
    {
        'ICN#', { 0, 128 },
        'FREF', { 0, 128 }
    }
};

resource 'FREF' (128) {
    'APPL', 0, ""
};

resource 'DLOG' (131) {
    {10, 58, 370, 422}, dBoxProc, visible, noGoAway, 0, 131,
    "Serial Bootstrap", centerMainScreen
};

resource 'DITL' (131) {
    {
        {328, 264, 350, 344}, Button {enabled, "Send"};
        {328, 168, 350, 248}, Button {enabled, "Cancel"};
        {14, 18, 104, 346}, StaticText {disabled,
            "1. Connect the serial cable. In Preferences select the Macintosh port and cable type.\r\r2. Start the Apple II without loading a disk. Press Control-Reset to reach the ] prompt."};
        {106, 18, 218, 346}, StaticText {disabled,
            "3. Type IN#2 and Return (use the serial card's slot; IIc/IIgs modem is slot 2).\r\r4. Press Control-A. At APPLE SSC: or blinking ?, type 14B. Press Return for an SSC."};
        {200, 18, 312, 346}, StaticText {disabled,
            "5. Click Send and wait for all three stages (9600 then automatic 19200).\r\r6. In ADTPro, format a blank disk, then Receive ADTPRO-2.1.0.DSK to make the boot disk."};
        {0, 0, 0, 0}, StaticText {disabled, ""};
    }
};

resource 'DLOG' (129) {
    {80, 86, 246, 406}, dBoxProc, invisible, noGoAway, 0, 129,
    "About MacADT Pro", centerMainScreen
};

resource 'DITL' (129) {
    {
        {130, 174, 152, 306}, Button {enabled, "Apple ][ forever"};
        /* Replaced with mixed-font drawing code after GetNewDialog. */
        {18, 20, 118, 300}, StaticText {disabled, ""};
        /* Replaced with the default-button frame before the dialog is shown. */
        {0, 0, 0, 0}, StaticText {disabled, ""};
    }
};

resource 'DLOG' (130) {
    {62, 76, 306, 436}, noGrowDocProc, invisible, goAway, 0, 130,
    "Preferences", centerMainScreen
};

resource 'DITL' (130) {
    {
        {204, 264, 226, 344}, Button {enabled, "OK"};
        {204, 168, 226, 248}, Button {enabled, "Cancel"};
        {12, 16, 30, 104}, StaticText {disabled, "Serial port"};
        {12, 120, 30, 224}, RadioButton {enabled, ""};
        {12, 244, 30, 344}, RadioButton {enabled, ""};
        {44, 16, 62, 344}, StaticText {disabled, "Transfer speed (baud)"};
        {68, 16, 86, 104}, RadioButton {enabled, ""};
        {68, 120, 86, 208}, RadioButton {enabled, ""};
        {68, 224, 86, 312}, RadioButton {enabled, ""};
        {92, 16, 110, 104}, RadioButton {enabled, ""};
        {92, 120, 110, 224}, RadioButton {enabled, ""};
        {124, 16, 144, 344}, CheckBox {enabled, "ImageWriter II cable"};
        {154, 16, 194, 344}, StaticText {disabled, ""};
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
