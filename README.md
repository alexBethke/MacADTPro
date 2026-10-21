# MacADTPro

Proposed native classic Macintosh ADTPro host, built with Retro68, with
serial transport and ADTPro 2.1.0 client compatibility.

Target: System 7.1 on 68k Macintosh. The assumed role is a Macintosh server
serving Apple II disk images. This repository includes serial connection and
Wide protocol diagnostics, directory listings, image-size queries and
Mac-to-Apple-II transfers of raw ProDOS-order `.po` and 140 KB DOS-order
`.dsk`/`.do` images.

## Build

```sh
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=/Users/alex/Git/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
cmake --build build
```

Adjust the toolchain path for other installations. Retro68 produces
`build/MacADTPro.bin` (MacBinary) and `build/MacADTPro.dsk` for transfer or emulator
use. Preferences currently last for the session only. The original configuration
UI was tested successfully by the user. The serial diagnostic build has passed
cross-compilation; physical serial I/O still needs testing.

## Current serial diagnostic

Any mouse click in the application's window while connected now cancels the
transfer and releases the serial port before Toolbox control tracking. Click
Connect again to reconnect. The same button toggles between Connect and Disconnect. Quit, window-click disconnect and I/O error handling share
one cleanup path. Serial cleanup is attempted even if closing an image fails;
both serial channels are attempted even if one cleanup operation fails. Failed
cleanup is reported and the application does not quit until cleanup succeeds.
This behavior is intended to address the reported connected-window crashes;
physical verification remains pending.

To receive an image, put a `.po` or DOS-order `.dsk`/`.do` file in `disks`, connect, and
choose Receive on the Apple II. Select the image or enter its filename, then
choose the destination disk. This writes the destination Apple II disk. Do not
rename DOS-order `.dsk` files to `.po`. The app converts `.dsk`/`.do` sector
ordering on the fly, using the sector pairing in ADTPro 2.1.0's AppleCommander
library. The image file is not modified. `.dsk`/`.do` requires complete 4096-byte tracks (16 sectors per track);
35-track images contain 143360 bytes, but smaller and larger images are accepted.
The transfer count comes from the file size divided by 512, with a short final
packet when needed; empty sectors remain part of the image.
Use `.po` for images stored in ProDOS block order, including ProDOS filesystems
that happen to have a `.dsk` extension. Filename extension describes image byte
ordering here, not the filesystem inside the image.

The app streams read-only files using the requested packet size (up to 40 blocks), honoring the client's
preferred packet size and 40-block buffer boundaries, retries NAKs up to ten
times, and requires the final client ACK before reporting completion. Cancel,
timeout, Disconnect and Quit close the image file. Completion or cancellation
also releases the serial port; reconnect before another operation. During image transfer, malformed acknowledgements or checksum errors retry
the current packet up to ten times, with increasing pauses from 250 ms to one
second. Bursts of damaged envelopes count as one retry until the packet is actually
resent; duplicate ACKs are ignored. ACK deadlines allow 120 seconds. Serial output
also allows two minutes of handshaking delay beyond the estimated wire time. Explicit client cancellation still stops immediately;
a retry limit or initial ACK timeout stops and disconnects. Failed image-file closure retains the
file reference for cleanup retries but never keeps transmission active.
Indexed selection uses the
most recent displayed directory page; manual filenames are also supported.
Preferred memory is 768 KB, minimum 512 KB. This leaves sufficient application
heap for the main window and the separate Preferences window on System 7.

2IMG, nibble images, nonstandard `.dsk` sizes, Apple-II-to-Mac uploads and directory
changes remain unsupported. Transfer tests pass on the host; physical image
transfer and the serial crash fix still need confirmation.

The main interface is a standard fixed-size document window, 380 by 304 pixels,
sized to fit a 512 by 384 screen with room for the menu bar and window frame.
The Apple menu contains About MacADT Pro, and the File menu contains Quit
(Command-Q). File also contains Preferences (Command-.), which opens a separate
window for the serial port, baud rate, cable option, and their explanatory text.
The main window lists image files from the `disks` folder in a compact, scrollable
list and keeps the connection and transfer status visible. Connect is the main
window's default button; OK is the Preferences default button, so Return or Enter
activates either one when its window is frontmost. The window close box also quits
through the same cleanup path.

Select Modem or Printer, choose the same baud rate as the Apple II client, and
click Connect. Settings are locked until Disconnect. If using the Printer port,
turn off AppleTalk first if it owns that port. Errors include the Mac OS error
number. Incoming Wide envelopes and payload XOR checksums are validated without
allocating buffers based on incoming lengths. Directory requests receive an
18-file page from `disks` with wildcard filtering, differential RLE, CRC-CCITT,
and acknowledgements. Files appear in HFS catalog order; subdirectories are
currently omitted. Unsupported commands return the
upstream Go Home envelope to the Apple II; they do not perform disk operations.
The status reports commands, acknowledgements and RX/TX byte counts. TX counts
driver-completed bytes; it does not prove receipt by the Apple II.

The default image folder is `disks` beside the running application. On startup,
the app uses the Process Manager to find its actual parent folder and creates
`disks` if missing. Existing folders and images are preserved. A file named
`disks`, or a volume that prevents creation, produces an error and disables
Connect. On read-only media, create the folder before copying the app to it.

The ImageWriter II checkbox enables CTS output gating and DTR receive flow
control through driver control code 14. Physical compatibility depends on the
cable and endpoint. The host asserts DTR/HSKo on connection. Writes run
asynchronously with a deadline allowing 300-baud transmission plus ten seconds so
missing handshake signals cannot freeze the UI. Incoming reads consume only
bytes already buffered by the driver. Disconnect cancels pending output and
detaches the application-owned receive buffer.

One connection button toggles between Connect and Disconnect. Button titles are
updated with SetControlTitle; SetDialogItemText is used only for text items.
The previous use of a text-item API on control handles corrupted controls.
The separate middle button has been removed.

File > Quit, Command-Q, or the window close box stops command processing immediately, cancels I/O on both channels,
detaches the system-heap receive buffer, closes input then output, frees the
buffer, disposes the dialog, and calls the native `ExitToShell` trap. Every
serial cleanup result is checked. If cleanup fails, Quit keeps the app alive
and reports the Mac OS error so it can be retried. This shutdown revision is
intended to address the reported System Error 1 after connecting; confirmation
on physical System 7.1 hardware is pending.

The SE/30-to-IIe setup has displayed an empty directory successfully. Bus errors
on TX Test and Quit remain under investigation. The current revision replaces
serial interface-library helpers with fully zero-initialized Device Manager
parameter blocks and word-aligns the transmit buffer. Compile-time assertions
check buffer/PB alignment. Physical confirmation of this revision is pending.

Host-side parser checks:

```sh
cc -std=c99 -Wall -Wextra -Isrc tests/wide_test.c src/wide.c -o build/wide_test
./build/wide_test
cc -std=c99 -Wall -Wextra -Itests/mac_stubs -Isrc tests/transfer_test.c src/transfer.c src/wide.c -o build/transfer_test
./build/transfer_test
```

## Proposed configuration

| Setting | Proposal |
| --- | --- |
| Serial port | Modem or Printer; default Modem. Report ports unavailable or in use. |
| Transfer baud | 300, 1200, 2400, 9600, 19200; default 19200. 115200 is optional and is not a release blocker. Expose it only with a verified driver/hardware path. Never silently substitute a different rate. |
| Cable profile | Standard serial cable or ImageWriter II cable; map to verified Macintosh handshake behavior and document endpoint-specific wiring. |
| Image folder | Default `disks` beside the application, created automatically if missing. A custom-folder selector may be added later. |
| Write access | Read-only or permit incoming disk images; default read-only. |
| Existing images | Reject overwrites by default. Receive to a temporary file and publish only after successful completion. |
| Bootstrap | Separate target/profile and initial speed from normal transfer speed. Optional after basic transfer support. |
| Diagnostics | Optional file log with protocol errors, retries, and serial overruns. Off by default. |

Keep framing fixed at 8 data bits, no parity, one stop bit, matching ADTPro.
Disable XON/XOFF for binary transfers. Avoid exposing arbitrary framing options.
Persist preferences in a versioned preferences file, using APIs appropriate to the
minimum supported System version.

Advanced timeout/retry and payload controls should initially follow the upstream
protocol. At 300 baud, deadlines must allow actual packet transmission time;
the Apple II client's deadlines and supported rates also need verification.
An app offering a baud rate does not establish that every client supports it.

## Protocol and implementation

Implement the ADTPro 2.x "Wide" protocol from the pinned `v_2_1_0` sources.
It was introduced in 2.0.0; extending legacy MacADT framing is insufficient.
Before implementation, extract commands, byte order, packet boundaries,
compression, checksums, acknowledgements, retry rules, and abort behavior from
both the upstream host and Apple II client. Record byte-level fixtures.

Separate a portable C/C++ protocol engine from Macintosh Serial Driver, HFS
file access, and Toolbox UI adapters. Use bounded buffers and streaming file
access rather than keeping a whole disk image in RAM. Use asynchronous serial
I/O with a cooperative event loop, so Cancel and window updates remain usable.

The local Apple Serial.h defines standard SerReset baud constants through
57600, plus driver control code `kSERD115KBaud` (115). That control code's presence
in newer headers does not guarantee System 7.1 driver/hardware support. Higher
rates remain optional pending testing. Avoid direct SCC programming initially.

An ImageWriter cable setting cannot electrically change a cable into another
pinout. ADTPro's transport offers hardware flow control on/off. Inspect how its
GUI checkbox enables hardware handshaking (verified in SerialConfig.java).
Map it to classic Mac handshake lines for each supported endpoint. Do not assume modern PC RTS/CTS maps directly
to every Macintosh port. Printer-port ownership may conflict with AppleTalk.

## Delivery stages

1. Confirm Apple II endpoints and actual cables; target System 7.1 on 68k.
2. Build a Retro68 Toolbox app with port/speed settings and a serial diagnostic.
3. Implement directory listing and host-to-Apple-II image transfer using Wide.
4. Implement Apple-II-to-host transfer, safe file creation, and saved preferences.
5. Add serial bootstrapping with bundled matching upstream client assets.

Initially prioritize raw DOS-order `.dsk`/`.do` and ProDOS-order `.po` images;
verify format interpretation against upstream before declaring support. Treat
2IMG, nibble images, batch operations, and virtual drives as separate scope.

## Validation required before release

- Byte-exact protocol tests against fixtures from ADTPro 2.1.0, including
  corrupt packets, timeouts, retransmissions, cancellation, and resynchronization.
- A Retro68 cross-build and emulator checks for UI and HFS behavior.
- Physical Mac-to-Apple-II transfers in both directions with hash comparison,
  across supported rates and cable profiles.
- Explicit 115200 and low-speed tests; report unsupported combinations honestly.

## References

- [ADTPro releases and Wide introduction](https://github.com/ADTPro/adtpro/releases)
- [Pinned ADTPro 2.1.0 serial transport](https://github.com/ADTPro/adtpro/blob/v_2_1_0/src/org/adtpro/transport/SerialTransport.java)
- [Retro68](https://github.com/autc04/Retro68)

The protocol implementation adapts ADTPro's GPL-2.0-or-later source; see LICENSE
and its source attribution.

The Bootstrap menu loads the matching ADTPro 2.1.0 serial client directly into
Apple II memory. It sends the upstream monitor loader, ProDOS MLI, and client in
three automatic stages. Monitor text starts at the upstream default of 9600
baud; the bootstrapper then changes both ends to 19200. The original 115200
defaults are changed in transit because the classic Macintosh Serial Manager/SCC
path does not offer 115200 baud. The official `ADTPRO-2.1.0.DSK` is copied to `build/disks`, where
the newly bootstrapped client can receive it to create a bootable floppy.

The bundled payloads and disk image are from the official ADTPro 2.1.0 release
(tag `v_2_1_0`, commit `31227fe4c88494f722f5a342d13ad2ff998e0017`).
Their unmodified upstream SHA-256 checksums are:

- `grub2.dmp`: `0c4e4d125ca70811f6c39f974fae262cf91e12c9a2b8ee054fb2382ca025e186`
- `PDSpeed.raw`: `4c8d617c4d0b9d00aa7bbbd3f47e860ccbaa1a0ec0fb77f274598f93f4fac47f`
- `adtpro.raw`: `c676b85df88e5c2115227640123ed7eb54993eded69c667806b5d94e1875ae03`
- `ADTPRO-2.1.0.DSK`: `e63776271c8025033e0dcfb0b137b21e6597810c4886688fb21180e02bd10048`

### Serial bootstrap procedure

1. Connect the Apple II and Macintosh serial ports. In MacADTPro Preferences,
   select the correct Macintosh port and cable type. Bootstrap starts at 9600
   and automatically switches to 19200, regardless of the normal speed setting.
2. Start the Apple II without loading an operating system. Press Control-Reset
   to reach the Applesoft `]` prompt.
3. Type `IN#2` and press Return. Replace `2` with the slot containing the Super
   Serial Card; the IIc and IIgs modem port use virtual slot 2.
4. Press Control-A. At `APPLE SSC:` or the blinking `?`, type `14B`. Press
   Return on a Super Serial Card; doing so is harmless on a IIc/IIgs.
5. In MacADTPro choose **Bootstrap > Load ADTPro 2.1.0 into memory...**, review
   the same on-screen instructions, and click **Send**.
6. Wait for all three status stages. The Apple II first shows monitor text, then
   the bootstrapper screen with changing progress characters, and finally the
   ADTPro client. Do not reset it or change baud rates during this sequence.
7. To make a permanent boot disk, insert and format a floppy in the Apple II,
   then receive `ADTPRO-2.1.0.DSK` from the Macintosh `disks` folder.

If the Apple II reports a syntax error before loading begins, press Return a few
times and repeat steps 3-5. A Franklin ACE 500 instead needs `PR#2`, Control-A
`14B`, `PR#0`, and `IN#2`, pressing Return after each command.

## Completion correction

The two-block host cap was removed: the Apple II advances by its configured
BAOCNT, so the host must honor that count, reducing packets only for the final
partial buffer/40-block boundary as upstream does. The CRC encoder now uses a
lookup table to reduce 68k encoding time. Boundary and final ACK deadlines allow
60 seconds for physical disk writes. Completion requires the final ACK for the
image's block count. A bootable disk alone does not verify all blocks transferred.
The regression test covers a 280-block transfer with client BAOCNT 10.

The retry build preserves the correct client-requested packet sizes; it does not
reinstate the incompatible two-block packet cap. Host regression tests verify
that an invalid ACK does not immediately cancel, retransmission is delayed, and
the fourth failed attempt stops the state machine.

The resilience build compares against the shared Windows/Linux Java server in
ADTPro tag `v_2_1_0`, `CommsThread.sendPacketWide`: increasing 250 ms retry
backoff capped at one second and recovery from unexpected ACKs. The desktop
server limits attempts to three; this port deliberately permits ten resends for
classic hardware. Explicit Apple II cancellation remains immediate.

Status and serial diagnostics use a small monochrome offscreen bitmap, copied
to the window after text drawing completes, to avoid erase/redraw flicker.
Unchanged text and button titles are not redrawn. This uses original QuickDraw
and does not require Color QuickDraw.

The final-ACK diagnostic build shows each packet's logical block range, encoded
byte count, and expected ACK block. Retries distinguish invalid ACK values/block
numbers, malformed frames, and ACK timeouts. The encoder is compared byte for
byte against packet/CRC code copied from ADTPro 2.1.0's Java server using
`tests/UpstreamWideReference.java` and `tests/upstream_wide_test.c`; fixtures cover
zero-filled, mixed, and trailing-zero data with packet sizes 1, 2, 10, 17 and 40
blocks. A full 143360-byte image always includes all 280 blocks; compression
does not remove logical blocks and no additional padding packet is required.
