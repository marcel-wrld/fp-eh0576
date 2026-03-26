#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <libusb-1.0/libusb.h>

#include "eh0576.h"

#define TEST_FRAMES 4

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
  uint sum = 0;
  uint count_dark = 0;
  for (int i = 0; i < img_len; i++)
  {
    uint b = (uint)img[i];
    sum += b;
    if (b < IMGP_DARK)
      count_dark++;
  }

  // No data was present
  if (sum == 0)
    return -1;

  int mean = sum / img_len;
  double sd = 0;
  for (int i = 0; i < img_len; i++)
  {
    int diff = (uint)img[i] - mean;
    sd += diff * diff;
  }

  double sd_dev = sqrt(sd / img_len);
  double dark_portion = (double)count_dark / img_len;
  bool finger_present = sd_dev > IMGP_SD_DEV && dark_portion > IMGP_DARK_PORTION;

  printf(">> IMAGE: %d\n", finger_present);
  return LIBUSB_SUCCESS;
}

int main()
{
  if ((__status = open_egis0576()) != LIBUSB_SUCCESS)
    goto CLEANUP;

  if ((__status = init_sequence()) != LIBUSB_SUCCESS)
    goto CLEANUP;

  int verified_images = 0;
  for (int i = 0; i < TEST_FRAMES; i++)
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
