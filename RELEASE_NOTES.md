# MacADTPro 0.1

MacADTPro 0.1 is the initial release of a native classic Macintosh host for
transferring Apple II disk images with the ADTPro 2.1.0 Wide protocol.

## Highlights

- Send disk images from a Macintosh to an Apple II and receive images from an
  Apple II onto the Macintosh.
- Browse disk images stored in a `disks` folder beside the application.
- Transfer ProDOS-order `.po` images and DOS-order `.dsk` and `.do` images.
- Use the modem or printer serial port at 300, 1200, 2400, 9600, or 19200 baud.
- Select standard serial wiring or an ImageWriter II cable profile with
  hardware handshaking.
- Bootstrap the bundled ADTPro 2.1.0 Apple II client directly into memory in
  three stages, without first preparing a boot disk.
- Save serial settings in the classic Mac OS System Preferences folder.
- Monitor transfer progress, retries, timeouts, and optional serial diagnostics.
- Continue with another operation after a successful transfer without manually
  reconnecting the serial port.

## Reliability and compatibility

Transfers use bounded streaming buffers rather than loading an entire image
into memory. MacADTPro validates Wide envelopes, checksums, packet CRCs, block
numbers, and the Apple II's final block acknowledgement. Failed packets are
retried with bounded backoff, while cancellations, timeouts, and I/O failures
clean up open files and serial resources.

Uploads are written to a temporary file and renamed only after successful
completion. Existing images are never overwritten, and incomplete uploads are
removed.

MacADTPro requires:

- a Macintosh with a 68020 or newer processor;
- System 7.0 or later;
- at least 512 KB of available application memory; and
- a compatible serial connection to the Apple II.

The application window fits a 512-by-384 display. The release is distributed
as a locked HFS disk image.

## Known limitations

- End-to-end validation on physical Macintosh and Apple II hardware is still
  recommended for each serial port, cable profile, and transfer speed.
- 2IMG, nibble images, partial-track DOS images, custom image folders, and
  subdirectory navigation are not supported.
- Uploads will not overwrite an existing destination image.
- File-based diagnostic logging and arbitrary serial framing are not included.

## Support development

MacADTPro is free software distributed under the GPL-2.0-or-later license. If
you would like to support its continued development, voluntary contributions
are welcome via [PayPal](https://paypal.me/Bethke918).

Thank you for trying the first release of MacADTPro. Apple ][ forever.
