/*
 * Egis Technology Inc. (aka. LighTuning) 0576 driver for libfprint
 * Copyright (C) 2026 Marcel (Sprayxe) <sprayxe.marcel@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/* Device config */
#define EGIS0576_INTF 0

/* Device endpoints */
#define EGIS0576_EPOUT 0x01
#define EGIS0576_EPIN 0x82

/* Device transfers */
#define EGIS0576_TIMEOUT 10000
#define EGIS0576_POLL_COUNT 3000

/* Sensor image */
#define EGIS0576_IMG_WIDTH 70
#define EGIS0576_IMG_HEIGHT 57
#define EGIS0576_IMG_SIZE ((EGIS0576_IMG_WIDTH) *(EGIS0576_IMG_HEIGHT))

/* Upscaled sensor image */
#define EGIS0576_IMG_UPSCALE 2
#define EGIS0576_IMG_WIDTH_UPSCALE ((EGIS0576_IMG_WIDTH) *(EGIS0576_IMG_UPSCALE))
#define EGIS0576_IMG_HEIGHT_UPSCALE ((EGIS0576_IMG_HEIGHT) *(EGIS0576_IMG_UPSCALE))
#define EGIS0576_IMG_SIZE_UPSCALE ((EGIS0576_IMG_WIDTH_UPSCALE) *(EGIS0576_IMG_HEIGHT_UPSCALE))

/* Canvas image */
#define EGIS0576_CANVAS_WIDTH 256
#define EGIS0576_CANVAS_HEIGHT 256
#define EGIS0576_CANVAS_SIZE ((EGIS0576_CANVAS_WIDTH) *(EGIS0576_CANVAS_HEIGHT))

/*
 * These values were acquired by testing with finger present and without finger
 * present.
 */
#define EGIS0576_CONTRAST 4
#define EGIS0576_VARIANCE (3.2 * 3.2)
#define EGIS0576_DARK_PORTION 0.05
#define EGIS0576_BG_VARIANCE (2.5 * 2.5)

/*
 *
 * All packets work like this:
 * E     G     I     S
 * 0x45, 0x47, 0x49, 0x53, [CMD], [REG], [VALUE(s)]
 *
 * [CMD] = Command
 * [REG] = Register
 * [VALUE(s)] = Values to write or read
 * [CMD] seem to be:
 * 0x60: Read [REG]
 * - [VALUE] is set to 0x00? Not sure, 0575 pkts sometimes had a value but 0x00
 * seems to work
 *
 * 0x61: Write [VALUE] into [REG]
 * - [VALUE] is the single byte to write
 *
 * 0x62: Read starting from [REG] (burst read)
 * - [VALUE] is the amount of bytes to read
 *
 * 0x63: Write starting from [REG] (burst read)
 * - [VALUE] consists of: [AMOUNT OF BYTES TO WRITE], [BYTES TO WRITE]
 * - example: "0x63, 0x01, 0x02, 0x0f, 0x03"
 *   -> 0x63 [CMD]; 0x01 [REG]; 0x02 [two bytes]; 0x0f, 0x03 [bytes]
 *
 * 0x64: Seems to be the command that fetches image data
 * - no separate [REG] or [VALUE]
 * - it is just the image size (width * size), in our case 70 * 57 = 3990
 * (0x0f96)
 *   -> So the command is "0x64, 0x0f, 0x96"
 *
 * Responses always (except the image response) echo back:
 * S     I     G     E
 * 0x53, 0x49, 0x47, 0x45, ...[CMD], [REG], [VALUE(s)] ?
 */
typedef struct
{
  int            len;
  unsigned char *cmd;
  int            res_len;
} Egis0576Pkt;

/*
 * Huge credit goes to Animeshz's efforts on github
 * (https://github.com/Animeshz/EgisTec-EH575)
 * and on libfprint
 * (https://gitlab.freedesktop.org/libfprint/libfprint/-/merge_requests/317) as
 * those laid the base for this effort.
 *
 * Initial tests proved that the EH0575 and the EH0576 seem to work basically
 * the same as by just providing the EH0575 packets I was able to occasionally
 * get little data out of mine.
 *
 * I was unable to get the fingerprint reader to work in a VM (both Win10 and
 * Win11) and Wireshark USBcap does not allow to be run on Windows-To-Go so I
 * could not use any USB captures from Windows.
 *
 * Therefore, the rest of the reverse engineering was solely done with static
 * analysis in Ghidra of the EgisTouchFP0576.dll UMDF driver provided
 * by Lenovo
 * (https://pcsupport.lenovo.com/de/en/products/laptops-and-netbooks/flex-series/flex-5-14alc7).
 * I am no professional "hacker" by any means so I cannot guarantee that these
 * are 100% correct but they work :D
 */

/*
 * According to static analysis the driver polls using this packet a max. amount
 * of EGIS0576_POLL_COUNT before requesting the image.
 * Translated to this driver it checks the following condition in order to
 * proceed to requesting the image:
 *
 * (response_buffer[6] & 0x01) == 0x01
 *
 * Based on observation this has been always true for already the first request
 * so it essentially runs once.
 */
static const Egis0576Pkt EGIS0576_POLL_PACKET
  = { .len = 7,
      .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x00, 0x00 },
      .res_len = 9 };

/* Fetches the image data. */
static const Egis0576Pkt EGIS0576_IMAGE_PACKET
  = { .len = 7,
      .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x64, 0x0f, 0x96 },
      .res_len = EGIS0576_IMG_SIZE };

/* Initialization packets. */
#define EGIS0576_INIT_PACKETS_LENGTH 30
static const Egis0576Pkt EGIS0576_INIT_PACKETS[] = {
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x00, 0x00 }, .res_len = 7 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x01, 0x00 }, .res_len = 7 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x10, 0xfd }, .res_len = 7 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x35, 0x02 }, .res_len = 7 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x80, 0x00 }, .res_len = 7 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x80, 0x00 }, .res_len = 7 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x10, 0xfc }, .res_len = 7 },
  { .len = 9,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x01, 0x02, 0x0f, 0x03 },
    .res_len = 9 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x0c, 0x22 }, .res_len = 7 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x09, 0x83 }, .res_len = 7 },
  { .len = 13,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x26, 0x06, 0x06, 0x60, 0x06, 0x05,
                              0x2f, 0x06 },
    .res_len = 13 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x10, 0xf4 }, .res_len = 7 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x0c, 0x44 }, .res_len = 7 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x50, 0x03 }, .res_len = 7 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x50, 0x00 }, .res_len = 7 },
  EGIS0576_IMAGE_PACKET,
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x40, 0x00 }, .res_len = 7 },
  { .len = 18,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x09, 0x0b, 0x83, 0x24, 0x00, 0x44,
                              0x0f, 0x08, 0x20, 0x20, 0x00, 0x00, 0x52 },
    .res_len = 18 },
  { .len = 13,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x26, 0x06, 0x06, 0x60, 0x06, 0x05,
                              0x2f, 0x06 },
    .res_len = 13 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x23, 0x00 }, .res_len = 7 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x24, 0x38 }, .res_len = 7 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x20, 0x00 }, .res_len = 7 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x21, 0x45 }, .res_len = 7 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x00, 0x00 }, .res_len = 7 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x01, 0x00 }, .res_len = 7 },
  { .len = 9,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x2c, 0x02, 0x00, 0x57 },
    .res_len = 9 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x2d, 0x00 }, .res_len = 7 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x62, 0x67, 0x03 }, .res_len = 10 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x0f, 0x00 }, .res_len = 7 },
  { .len = 9,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x2c, 0x02, 0x00, 0x13 },
    .res_len = 9 }
};

/* Repeat packets. */
#define EGIS0576_REPEAT_PACKETS_LENGTH 5
static const Egis0576Pkt EGIS0576_REPEAT_PACKETS[] = {
  { .len = 9,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x2c, 0x02, 0x00, 0x57 },
    .res_len = 9 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x2d, 0x00 }, .res_len = 7 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x62, 0x67, 0x03 }, .res_len = 10 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x0f, 0x00 }, .res_len = 7 },
  { .len = 9,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x2c, 0x02, 0x00, 0x13 },
    .res_len = 9 }
};
