#include <stdio.h>
#include <unistd.h>

#include <libusb-1.0/libusb.h>

#include "eh0576.h"

int rcvd = 0;
unsigned char resp[8192] = { 0 };
unsigned char img[8192] = { 0 };

int pre_init_sequence()
{
  // printf("Sending PRE_INIT sequence.\n");
  for (int i = 0; i < EGIS0576_PRE_INIT_PACKETS_LENGTH; i++)
  {
    Pkt pkt = EGIS0576_PRE_INIT_PACKETS[i];
    if (send_egis_pkt(pkt.payload, pkt.len, resp, pkt.res_len, &rcvd) != LIBUSB_SUCCESS)
    {
      printf("PRE_INIT failed at pkt index: %d.\n", i);
      return -1;
    }
  }

  return post_init_sequence(true);
}

int post_init_sequence(bool from_pre_init)
{
  // printf("Sending POST_INIT sequence.\n");
  for (int i = 0; i < EGIS0576_POST_INIT_PACKETS_LENGTH; i++)
  {
    Pkt pkt = EGIS0576_POST_INIT_PACKETS[i];
    if (send_egis_pkt(pkt.payload, pkt.len, resp, pkt.res_len, &rcvd) != LIBUSB_SUCCESS)
    {
      printf("POST_INIT failed at pkt index: %d.\n", i);
      return -1;
    }

    if (i == 1 && resp[5] == 0x01)
    {
      printf("WARNING: PRE_INIT sequence is required.\n");

      if (from_pre_init)
      {
        printf("Cannot call PRE_INIT sequence because it was called already.\n");
        return -1;
      }

      return pre_init_sequence();
    }
  }

  return LIBUSB_SUCCESS;
}

int repeat_sequence()
{
  // printf("Sending REPEAT sequence.\n");
  for (int i = 0; i < EGIS0576_REPEAT_PACKETS_LENGTH; i++)
  {
    Pkt pkt = EGIS0576_REPEAT_PACKETS[i];
    if (send_egis_pkt(pkt.payload, pkt.len, resp, pkt.res_len, &rcvd) != LIBUSB_SUCCESS)
    {
      printf("REPEAT failed at pkt index: %d.\n", i);
      return -1;
    }
  }

  return LIBUSB_SUCCESS;
}

int save_image()
{
  // printf("Sending IMAGE request.\n");

  unsigned char img_cmd[] = { 0x45, 0x47, 0x49, 0x53, 0x64, 0x0f, 0x96 };
  if (send_egis_pkt_t(img_cmd, 7, img, 8192, &rcvd, 190000) != LIBUSB_SUCCESS)
  {
    printf("Request to save image failed.\n");
    return -1;
  }

  return LIBUSB_SUCCESS;
}

int verify_image()
{
  uint total = 0;
  for (int i = 0; i < rcvd; i++)
  {
    total += (uint)img[i];
  }

  if (total != 0)
  {
    printf("We have got the image! (%d)\n", total);
    return LIBUSB_SUCCESS;
  }

  return -1;
}

int main()
{
  if (open_and_handshake() != LIBUSB_SUCCESS)
  {
    return -1;
  }

  if (pre_init_sequence() != LIBUSB_SUCCESS)
  {
    dispose();
    return -1;
  }

  usleep(190000);

  int verified_images = 0;
  for (int i = 0; i < EGIS0575_CONSECUTIVE_CAPTURES; i++)
  {
    if (i != 0 && repeat_sequence() != LIBUSB_SUCCESS)
    {
      dispose();
      return -1;
    }

    if (save_image() != LIBUSB_SUCCESS)
    {
      dispose();
      return -1;
    }

    if (verify_image() == LIBUSB_SUCCESS)
    {
      verified_images++;
    }
  }

  if (verified_images == 0)
  {
    printf("Could not verify any images.\n");
    return -1;
  }

  printf("Exiting.\n");
  dispose();
  return 1;
}
