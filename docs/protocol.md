# ADTPro 2.1.0 protocol notes

Reference revision: `31227fe4c88494f722f5a342d13ad2ff998e0017`, tag `v_2_1_0`.
The source was inspected locally; tests exercise the port's independent parser.
This is a partial protocol implementation, not an interoperability certification.

`CommsThread.pullEnvelopeWide` reads `C1 length-low length-high command XOR`.
The header XOR covers C1 and the next three bytes. Length is little endian.
`pullPayloadWide` reads length bytes and their XOR byte for nonempty payloads.
An empty payload has no payload checksum byte. Payloads must be consumed before
interpreting the next command; bytes C1 or D9 within a payload have no special
meaning. The parser retains up to 256 payload bytes while validating the full XOR.

`sendHome` emits `C1 00 00 D8 19`. This build sends it for valid requests except
Go Home itself, supported directory/image requests, and their acknowledgements.
A standalone D9 ping is recognized without sending a response,
matching the upstream serial command loop. Invalid checksums are reported and
incomplete packets reset after five seconds without another received byte.
Directory packets now use sendPacketWide differential RLE and CRC-CCITT
(polynomial 1021, initial value zero). A directory page is 1024 bytes with up to
18 filenames. Header block number follows upstream's requested-page value.
CB ACK payloads contain ACK/NAK and the next block number; NAK or invalid ACK
retries are limited to ten. Raw `.po` and DOS-order `.dsk`/`.do` transfers use
the same packet encoder. DOS-order files must contain complete 4096-byte tracks and are read through
the sector mapping in the AppleCommander 1.3.5.13 library bundled with ADTPro 2.1.0.
The first sectors of blocks within a track are 0,13,11,9,7,5,3,1; the second
sectors are 14,12,10,8,6,4,2,15. A 4096-byte track cache bounds memory use.
DA replies contain block-count LE16 plus status. C7 replies with ready status
zero, waits for a CB ACK for block zero, then sends chunks respecting the client's
preferred size (up to 40 blocks) and 40-block buffer boundaries. Every ACK must name the next
block. NAK retries are limited to ten. Files are read in bounded chunks.

During active image transfers, malformed headers/payloads schedule a bounded
retry of the current packet with a progressive 250 ms to one-second pause; they do not disconnect on the
first error. All image ACK deadlines are 120 seconds. Completion, client
cancel and acknowledgement timeout also disconnect. Transfer state becomes idle
before closing the image file, even if the close operation fails. Serial output
cancellation only calls KillIO for a pending asynchronous write and verifies
that its parameter block is no longer pending before marking it cancelled.

Command coverage:

| Byte | Operation |
| --- | --- |
| C3 | Change directory |
| C4 | Directory listing (implemented) |
| C7 | Apple II receives an image (.po and standard DOS-order .dsk/.do implemented) |
| D0 | Apple II sends an image |
| DA | Image size query (.po and standard DOS-order .dsk/.do implemented) |
| CB | Acknowledgement (directory and image handling implemented) |
| D8 | Go Home (implemented) |

SerialConfig.java stores the ImageWriter checkbox as `HardwareHandshaking=true`.
SerialTransport.java enables RTS/CTS flow control for that flag. Classic Mac
ports use their own handshake wiring; the current adapter enables CTS output
gating and DTR receive flow control using driver control code 14 on the output
driver. Software handshaking is disabled. Internal clocking is selected explicitly.
Physical compatibility remains unverified. Reference:
[Apple Inside Macintosh, Serial Driver control routines](https://dev.os9.ca/techpubs/mac/Devices/Devices-325.html).

The Apple II increments BLKLO by its configured BAOCNT in sr.asm, not the
received envelope length. The host therefore must not impose a smaller packet
size. Final ACK is emitted by sr.asm after writing the final buffer. Its
deadline is 120 seconds; ordinary image ACK deadlines are also 120 seconds.

Resilience: permit ten actual resends per image packet. Coalesce damaged frames
while a resend is already scheduled, ignore stale/duplicate successful ACKs, and
reset incomplete parser input when queuing a packet. Unexpected commands during
an image transfer schedule recovery rather than immediately closing the image.
Explicit HOME or CAN still cancels immediately, as in the upstream server.

Image block counts are derived from data-fork length divided by 512, not fixed
at 280. DA advertises that count; C7 sends only those blocks, including a short
final packet. Completion requires an ACK for the actual image block count.

Empty-block verification: the server uses the same D3 data packet for empty and
nonempty blocks. For N all-zero 512-byte blocks the encoded packet occupies
9 + 4*N bytes: 7 header/block bytes, two RLE bytes (00 00) per 256-byte page,
and two zero CRC bytes. The declared uncompressed length remains N*512.
There is no separate padding packet in sendPacketWide or RECVWIDE.
`tests/transfer_test.c` models RECVWIDE's differential writes/run skips followed
by UNDIFFWide and independent CRC verification. A 280-block image with its final
130 blocks zero is reconstructed correctly for BAOCNT 1 through 40. Actual framed
client ACKs are parsed for each packet; final ACK 280 completes the transfer
without an additional packet. These host tests do not verify serial timing on
physical hardware.

Successful image ACKs reset both the packet retry count and its stored reason
immediately, including ACKs arriving during resend backoff. Status text reports
zero after recovery. Delayed ACK/NAK frames for blocks preceding the current
packet are ignored. Recovery tests retry every packet eight times, then accept
a successful ACK during the ninth backoff across a complete 280-block image.

Disk-write wait policy: ACKs at 40-block boundaries and at image end follow
WRITING on the Apple II. Silence there permits five minutes for disk activity,
shows the expected ACK, and never automatically resends a data packet while the
client could still be writing. Expiry stops with a specific disk-write ACK timeout;
it cannot determine whether the cause was drive/media, a lost ACK, or a stalled
client. Explicit NAKs still use packet retries, with reset on successful ACK.
Tests simulate delayed boundary and final ACKs, and verify no resend on silence.
