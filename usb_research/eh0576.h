#pragma once

#include <stdbool.h>

#include <libusb-1.0/libusb.h>

// Device values
#define VID     0x1c7a
#define PID     0x0576
#define IID     0
#define TIMEOUT 10000

// Image values
#define IMG_WIDTH  70
#define IMG_HEIGHT 57
#define IMG_SIZE   ((IMG_WIDTH) * (IMG_HEIGHT))

// Idk how many EH0576 does, since its a touch sensor it could be less
#define EGIS0576_CONSECUTIVE_CAPTURES 8

// All packets work like this:
// E     G     I     S
// 0x45, 0x47, 0x49, 0x53, [CMD], [REG], [VALUE(s)]
//
// [CMD] = Command
// [REG] = Register
// [VALUE(s)] = Values to write or read
//
// [CMD] seem to be:
// 0x60: Read [REG]
// - [VALUE] is set to 0x00? Not sure, 0575 pkts sometimes had a value but 0x00 seems to work
//
// 0x61: Write [VALUE] into [REG]
// - [VALUE] is the single byte to write
//
// 0x62: Read starting from [REG] (burst read)
// - [VALUE] is the amount of bytes to read
//
// 0x63: Write starting from [REG] (burst read)
// - [VALUE] consists of: [AMOUNT OF BYTES TO WRITE], [BYTES TO WRITE]
// - example: "0x63, 0x01, 0x02, 0x0f, 0x03"
//    -> 0x63 [CMD]; 0x01 [REG]; 0x02 [two bytes]; 0x0f, 0x03 [bytes]
//
// 0x64: Seems to be the command that fetches image data
// - no separate [REG] or [VALUE]
// - it is just the image size (width * size), in our case 70 * 57 = 3990 (0x0f96)
// -> So the command is "0x64, 0x0f, 0x96"
//
// Responses always (except the image response) echo back:
// S     I     G     E
// 0x53, 0x49, 0x47, 0x45, ...[CMD], [REG], [VALUE(s)] ?
typedef struct
{
  int len;
  unsigned char *cmd;
  int res_len;
} Pkt;

int open_egis0576();

void close_egis0576();

int perform_setup();

int init_sequence();

int poll_device_ready();

int repeat_sequence();

int send_egis_pkt(Pkt pkt, unsigned char *resp_buf, int *rcvd);

int send_egis_pkt_raw(unsigned char *seg_buf, const int seg_len, unsigned char *resp_buf,
                      const int resp_len, int *rcvd);

static const Pkt EGIS0576_POLL_PACKET
  = { .len = 7,
      .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x00, 0x00 },
      .res_len = 7 };

static const Pkt EGIS0576_IMAGE_PACKET
  = { .len = 7,
      .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x64, 0x0f, 0x96 },
      .res_len = IMG_SIZE };

#define EGIS0576_INIT_PACKETS_LENGTH 30
static const Pkt EGIS0576_INIT_PACKETS[] = {
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x00, 0x00 },
    .res_len = 7 },  // correct: FUN_180009854
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x01, 0x00 },
    .res_len = 7 },  // correct: FUN_180009854
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x10, 0xfd },
    .res_len = 7 },  // corrected REG from 0x0a to 0x10
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x35, 0x02 },
    .res_len = 7 },  // correct: FUN_18000944c
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x80, 0x00 },
    .res_len = 7 },  // correct: FUN_18000944c
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x80, 0x00 },
    .res_len = 7 },  // correct: FUN_18000944c
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x10, 0xfc },
    .res_len = 7 },  // corrected REG from 0x0a to 0x10
  { .len = 9,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x01, 0x02, 0x0f, 0x03 },
    .res_len = 9 },  // correct: FUN_18000944c
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x0c, 0x22 },
    .res_len = 7 },  // correct: FUN_18000944c
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x09, 0x83 },
    .res_len = 7 },  // correct: FUN_18000944c
  { .len = 13,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x26, 0x06, 0x06, 0x60, 0x06, 0x05,
                              0x2f, 0x06 },
    .res_len = 13 },  // payload unknown: FUN_18000944c
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x10, 0xf4 },
    .res_len = 7 },  // corrected REG from 0x0a to 0x10: FUN_1800095b8
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x0c, 0x44 },
    .res_len = 7 },  // correct: FUN_1800095b8
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x50, 0x03 },
    .res_len = 7 },  // correct: FUN_1800095b8
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x50, 0x00 },
    .res_len = 7 },       // corrected REG_OUT from 0x03 to 0x00: FUN_1800095b8
  EGIS0576_IMAGE_PACKET,  // prepare/clear the buffer? correct: FUN_1800095b8 (via FUN_180009fdc)
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x40, 0x00 },
    .res_len = 7 },  // corrected REG_OUT from 0xec to 0x00: FUN_180009dfc
  { .len = 18,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x09, 0x0b, 0x83, 0x24, 0x00, 0x44,
                              0x0f, 0x08, 0x20, 0x20, 0x00, 0x00, 0x52 },
    .res_len = 18 },  // payload unknown: FUN_180009dfc
  { .len = 13,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x26, 0x06, 0x06, 0x60, 0x06, 0x05,
                              0x2f, 0x06 },
    .res_len = 13 },  // payload unknown: FUN_180009dfc
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x23, 0x00 },
    .res_len = 7 },  // correct: FUN_180009dfc
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x24, 0x38 },
    .res_len = 7 },  // corrected REG_VALUE from 0x33 to 0x38: FUN_180009dfc
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x20, 0x00 },
    .res_len = 7 },  // correct: FUN_180009dfc
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x21, 0x45 },
    .res_len = 7 },  // corrected REG_VALUE from 0x66 to 0x45: FUN_180009dfc
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x00, 0x00 }, .res_len = 7 },
  { .len = 7, .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x01, 0x00 }, .res_len = 7 },
  { .len = 9,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x2c, 0x02, 0x00, 0x57 },
    .res_len = 9 },  // correct: FUN_180008350
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x2d, 0x00 },
    .res_len = 7 },  // corrected REG_OUT from 0x02 to 0x00: FUN_180008350
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x62, 0x67, 0x03 },
    .res_len = 10 },  // correct: FUN_180008350
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x0f, 0x00 },
    .res_len = 7 },  // corrected REG_OUT from 0x03 to 0x00: FUN_180008f90
  { .len = 9,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x2c, 0x02, 0x00, 0x13 },
    .res_len = 9 },  // correct: FUN_180009230
};

#define EGIS0576_REPEAT_PACKETS_LENGTH 5
static const Pkt EGIS0576_REPEAT_PACKETS[] = {
  { .len = 9,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x2c, 0x02, 0x00, 0x57 },
    .res_len = 9 },  // correct: FUN_180008350
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x2d, 0x00 },
    .res_len = 7 },  // corrected REG_OUT from 0x02 to 0x00: FUN_180008350
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x62, 0x67, 0x03 },
    .res_len = 10 },  // correct: FUN_180008350
  { .len = 7,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x0f, 0x00 },
    .res_len = 7 },  // corrected REG_OUT from 0x03 to 0x00: FUN_180008f90
  { .len = 9,
    .cmd = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x2c, 0x02, 0x00, 0x13 },
    .res_len = 9 },  // correct: FUN_180009230
};
