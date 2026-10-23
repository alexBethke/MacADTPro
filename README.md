# MacADTPro

MacADTPro is a native classic Macintosh host for transferring Apple II disk
images with the ADTPro 2.1.0 Wide protocol. It is built with Retro68 and targets
68020-or-newer Macs running System 7.0 or later.

The application currently provides:

- modem- and printer-port serial connections at 300, 1200, 2400, 9600, or
  19200 baud;
- standard serial-cable and ImageWriter II cable profiles;
- a browser for disk images stored beside the application;
- ADTPro directory, image-size, and Mac-to-Apple-II image-transfer commands;
- ProDOS-order `.po` images and DOS-order `.dsk`/`.do` images;
- an in-memory serial bootstrap for the bundled ADTPro 2.1.0 Apple II client;
- persistent serial preferences; and
- transfer status, retry details, and serial diagnostics.

Apple-II-to-Mac image uploads are not implemented.

## Build

Configure with the Retro68 68k toolchain and build with CMake:

```sh
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
cmake --build build
```

The build produces:

- `build/MacADTPro.bin`: MacBinary application;
- `build/MacADTPro.dsk`: emulator/transfer disk image; and
- `build/disks/ADTPRO-2.1.0.DSK`: the bundled ADTPro boot-disk image.

`scripts/build.sh` can also regenerate the embedded rainbow PICT and run the
configured build. See `scripts/README.md` for details.

The application requests 768 KB of preferred memory and 512 KB minimum.

## Using MacADTPro

Place supported images in a folder named `disks` beside the application. The
folder is created on startup if it does not exist. Existing files are never
removed or replaced. A file named `disks`, a read-only volume, or another folder
error disables Connect and displays the corresponding Mac OS error.

Open **File > Preferences...** (Command-.) and choose:

- the Modem or Printer port;
- the same transfer speed configured in the Apple II client; and
- whether an ImageWriter II cable is in use.

The default is the Modem port at 19200 baud with the ImageWriter II option off.
Clicking OK saves these settings in the versioned, checksummed `MacADTPro Prefs`
file in the System Preferences folder. Missing, corrupt, or unsupported
preference files fall back to the defaults. Cancel and the Preferences close box
discard changes.

If the Printer port is selected, disable AppleTalk when it owns that port.
Preferences are locked while connected.

Click **Connect**, then choose Receive in the Apple II ADTPro client. Select an
image from the client directory or enter its name and choose the destination
disk. A completed transfer, cancellation, timeout, or error releases the serial
port; reconnect before starting another operation. The same button changes to
Disconnect while a connection is active.

The main window is fixed at 380 by 304 pixels so it fits a 512 by 384 display.
Its image list is scrollable and shows up to 128 HFS catalog entries. The Apple
II client receives directory pages of 18 files with wildcard filtering.
Subdirectories are omitted.

## Image formats

`.po` files are read in ProDOS block order. `.dsk` and `.do` files are converted
from DOS sector order to ProDOS block order while being sent; the source file is
not modified. Extensions describe the byte ordering, not the filesystem inside
the image, so do not rename a DOS-order `.dsk` file to `.po`.

Images are opened read-only. `.po` files must contain complete 512-byte blocks.
DOS-order images must contain complete 4096-byte tracks. A conventional 35-track
floppy image is 143360 bytes, although other complete-track sizes are accepted.

2IMG, nibble images, partial-track DOS images, directory changes, and
Apple-II-to-Mac uploads are not supported.

## Transfer behavior

The transfer engine streams data rather than loading an entire image into
memory. It honors the Apple II client's requested packet size up to 40 blocks,
splits packets at 40-block boundaries, and sends a smaller final packet when
needed. DOS-order conversion is also performed one track at a time.

Wide envelopes, payload XOR checksums, compressed packets, and CRC-CCITT values
are validated or generated with bounded buffers. Completion is reported only
after the final client ACK for the image's full block count.

Malformed or unexpected acknowledgements and checksum failures retry the current
packet up to ten times. Retry delay increases from 250 ms to one second, and a
burst of damaged input consumes only one retry when the packet is actually
resent. Duplicate ACKs are ignored. Ordinary packet ACKs allow 120 seconds;
ACKs that follow a 40-block boundary or the final packet allow five minutes for
physical disk writes. Serial writes allow the estimated wire time plus two
minutes for handshaking delays. Explicit client cancellation stops immediately.

Serial I/O is asynchronous so the event loop remains responsive. Disconnect and
Quit cancel pending output, detach the application-owned receive buffer, close
both serial channels, and close any open image. Cleanup failures are displayed
and can be retried. Status and diagnostics are drawn through monochrome
offscreen bitmaps to reduce flicker on classic QuickDraw systems.

The ImageWriter II profile enables CTS output gating and DTR receive flow control
through Serial Driver control code 14. The host asserts DTR/HSKo on connection.
Actual compatibility still depends on the cable and Apple II serial interface.

## Serial bootstrap

The **Bootstrap** menu loads the bundled ADTPro 2.1.0 client directly into Apple
II memory in three stages: monitor loader, ProDOS MLI, and ADTPro client. The
monitor stage starts at 9600 baud and both computers automatically switch to
19200 baud for the remaining stages. This is independent of the normal transfer
speed preference.

1. Connect the Apple II and Macintosh serial ports. Select the correct Macintosh
   port and cable profile in Preferences.
2. Start the Apple II without loading an operating system and press Control-Reset
   to reach the Applesoft `]` prompt.
3. Type `IN#2` and press Return. Replace `2` with the Super Serial Card slot; the
   IIc and IIgs modem port use virtual slot 2.
4. Press Control-A. At `APPLE SSC:` or the blinking `?`, type `14B`. Press Return
   for a Super Serial Card; doing so is harmless on a IIc or IIgs.
5. Choose **Bootstrap > Load ADTPro 2.1.0 into memory...**, review the instructions,
   and click **Send**.
6. Wait for all three stages to finish. Do not reset the Apple II or change baud
   rates during the sequence.
7. To create a permanent boot disk, format a floppy in ADTPro and receive
   `ADTPRO-2.1.0.DSK` from the Macintosh.

If the Apple II reports a syntax error before loading begins, press Return a few
times and repeat the setup commands. A Franklin ACE 500 instead needs `PR#2`,
Control-A `14B`, `PR#0`, and `IN#2`, pressing Return after each command.

The bootstrap assets come from ADTPro tag `v_2_1_0`, commit
`31227fe4c88494f722f5a342d13ad2ff998e0017`. The application changes the
upstream bootstrap's 115200-baud stages to 19200 because the supported classic
Mac Serial Manager path does not provide 115200 baud.

Unmodified upstream asset SHA-256 checksums:

- `grub2.dmp`: `0c4e4d125ca70811f6c39f974fae262cf91e12c9a2b8ee054fb2382ca025e186`
- `PDSpeed.raw`: `4c8d617c4d0b9d00aa7bbbd3f47e860ccbaa1a0ec0fb77f274598f93f4fac47f`
- `adtpro.raw`: `c676b85df88e5c2115227640123ed7eb54993eded69c667806b5d94e1875ae03`
- `ADTPRO-2.1.0.DSK`: `e63776271c8025033e0dcfb0b137b21e6597810c4886688fb21180e02bd10048`

## Tests

Portable tests cover Wide parsing and encoding, comparison against code derived
from ADTPro 2.1.0, DOS/ProDOS sector ordering, image sizing, transfer state and
retry behavior, and the preferences-file format. Example commands:

```sh
mkdir -p build/tests

cc -std=c99 -Wall -Wextra -Isrc \
  tests/wide_test.c src/wide.c -o build/tests/wide_test
./build/tests/wide_test

cc -std=c99 -Wall -Wextra -Itests/mac_stubs -Isrc \
  tests/transfer_test.c src/transfer.c src/wide.c -o build/tests/transfer_test
./build/tests/transfer_test

cc -std=c99 -Wall -Wextra -Itests/mac_stubs -Isrc \
  tests/preferences_test.c src/preferences_format.c \
  -o build/tests/preferences_test
./build/tests/preferences_test
```

The transfer regression suite includes a complete 280-block image with a
client-requested packet size of ten blocks. Wide encoder fixtures cover zero,
mixed, and trailing-zero data at packet sizes 1, 2, 10, 17, and 40 blocks.

## Current limitations and validation

The implementation and portable regression tests pass, and the application
cross-builds with Retro68. Release validation still requires end-to-end testing
on physical Macintosh and Apple II hardware across the supported speeds, ports,
and cable profiles, including byte-for-byte disk comparisons and the complete
bootstrap sequence.

The following are intentionally outside the current feature set:

- Apple-II-to-Mac uploads and overwrite handling;
- 2IMG, nibble, and other image containers;
- custom image folders and subdirectory navigation;
- arbitrary serial framing or speeds above 19200; and
- file-based diagnostic logging.

## Protocol and licensing

MacADTPro implements the ADTPro 2.x Wide protocol introduced in ADTPro 2.0.0
and targets the pinned 2.1.0 client. Protocol code adapted from ADTPro is
GPL-2.0-or-later; see `LICENSE` and the source-file attribution.

References:

- [ADTPro releases and Wide introduction](https://github.com/ADTPro/adtpro/releases)
- [ADTPro 2.1.0 serial transport](https://github.com/ADTPro/adtpro/blob/v_2_1_0/src/org/adtpro/transport/SerialTransport.java)
- [Retro68](https://github.com/autc04/Retro68)
