#include <stdio.h>
#include <unistd.h>

#include <libusb-1.0/libusb.h>

#include "eh0576.h"

int img_len = 0;
unsigned char img[8192] = { 0 };

int __status;

int save_image()
{
  // printf("Sending IMAGE request.\n");

  if (poll_device_ready() != LIBUSB_SUCCESS)
  {
    return -1;
  }

  Pkt pkt = EGIS0576_IMAGE_PACKET;
  if (send_egis_pkt(pkt, img, &img_len) != LIBUSB_SUCCESS)
  {
    printf("Request to save image failed.\n");
    return -1;
  }

  return LIBUSB_SUCCESS;
}

int verify_image()
{
  uint total = 0;
  for (int i = 0; i < img_len; i++)
  {
    total += (uint)img[i];
  }

  if (total != 0)
  {
    printf(">> IMAGE: %d\n", total);
    return LIBUSB_SUCCESS;
  }

  return -1;
}

int main()
{
  if ((__status = open_egis0576()) != LIBUSB_SUCCESS)
    goto CLEANUP;

  if ((__status = init_sequence()) != LIBUSB_SUCCESS)
    goto CLEANUP;

  int verified_images = 0;
  for (int i = 0; i < EGIS0576_CONSECUTIVE_CAPTURES; i++)
  {
    if (i != 0 && (__status = repeat_sequence()) != LIBUSB_SUCCESS)
      goto CLEANUP;

    if ((__status = save_image()) != LIBUSB_SUCCESS)
      goto CLEANUP;

    if (verify_image() == LIBUSB_SUCCESS)
    {
      verified_images++;
    }
  }

  if (verified_images == 0)
  {
    printf("Could not verify any images.\n");
    __status = -1;
  }

CLEANUP:
  if (__status != LIBUSB_SUCCESS)
  {
    __status = -1;
  }
  else
  {
    printf("Exiting.\n");
    __status = 1;
  }

  close_egis0576();
  return __status;
}
