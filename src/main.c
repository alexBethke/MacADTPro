#include <Quickdraw.h>
#include <Fonts.h>
#include <Windows.h>
#include <Menus.h>
#include <TextEdit.h>
#include <Dialogs.h>
#include <Controls.h>
#include <Lists.h>
#include <Events.h>
#include <OSUtils.h>
#include <Processes.h>
#include <Resources.h>
#include <stdio.h>
#include <string.h>
#include "serial.h"
#include "wide.h"
#include "image_folder.h"
#include "directory.h"
#include "transfer.h"

/* Configuration scaffold. Values currently last only for this session. */
typedef struct {
    short printerPort;
    short baudIndex;
    short imageWriterCable;
    ImageFolder imageFolder;
} Configuration;

enum {
    kAppleMenu = 128,
    kFileMenu = 129,
    kAboutItem = 1,
    kPreferencesItem = 1,
    kQuitItem = 3,
    kMenuNone = 0,
    kMenuQuit = 1,
    kMenuPreferences = 2
};

enum { kVisibleFiles = 128 };
static unsigned char folderNames[kVisibleFiles][32];
static unsigned folderNameCount;
static Boolean folderHasMore;
static ListHandle folderList;

static void drawRainbowInRect(const Rect *bounds)
{
    PicHandle picture = GetPicture(128);
    if (!picture) return;
    DrawPicture(picture, bounds);
    ReleaseResource((Handle)picture);
}

static void drawRainbow(DialogPtr dialog)
{
    Rect bounds;
    SetRect(&bounds, 188, 205, 380, 325);
    SetPort((GrafPtr)dialog);
    drawRainbowInRect(&bounds);
}

static void frameFolderList(void)
{
    Rect frame;
    if (!folderList) return;
    frame = (**folderList).rView;
    /* Keep the frame outside pixels owned and redrawn by the List Manager.
       Its vertical scrollbar occupies the 15 pixels immediately right of rView. */
    --frame.left;
    --frame.top;
    frame.right += 16;
    ++frame.bottom;
    FrameRect(&frame);
}

static pascal void drawDefaultButton(DialogPtr dialog, short item)
{
    short type, buttonItem = item == 18 ? 13 : 1;
    Handle handle;
    Rect bounds;
    GetDialogItem(dialog, buttonItem, &type, &handle, &bounds);
    InsetRect(&bounds, -4, -4);
    PenSize(3, 3);
    FrameRoundRect(&bounds, 16, 16);
    PenNormal();
}

static void installDefaultButton(DialogPtr dialog, short frameItem)
{
    short type, buttonItem = frameItem == 18 ? 13 : 1;
    Handle handle;
    Rect bounds;
    GetDialogItem(dialog, buttonItem, &type, &handle, &bounds);
    InsetRect(&bounds, -4, -4);
    SetDialogItem(dialog, frameItem, userItem | itemDisable,
                  (Handle)NewUserItemUPP(drawDefaultButton), &bounds);
    SetDialogDefaultItem(dialog, buttonItem);
}

static pascal void drawAboutText(DialogPtr dialog, short item)
{
    static const unsigned char title[] =
        {10, 'M', 'a', 'c', 'A', 'D', 'T', ' ', 'P', 'r', 'o'};
    static const char body[] =
        "Classic Macintosh host for ADTPro disk image transfers. "
        "This application runs on any Macintosh with a 68020 or better CPU "
        "and System 7.0 or later. It is not affiliated with the MacADT app, "
        "provided as-is, without warranty of any kind. Use at your own risk.";
    GrafPtr savedPort;
    short type;
    Handle handle;
    Rect bounds, bodyBounds;
    GetDialogItem(dialog, item, &type, &handle, &bounds);
    GetPort(&savedPort);
    SetPort((GrafPtr)dialog);
    EraseRect(&bounds);
    TextFace(0);
    TextFont(systemFont); /* Chicago on the target System 7 systems. */
    TextSize(12);
    MoveTo(bounds.left, bounds.top + 14);
    DrawString(title);
    bodyBounds = bounds;
    bodyBounds.top += 26;
    TextFont(kFontIDGeneva);
    TextSize(9);
    TETextBox(body, sizeof(body) - 1, &bodyBounds, teJustLeft);
    TextFont(systemFont);
    TextSize(12);
    SetPort(savedPort);
}

static void installAboutText(DialogPtr dialog)
{
    short type;
    Handle handle;
    Rect bounds;
    GetDialogItem(dialog, 2, &type, &handle, &bounds);
    SetDialogItem(dialog, 2, userItem | itemDisable,
                  (Handle)NewUserItemUPP(drawAboutText), &bounds);
    DrawDialog(dialog);
}

static void showAbout(void)
{
    DialogPtr about = GetNewDialog(129, 0, (WindowPtr)-1);
    short item = 0;
    if (!about) return;
    installAboutText(about);
    while (item != 1) ModalDialog(0, &item);
    DisposeDialog(about);
}

static short handleMenu(long choice)
{
    short menu = (short)(choice >> 16);
    short item = (short)choice;
    short action = kMenuNone;
    if (menu == kAppleMenu && item == kAboutItem) showAbout();
    else if (menu == kFileMenu && item == kPreferencesItem) action = kMenuPreferences;
    else if (menu == kFileMenu && item == kQuitItem) action = kMenuQuit;
    HiliteMenu(0);
    return action;
}

static void setControl(DialogPtr dialog, short item, short value)
{
    short type;
    Handle handle;
    Rect bounds;
    GetDialogItem(dialog, item, &type, &handle, &bounds);
    if (GetControlValue((ControlHandle)handle) != value)
        SetControlValue((ControlHandle)handle, value);
}

static OSErr refreshFolderList(DialogPtr dialog, const ImageFolder *folder)
{
    unsigned i;
    Cell cell;
    OSErr error = directoryFolderItems(folder, folderNames, kVisibleFiles,
                                       &folderNameCount, &folderHasMore);
    if (error) return error;
    if (!folderList) return noErr;
    SetPort((GrafPtr)dialog);
    LSetDrawingMode(false, folderList);
    if ((**folderList).dataBounds.bottom)
        LDelRow((**folderList).dataBounds.bottom, 0, folderList);
    LAddRow(folderNameCount + (folderHasMore ? 1 : 0), 0, folderList);
    cell.h = 0;
    for (i = 0; i < folderNameCount; ++i) {
        cell.v = i;
        LSetCell(folderNames[i] + 1, folderNames[i][0], cell, folderList);
    }
    if (folderHasMore) {
        static const char more[] = "... more items ...";
        cell.v = folderNameCount;
        LSetCell(more, sizeof(more) - 1, cell, folderList);
    }
    LSetDrawingMode(true, folderList);
    TextFont(kFontIDGeneva);
    TextSize(9);
    LUpdate(((WindowPtr)dialog)->visRgn, folderList);
    frameFolderList();
    TextFont(systemFont);
    TextSize(12);
    return noErr;
}

static Boolean initFolderList(DialogPtr dialog)
{
    short type;
    Handle handle;
    Rect view, dataBounds;
    Point cellSize;
    GetDialogItem(dialog, 16, &type, &handle, &view);
    SetRect(&dataBounds, 0, 0, 1, 0);
    cellSize.h = 0;
    cellSize.v = 11;
    SetPort((GrafPtr)dialog);
    TextFont(kFontIDGeneva);
    TextSize(9);
    folderList = LNew(&view, &dataBounds, cellSize, 0, (WindowPtr)dialog,
                      true, false, false, true);
    frameFolderList();
    TextFont(systemFont);
    TextSize(12);
    return folderList != 0;
}

static unsigned char statusValues[3][256];
static Boolean statusDrawingReady;
enum {
    kStatusBufferWidth = 352,
    kStatusBufferHeight = 40,
    kStatusBufferRowBytes = ((kStatusBufferWidth + 15) / 16) * 2
};
static short statusPixels[kStatusBufferRowBytes * kStatusBufferHeight / 2];
static short statusBackgroundPixels[3][kStatusBufferRowBytes * kStatusBufferHeight / 2];
static BitMap statusBits;
static BitMap statusBackgroundBits[3];
static GrafPort statusPort;

static short statusIndex(short item)
{
    if (item == 14) return 1;
    if (item == 15) return 2;
    return 0;
}

static pascal void drawStatus(DialogPtr dialog, short item)
{
    GrafPtr savedPort;
    short type;
    Handle handle;
    Rect bounds, localBounds;
    short index = statusIndex(item);
    unsigned char *value = statusValues[index];
    GetDialogItem(dialog, item, &type, &handle, &bounds);
    GetPort(&savedPort);
    localBounds = bounds;
    OffsetRect(&localBounds, -bounds.left, -bounds.top);

    SetPort(&statusPort);
    RectRgn(statusPort.clipRgn, &localBounds);
    CopyBits(&statusBackgroundBits[index], &statusBits,
             &localBounds, &localBounds, srcCopy, 0);
    TextFont(kFontIDGeneva);
    TextSize(9);
    TextFace(0);
    TextMode(srcOr);
    TETextBox(value + 1, value[0], &localBounds, teJustLeft);
    TextMode(srcCopy);

    SetPort((GrafPtr)dialog);
    CopyBits(&statusBits, &((GrafPtr)dialog)->portBits,
             &localBounds, &bounds, srcCopy, 0);
    SetPort(savedPort);
}

static void initStatusDrawing(DialogPtr dialog)
{
    short item, type, index;
    Handle handle;
    Rect bounds, localBounds, rainbowBounds;
    GrafPtr savedPort;

    statusBits.baseAddr = (Ptr)statusPixels;
    statusBits.rowBytes = kStatusBufferRowBytes;
    SetRect(&statusBits.bounds, 0, 0, kStatusBufferWidth, kStatusBufferHeight);
    GetPort(&savedPort);
    OpenPort(&statusPort);
    SetPort(&statusPort);
    SetPortBits(&statusBits);
    PortSize(kStatusBufferWidth, kStatusBufferHeight);

    for (item = 12; item <= 15; ++item) {
        if (item == 13) continue;
        GetDialogItem(dialog, item, &type, &handle, &bounds);
        index = statusIndex(item);
        GetDialogItemText(handle, statusValues[index]);

        statusBackgroundBits[index].baseAddr = (Ptr)statusBackgroundPixels[index];
        statusBackgroundBits[index].rowBytes = kStatusBufferRowBytes;
        SetRect(&statusBackgroundBits[index].bounds, 0, 0,
                kStatusBufferWidth, kStatusBufferHeight);
        SetPortBits(&statusBackgroundBits[index]);
        localBounds = bounds;
        OffsetRect(&localBounds, -bounds.left, -bounds.top);
        RectRgn(statusPort.clipRgn, &localBounds);
        EraseRect(&localBounds);
        SetRect(&rainbowBounds, 188 - bounds.left, 205 - bounds.top,
                380 - bounds.left, 325 - bounds.top);
        drawRainbowInRect(&rainbowBounds);

        SetDialogItem(dialog, item, userItem | itemDisable,
                      (Handle)NewUserItemUPP(drawStatus), &bounds);
    }
    SetPortBits(&statusBits);
    PortSize(kStatusBufferWidth, kStatusBufferHeight);
    SetPort(savedPort);
    statusDrawingReady = true;
}

static void setText(DialogPtr dialog, short item, const char *text)
{
    short type;
    Handle handle;
    Rect bounds;
    unsigned char value[256], previous[256];
    size_t length = strlen(text);
    if (length > 255) length = 255;
    value[0] = length;
    memcpy(value + 1, text, length);
    if (statusDrawingReady && (item == 12 || item == 14 || item == 15)) {
        unsigned char *cached = statusValues[statusIndex(item)];
        if (cached[0] == value[0] && !memcmp(cached + 1, value + 1, length)) return;
        memcpy(cached, value, length + 1);
        drawStatus(dialog, item);
        return;
    }
    GetDialogItem(dialog, item, &type, &handle, &bounds);
    if ((type & 0x7f) >= ctrlItem && (type & 0x7f) <= (ctrlItem + 3))
    {
        GetControlTitle((ControlHandle)handle, previous);
        if (previous[0] != value[0] || memcmp(previous + 1, value + 1, length))
            SetControlTitle((ControlHandle)handle, value);
    } else {
        GetDialogItemText(handle, previous);
        if (previous[0] != value[0] || memcmp(previous + 1, value + 1, length)) SetDialogItemText(handle, value);
    }
}

static void enableSettings(DialogPtr dialog, short enabled)
{
    static const short items[] = {4, 5, 7, 8, 9, 10, 11, 12};
    short i, type;
    Handle handle;
    Rect bounds;
    for (i = 0; i < (short)(sizeof(items) / sizeof(items[0])); ++i) {
        GetDialogItem(dialog, items[i], &type, &handle, &bounds);
        HiliteControl((ControlHandle)handle, enabled ? 0 : 255);
    }
}

static void setPreferenceControls(DialogPtr dialog, const Configuration *config)
{
    short i;
    setControl(dialog, 4, !config->printerPort);
    setControl(dialog, 5, config->printerPort);
    for (i = 0; i < 5; ++i) setControl(dialog, 7 + i, config->baudIndex == i);
    setControl(dialog, 12, config->imageWriterCable);
}

/* Always attempt both cleanup operations; file errors must not retain a port. */
static OSErr releaseConnection(SerialConnection *serial, ImageTransfer *transfer)
{
    OSErr portError = serialClose(serial);
    OSErr fileError = transferCancel(transfer);
    return portError ? portError : fileError;
}

int main(void)
{
    Configuration config = {0, 4, 0, {0, 0}};
    Configuration preferenceDraft;
    DialogPtr dialog;
    DialogPtr preferences = 0;
    Handle menuBar;
    short item;
    static SerialConnection serial;
    static ImageTransfer transfer;
    WideParser parser;
    EventRecord event;
    DialogPtr eventDialog;
    Boolean connected = false, quit = false, quitRequested, releasedByClick;
    Boolean closePreferences, dispatchDialog;
    short menuAction;
    unsigned long received = 0, commands = 0, lastByte = 0;
    unsigned char bytes[256];
    char status[256];
    OSErr error;
    OSErr folderError;
    static unsigned char directoryData[1024], directoryPacket[2060];
    unsigned short directoryLength = 0, directoryNumber = 0;
    Boolean awaitingDirectory = false;
    short directoryRetries = 0;
    unsigned long ackStarted = 0;
    unsigned long diagnosticTick = 0;
    wideReset(&parser);

    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(0);
    InitCursor();
    menuBar = GetNewMBar(128);
    if (!menuBar) return 1;
    SetMenuBar(menuBar);
    DrawMenuBar();
    dialog = GetNewDialog(128, 0, (WindowPtr)-1);
    if (!dialog) return 1;
    installDefaultButton(dialog, 18);
    initStatusDrawing(dialog);
    SetPort((GrafPtr)dialog);
    drawRainbow(dialog);
    DrawDialog(dialog);
    if (!initFolderList(dialog)) {
        DisposeDialog(dialog);
        return 1;
    }
    folderError = imageFolderDefault(&config.imageFolder);
    if (folderError) {
        sprintf(status, "Cannot use disks beside the app (Mac error %d).", folderError);
        setText(dialog, 14, status);
        {
            short type;
            Handle handle;
            Rect bounds;
            GetDialogItem(dialog, 13, &type, &handle, &bounds);
            HiliteControl((ControlHandle)handle, 255);
        }
    } else {
        setText(dialog, 14, "Image folder: disks (beside this application)");
        error = refreshFolderList(dialog, &config.imageFolder);
        if (error) {
            sprintf(status, "Cannot list disks folder (Mac error %d).", error);
            setText(dialog, 12, status);
        }
    }

    do {
        item = 0;
        eventDialog = 0;
        quitRequested = false;
        menuAction = kMenuNone;
        closePreferences = false;
        dispatchDialog = true;
        releasedByClick = false;
        if (WaitNextEvent(everyEvent, &event, connected ? 1 : 6, 0)) {
            if (event.what == activateEvt && (WindowPtr)event.message == (WindowPtr)dialog &&
                (event.modifiers & activeFlag) && !folderError) {
                LActivate(true, folderList);
                error = refreshFolderList(dialog, &config.imageFolder);
                if (error) {
                    sprintf(status, "Cannot refresh disks folder (Mac error %d).", error);
                    setText(dialog, 12, status);
                }
            }
            if (event.what == activateEvt && (WindowPtr)event.message == (WindowPtr)dialog &&
                !(event.modifiers & activeFlag)) LActivate(false, folderList);
            if (event.what == keyDown && (event.modifiers & cmdKey)) {
                menuAction = handleMenu(MenuKey(event.message & charCodeMask));
                dispatchDialog = false;
            } else if (event.what == keyDown &&
                       ((event.message & charCodeMask) == '\r' ||
                        (event.message & charCodeMask) == 3)) {
                if (FrontWindow() == (WindowPtr)preferences && preferences) {
                    eventDialog = preferences;
                    item = 1;
                } else if (FrontWindow() == (WindowPtr)dialog) {
                    eventDialog = dialog;
                    item = 13;
                }
                dispatchDialog = false;
            }
            if (event.what == mouseDown) {
                WindowPtr clickedWindow;
                short part = FindWindow(event.where, &clickedWindow);
                if (part == inMenuBar) {
                    menuAction = handleMenu(MenuSelect(event.where));
                    dispatchDialog = false;
                }
                if (part == inGoAway && clickedWindow == (WindowPtr)dialog) {
                    if (TrackGoAway(clickedWindow, event.where)) quitRequested = true;
                    dispatchDialog = false;
                }
                if (part == inGoAway && clickedWindow == (WindowPtr)preferences) {
                    if (TrackGoAway(clickedWindow, event.where)) closePreferences = true;
                    dispatchDialog = false;
                }
                if (part == inDrag && (clickedWindow == (WindowPtr)dialog ||
                                       clickedWindow == (WindowPtr)preferences)) {
                    DragWindow(clickedWindow, event.where, &qd.screenBits.bounds);
                    dispatchDialog = false;
                }
                if (part == inSysWindow) {
                    SystemClick(&event, clickedWindow);
                    dispatchDialog = false;
                }
                if (part == inContent && clickedWindow == (WindowPtr)dialog && connected) {
                    /* Release before DialogSelect can enter control tracking. */
                    error = releaseConnection(&serial, &transfer);
                    connected = false;
                    awaitingDirectory = false;
                    wideReset(&parser);
                    releasedByClick = true;
                    if (preferences) enableSettings(preferences, true);
                    setText(dialog, 13, "Connect");
                    if (error) {
                        sprintf(status, "Cleanup error %d. Quit or Disconnect to retry.", error);
                        setText(dialog, 12, status);
                        continue;
                    }
                    setText(dialog, 12, "Disconnected by window click.");
                }
                if (part == inContent && clickedWindow == (WindowPtr)dialog && folderList) {
                    Point local = event.where;
                    Rect listAndScrollBar = (**folderList).rView;
                    listAndScrollBar.right += 16;
                    SetPort((GrafPtr)dialog);
                    GlobalToLocal(&local);
                    if (PtInRect(local, &listAndScrollBar)) {
                        TextFont(kFontIDGeneva);
                        TextSize(9);
                        LClick(local, event.modifiers, folderList);
                        TextFont(systemFont);
                        TextSize(12);
                        dispatchDialog = false;
                    }
                }
            }
            if (event.what == updateEvt && (WindowPtr)event.message == (WindowPtr)dialog) {
                SetPort((GrafPtr)dialog);
                BeginUpdate((WindowPtr)dialog);
                drawRainbow(dialog);
                DrawDialog(dialog);
                TextFont(kFontIDGeneva);
                TextSize(9);
                LUpdate(((WindowPtr)dialog)->visRgn, folderList);
                frameFolderList();
                TextFont(systemFont);
                TextSize(12);
                EndUpdate((WindowPtr)dialog);
                dispatchDialog = false;
            }
            if (dispatchDialog && IsDialogEvent(&event)) DialogSelect(&event, &eventDialog, &item);
        }
        if (closePreferences) {
            DisposeDialog(preferences);
            preferences = 0;
        }
        if (menuAction == kMenuQuit) quitRequested = true;
        if (menuAction == kMenuPreferences) {
            if (!preferences) {
                preferenceDraft = config;
                preferences = GetNewDialog(130, 0, (WindowPtr)-1);
                if (preferences) {
                    installDefaultButton(preferences, 14);
                    setPreferenceControls(preferences, &preferenceDraft);
                    enableSettings(preferences, !connected);
                }
            } else SelectWindow((WindowPtr)preferences);
        }
        if (eventDialog == preferences && preferences) {
            if (!connected) {
                if (item == 4 || item == 5) preferenceDraft.printerPort = item == 5;
                if (item >= 7 && item <= 11) preferenceDraft.baudIndex = item - 7;
                if (item == 12) preferenceDraft.imageWriterCable = !preferenceDraft.imageWriterCable;
                setPreferenceControls(preferences, &preferenceDraft);
            }
            if (item == 1) {
                config.printerPort = preferenceDraft.printerPort;
                config.baudIndex = preferenceDraft.baudIndex;
                config.imageWriterCable = preferenceDraft.imageWriterCable;
                DisposeDialog(preferences);
                preferences = 0;
            } else if (item == 2) {
                DisposeDialog(preferences);
                preferences = 0;
            }
            item = 0;
        }
        /* Stop polling/dispatching as soon as Quit is selected. */
        if (quitRequested) {
            error = releaseConnection(&serial, &transfer);
            if (error) {
                connected = false;
                awaitingDirectory = false;
                sprintf(status, "Serial close failed (Mac error %d). Quit again to retry.", error);
                setText(dialog, 12, status);
                if (preferences) enableSettings(preferences, true);
                setText(dialog, 13, "Connect");
                continue;
            }
            quit = true;
            break;
        }
        if (item == 13 && !folderError && !releasedByClick) {
            if (connected) {
                error = releaseConnection(&serial, &transfer);
                connected = false;
                awaitingDirectory = false;
                if (error) {
                    sprintf(status, "Serial close failed (Mac error %d).", error);
                    setText(dialog, 12, status);
                } else setText(dialog, 12, "Disconnected.");
            } else {
                error = serialOpen(&serial, config.printerPort, config.baudIndex,
                                   config.imageWriterCable);
                connected = error == noErr;
                received = commands = 0;
                wideReset(&parser);
                awaitingDirectory = false;
                if (error) sprintf(status, "Cannot open serial port (Mac error %d).", error);
                else strcpy(status, "Connected. Waiting for Apple II commands.");
                setText(dialog, 12, status);
            }
            if (preferences) enableSettings(preferences, !connected);
            setText(dialog, 13, connected ? "Disconnect" : "Connect");
        }
        if (connected) {
            long count = sizeof(bytes), n;
            error = serialPollWrite(&serial);
            if (awaitingDirectory && serial.writing) ackStarted = TickCount();
            if (!error) error = serialRead(&serial, bytes, &count);
            if (error) {
                sprintf(status, "Serial error %d. Disconnected.", error);
                { OSErr cleanupError = releaseConnection(&serial, &transfer);
                  if (cleanupError) sprintf(status, "Serial error %d; cleanup error %d. Quit to retry.", error, cleanupError); }
                connected = false;
                if (preferences) enableSettings(preferences, true);
                setText(dialog, 13, "Connect");
                setText(dialog, 12, status);
            } else {
                for (n = 0; n < count; ++n) {
                    int result = wideFeed(&parser, bytes[n]);
                    ++received;
                    lastByte = TickCount();
                    if (result == WIDE_COMMAND) {
                        static const unsigned char home[] = {0xc1, 0, 0, 0xd8, 0x19};
                        ++commands;
                        {
                            Boolean handled;
                            Boolean wasActive = transfer.state != 0;
                            error = transferCommand(&transfer, &serial, &config.imageFolder, &parser, &handled);
                            if (error) break;
                            if (handled) {
                                awaitingDirectory = false;
                                setText(dialog, 12, transfer.message);
                                if (wasActive && !transfer.state) {
                                    error = releaseConnection(&serial, &transfer);
                                    connected = false;
                                    if (preferences) enableSettings(preferences, true);
                                    setText(dialog, 13, "Connect");
                                    if (error) {
                                        sprintf(status, "Transfer stopped; cleanup error %d. Quit to retry.", error);
                                        setText(dialog, 12, status);
                                    }
                                    break;
                                }
                                continue;
                            }
                            if (transfer.state) {
                                /* Unexpected/noisy commands must not end an image transfer. */
                                error = transferRetry(&transfer);
                                setText(dialog, 12, transfer.message);
                                if (error || !transfer.state) break;
                                continue;
                            }
                        }
                        if (parser.command == 0xc4 && parser.length <= sizeof(parser.payload)) {
                            if (serial.writing) { error = paramErr; break; }
                            error = directoryPage(&config.imageFolder, parser.payload,
                                                  parser.length, directoryData, &directoryNumber);
                            if (error) { sprintf(status, "Cannot read disks folder (Mac error %d).", error);
                                setText(dialog, 12, status); error = noErr; }
                            else {
                                directoryLength = wideEncode(directoryData, sizeof(directoryData),
                                    directoryNumber, directoryPacket, sizeof(directoryPacket));
                                if (!directoryLength) { error = paramErr; break; }
                                error = serialSend(&serial, directoryPacket, directoryLength);
                                if (error) break;
                                awaitingDirectory = true;
                                directoryRetries = 0;
                                ackStarted = TickCount();
                                setText(dialog, 12, "Sending directory of disks. Waiting for Apple II acknowledgement.");
                                continue;
                            }
                        }
                        if (parser.command == 0xcb && awaitingDirectory) {
                            unsigned short next = parser.payload[1] | ((unsigned short)parser.payload[2] << 8);
                            if (parser.length == 3 && parser.payload[0] == 6 && next == directoryNumber + 2) {
                                awaitingDirectory = false;
                                setText(dialog, 12, "Directory acknowledged by Apple II.");
                            } else if (parser.length == 3 && parser.payload[0] == 0x18) {
                                awaitingDirectory = false;
                                setText(dialog, 12, "Directory cancelled by Apple II.");
                            } else if (++directoryRetries <= 3 && !serial.writing) {
                                error = serialSend(&serial, directoryPacket, directoryLength);
                                ackStarted = TickCount();
                                setText(dialog, 12, "Retrying directory packet.");
                            } else {
                                awaitingDirectory = false;
                                setText(dialog, 12, "Directory failed after retries.");
                            }
                            continue;
                        }
                        awaitingDirectory = false;
                        /* Return unsupported commands to the client menu. */
                        if (parser.command != 0xd8) {
                            if (serial.writing) { error = paramErr; break; }
                            error = serialSend(&serial, home, sizeof(home));
                            if (error) break;
                        }
                        sprintf(status, "Unsupported command $%02X.\rBytes: %lu  Commands: %lu",
                                parser.command, received, commands);
                        setText(dialog, 12, status);
                    } else if (result == WIDE_BAD_HEADER || result == WIDE_BAD_PAYLOAD) {
                        if (transfer.state) {
                            error = transferRetry(&transfer);
                            setText(dialog, 12, transfer.message);
                            if (error || !transfer.state) {
                                OSErr cleanupError = releaseConnection(&serial, &transfer);
                                connected = false;
                                if (preferences) enableSettings(preferences, true);
                                setText(dialog, 13, "Connect");
                                if (cleanupError) {
                                    sprintf(status, "Retry limit reached; cleanup error %d.", cleanupError);
                                    setText(dialog, 12, status);
                                }
                                break;
                            }
                            continue;
                        }
                        error = releaseConnection(&serial, &transfer);
                        connected = false;
                        awaitingDirectory = false;
                        if (preferences) enableSettings(preferences, true);
                        setText(dialog, 13, "Connect");
                        if (error) sprintf(status, "Checksum error at block %u; cleanup error %d.", transfer.block, error);
                        else sprintf(status, "Checksum error at block %u. Transfer stopped and disconnected.", transfer.block);
                        setText(dialog, 12, status);
                        wideReset(&parser);
                        break;
                    } else if (result == WIDE_PING) {
                        setText(dialog, 12, "Apple II ping received.");
                    }
                }
                if (!connected) continue;
                if (count && !parser.state && !commands)
                    { sprintf(status, "Receiving serial data: %lu bytes.", received); setText(dialog, 12, status); }
                if (parser.state && (unsigned long)(TickCount() - lastByte) > 300) {
                    wideReset(&parser);
                    if (transfer.state) {
                        error = transferRetry(&transfer);
                        setText(dialog, 12, transfer.message);
                        if (!error && transfer.state) continue;
                    }
                    error = releaseConnection(&serial, &transfer);
                    connected = false;
                    awaitingDirectory = false;
                    if (preferences) enableSettings(preferences, true);
                    setText(dialog, 13, "Connect");
                    if (error) sprintf(status, "Packet timeout. Transfer stopped; cleanup error %d.", error);
                    else strcpy(status, "Packet timeout. Transfer stopped and disconnected.");
                    setText(dialog, 12, status);
                    continue;
                }
                if (error) {
                    OSErr cleanupError = releaseConnection(&serial, &transfer);
                    connected = false;
                    if (preferences) enableSettings(preferences, true);
                    setText(dialog, 13, "Connect");
                    if (cleanupError) {
                        sprintf(status, "Output error %d; cleanup error %d. Quit to retry.", error, cleanupError);
                        setText(dialog, 12, status);
                    } else setText(dialog, 12, "Serial output busy or failed. Disconnected.");
                }
                if (connected && transfer.state) {
                    char previous[128];
                    strcpy(previous, transfer.message);
                    Boolean wasWriting = serial.writing;
                    error = transferPoll(&transfer, &serial);
                    /* Discard any incomplete stale envelope before a new packet ACK. */
                    if (!wasWriting && serial.writing) wideReset(&parser);
                    if (strcmp(previous, transfer.message)) setText(dialog, 12, transfer.message);
                    if (error || !transfer.state) {
                        { OSErr cleanupError = releaseConnection(&serial, &transfer);
                          if (cleanupError) error = cleanupError; }
                        connected = false;
                        if (preferences) enableSettings(preferences, true);
                        setText(dialog, 13, "Connect");
                        if (error) sprintf(status, "Image transfer/cleanup error %d. Quit to retry.", error);
                        else snprintf(status, sizeof(status), "%s Disconnected.", transfer.message);
                        setText(dialog, 12, status);
                    }
                }
                if (!connected) continue;
                if (awaitingDirectory && !serial.writing && (unsigned long)(TickCount() - ackStarted) > 900) {
                    awaitingDirectory = false;
                    setText(dialog, 12, "No directory acknowledgement. Check baud rate, cable and handshaking.");
                }
                if ((unsigned long)(TickCount() - diagnosticTick) >= 30) {
                    OSErr diagnosticError = serialDiagnostics(&serial);
                    diagnosticTick = TickCount();
                    if (diagnosticError)
                        sprintf(status, "RX: %lu TX: %ld  Status error: %d", received, serial.sentBytes, diagnosticError);
                    else
                        sprintf(status, "RX: %lu TX: %ld  CTS: %s\rWrite: %s DTR: %s Errors: $%02X",
                            received, serial.sentBytes, serial.ctsHeld ? "blocked" : "not held",
                            serial.writing ? "pending" : "idle", serial.dtrHeld ? "yes" : "no", serial.driverErrors);
                    setText(dialog, 15, status);
                }
            }
        }
    } while (!quit);

    if (preferences) DisposeDialog(preferences);
    if (folderList) LDispose(folderList);
    ClosePort(&statusPort);
    DisposeDialog(dialog);
    /* Native Toolbox exit after owned resources have been released.
       No C streams or application destructors need runtime exit cleanup. */
    ExitToShell();
    return 0;
}
