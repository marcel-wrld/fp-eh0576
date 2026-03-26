#pragma once

#include <stdbool.h>

#include <libusb-1.0/libusb.h>

#define VID                           0x1c7a
#define PID                           0x0576
#define IID                           0
#define TIMEOUT                       10000
#define IMG_WIDTH                     70
#define IMG_HEIGHT                    57
#define IMG_SIZE                      ((IMG_WIDTH) * (IMG_HEIGHT))
#define EGIS0576_CONSECUTIVE_CAPTURES 8

typedef struct
{
  int len;
  unsigned char *payload;
  int res_len;
} Pkt;

int open_egis0576();

void close_egis0576();

int perform_setup();

int pre_init_sequence();

int post_init_sequence(bool from_pre_init);

int poll_device_ready();

int repeat_sequence();

int send_egis_pkt(Pkt pkt, unsigned char *resp_buf, int *rcvd);

int send_egis_pkt_raw(unsigned char *seg_buf, const int seg_len, unsigned char *resp_buf,
                      const int resp_len, int *rcvd);

void print_data(unsigned char *data, int size);

#define EGIS0576_PRE_INIT_PACKETS_LENGTH 29
static const Pkt EGIS0576_PRE_INIT_PACKETS[] = {
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x00, 0x00 },
    .res_len = 7 },  // correct: FUN_180009854
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x01, 0x00 },
    .res_len = 7 },  // correct: FUN_180009854
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x10, 0xfd },
    .res_len = 7 },  // corrected REG from 0x0a to 0x10
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x35, 0x02 },
    .res_len = 7 },  // correct: FUN_18000944c
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x80, 0x00 },
    .res_len = 7 },  // correct: FUN_18000944c
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x80, 0x00 },
    .res_len = 7 },  // correct: FUN_18000944c
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x10, 0xfc },
    .res_len = 7 },  // corrected REG from 0x0a to 0x10
  { .len = 9,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x01, 0x02, 0x0f, 0x03 },
    .res_len = 9 },  // correct: FUN_18000944c
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x0c, 0x22 },
    .res_len = 7 },  // correct: FUN_18000944c
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x09, 0x83 },
    .res_len = 7 },  // correct: FUN_18000944c
  { .len = 13,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x26, 0x06, 0x06, 0x60, 0x06, 0x05,
                                  0x2f, 0x06 },
    .res_len = 13 },  // payload unknown: FUN_18000944c
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x10, 0xf4 },
    .res_len = 7 },  // corrected REG from 0x0a to 0x10: FUN_1800095b8
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x0c, 0x44 },
    .res_len = 7 },  // correct: FUN_1800095b8
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x50, 0x03 },
    .res_len = 7 },  // correct: FUN_1800095b8
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x50, 0x00 },
    .res_len = 7 },  // corrected REG_OUT from 0x03 to 0x00: FUN_1800095b8
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x64, 0x0f, 0x96 },
    .res_len = IMG_SIZE },  // correct: FUN_1800095b8 (values via FUN_180009fdc)
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x40, 0x00 },
    .res_len = 7 },  // corrected REG_OUT from 0xec to 0x00: FUN_180009dfc
  { .len = 18,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x09, 0x0b, 0x83, 0x24, 0x00, 0x44,
                                  0x0f, 0x08, 0x20, 0x20, 0x00, 0x00, 0x52 },
    .res_len = 18 },  // payload unknown: FUN_180009dfc
  { .len = 13,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x26, 0x06, 0x06, 0x60, 0x06, 0x05,
                                  0x2f, 0x06 },
    .res_len = 13 },  // payload unknown: FUN_180009dfc
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x23, 0x00 },
    .res_len = 7 },  // correct: FUN_180009dfc
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x24, 0x38 },
    .res_len = 7 },  // corrected REG_VALUE from 0x33 to 0x38: FUN_180009dfc
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x20, 0x00 },
    .res_len = 7 },  // correct: FUN_180009dfc
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x21, 0x45 },
    .res_len = 7 },  // corrected REG_VALUE from 0x66 to 0x45: FUN_180009dfc
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x00, 0x00 },
    .res_len = 7 },  // correct: FUN_1800086f4
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x01, 0x00 },
    .res_len = 7 },  // correct: FUN_1800086f4
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x40, 0x00 },
    .res_len = 7 },  // correct: FUN_180009cd0
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x0c, 0x22 },
    .res_len = 7 },  // correct: FUN_180009cd0
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x0b, 0x03 },
    .res_len = 7 },  // correct: FUN_180009cd0
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x10, 0xfc },
    .res_len = 7 },  // corrected REG from 0x0a to 0x10: FUN_180009cd0
};

#define EGIS0576_POST_INIT_PACKETS_LENGTH 16
static const Pkt EGIS0576_POST_INIT_PACKETS[] = {
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x00, 0x00 },
    .res_len = 7 },
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x01, 0x00 },
    .res_len = 7 }, /* Change to EGIS0576_PRE_INIT_PACKETS if response == 01:01:01 */
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x40, 0x00 },
    .res_len = 7 },
  { .len = 18,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x09, 0x0b, 0x83, 0x24, 0x00, 0x44,
                                  0x0f, 0x08, 0x20, 0x20, 0x00, 0x00, 0x52 },
    .res_len = 18 },  // payload unknown: FUN_180009dfc
  { .len = 13,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x26, 0x06, 0x06, 0x60, 0x06, 0x05,
                                  0x2f, 0x06 },
    .res_len = 13 },  // payload unknown: FUN_180009dfc
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x23, 0x00 },
    .res_len = 7 },  // correct: FUN_180009dfc
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x24, 0x38 },
    .res_len = 7 },  // corrected REG_VALUE from 0x33 to 0x38: FUN_180009dfc
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x20, 0x00 },
    .res_len = 7 },  // correct: FUN_180009dfc
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x21, 0x45 },
    .res_len = 7 },  // corrected REG_VALUE from 0x66 to 0x45: FUN_180009dfc
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x00, 0x00 },
    .res_len = 7 },
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x01, 0x00 },
    .res_len = 7 },
  { .len = 9,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x2c, 0x02, 0x00, 0x57 },
    .res_len = 9 },  // correct: FUN_180008350
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x2d, 0x00 },
    .res_len = 7 },  // corrected REG_OUT from 0x02 to 0x00: FUN_180008350
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x62, 0x67, 0x03 },
    .res_len = 10 },  // correct: FUN_180008350
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x0f, 0x00 },
    .res_len = 7 },  // corrected REG_OUT from 0x03 to 0x00: FUN_180008f90
  { .len = 9,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x2c, 0x02, 0x00, 0x13 },
    .res_len = 9 },  // correct: FUN_180009230
};

#define EGIS0576_REPEAT_PACKETS_LENGTH 8
static const Pkt EGIS0576_REPEAT_PACKETS[] = {
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x61, 0x2d, 0x20 },
    .res_len = 7 },
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x00, 0x00 },
    .res_len = 7 },
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x01, 0x00 },
    .res_len = 7 },
  { .len = 9,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x2c, 0x02, 0x00, 0x57 },
    .res_len = 9 },  // correct: FUN_180008350
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x2d, 0x00 },
    .res_len = 7 },  // corrected REG_OUT from 0x02 to 0x00: FUN_180008350
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x62, 0x67, 0x03 },
    .res_len = 10 },  // correct: FUN_180008350
  { .len = 7,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x0f, 0x00 },
    .res_len = 7 },  // corrected REG_OUT from 0x03 to 0x00: FUN_180008f90
  { .len = 9,
    .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x63, 0x2c, 0x02, 0x00, 0x13 },
    .res_len = 9 },  // correct: FUN_180009230
};

static const Pkt EGIS0576_POLL_PACKET
  = { .len = 7,
      .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x60, 0x00, 0x00 },
      .res_len = 7 };

static const Pkt EGIS0576_REQUEST_IMAGE_PACKET
  = { .len = 7,
      .payload = (unsigned char[]){ 0x45, 0x47, 0x49, 0x53, 0x64, 0x0f, 0x96 },
      .res_len = 3990 };
