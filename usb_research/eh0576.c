#include <stdio.h>
#include <unistd.h>

#include <libusb-1.0/libusb.h>

#include "eh0576.h"

libusb_context *ctx = NULL;
libusb_device_handle *h = NULL;
bool claimed_interface = false;

int rcvd = 0;
unsigned char resp_buf[8192] = { 0 };

int open_egis0576()
{
  libusb_init(&ctx);

  // Open device
  h = libusb_open_device_with_vid_pid(ctx, VID, PID);
  if (!h)
  {
    printf("Could not open device.\n");
    return -1;
  }

  if (libusb_claim_interface(h, IID) != LIBUSB_SUCCESS)
  {
    printf("Could not claim interface.\n");
    return -1;
  }

  claimed_interface = true;
  return LIBUSB_SUCCESS;
}

void close_egis0576()
{
  if (claimed_interface)
  {
    libusb_release_interface(h, IID);
  }

  if (h)
  {
    libusb_close(h);
  }

  if (ctx)
  {
    libusb_exit(ctx);
  }
}

int init_sequence()
{
  // printf("Sending INIT sequence.\n");
  for (int i = 0; i < EGIS0576_INIT_PACKETS_LENGTH; i++)
  {
    Pkt pkt = EGIS0576_INIT_PACKETS[i];
    if (send_egis_pkt(pkt, resp_buf, &rcvd) != LIBUSB_SUCCESS)
    {
      printf("INIT failed at pkt index: %d.\n", i);
      return -1;
    }
  }

  return LIBUSB_SUCCESS;
}

int poll_device_ready()
{
  Pkt pkt = EGIS0576_POLL_PACKET;
  for (int i = 0; i < POLL_COUNT; i++)
  {
    if (send_egis_pkt(pkt, resp_buf, &rcvd) != LIBUSB_SUCCESS)
    {
      printf("POLL failed at index: %d.\n", i);
      return -1;
    }

    if ((resp_buf[6] & 0x01) == 0x01)
    {
      // printf("Data poll completed!\n");
      return LIBUSB_SUCCESS;
    }
  }

  return -1;
}

int repeat_sequence()
{
  // printf("Sending REPEAT sequence.\n");
  for (int i = 0; i < EGIS0576_REPEAT_PACKETS_LENGTH; i++)
  {
    Pkt pkt = EGIS0576_REPEAT_PACKETS[i];
    if (send_egis_pkt(pkt, resp_buf, &rcvd) != LIBUSB_SUCCESS)
    {
      printf("REPEAT failed at pkt index: %d.\n", i);
      return -1;
    }
  }

  return LIBUSB_SUCCESS;
}

int send_egis_pkt(Pkt pkt, unsigned char *resp_buf, int *rcvd)
{
  return send_egis_pkt_raw(pkt.cmd, pkt.len, resp_buf, pkt.res_len, rcvd);
}

int send_egis_pkt_raw(unsigned char *seg_buf, const int seg_len, unsigned char *resp_buf,
                      const int resp_len, int *rcvd)
{
  int sent = 0;
  *rcvd = 0;

  int r = libusb_bulk_transfer(h, 0x01, seg_buf, seg_len, &sent, TIMEOUT);
  if (r != LIBUSB_SUCCESS)
  {
    printf("Could not sent data: %s\n", libusb_error_name(r));
    return -1;
  }

  r = libusb_bulk_transfer(h, 0x82, resp_buf, resp_len, rcvd, TIMEOUT);
  if (r != LIBUSB_SUCCESS)
  {
    printf("Could not receive data: %s\n", libusb_error_name(r));
    return -1;
  }

  return LIBUSB_SUCCESS;
}
