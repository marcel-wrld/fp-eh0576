#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libusb-1.0/libusb.h>

#include "eh0576.h"

bool have_bg = false;
unsigned char bg[8192] = { 0 };
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
  int img_len;
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
  for (int i = 0; i < IMG_SIZE; i++)
  {
    sum += (uint)img[i];
  }

  // No data was present
  if (sum == 0)
    return -1;

  int mean = sum / IMG_SIZE;
  double sd = 0;
  for (int i = 0; i < IMG_SIZE; i++)
  {
    int diff = (uint)img[i] - mean;
    sd += diff * diff;
  }

  double sd_dev_sq = sd / IMG_SIZE;
  if (!have_bg)
  {
    if (sd_dev_sq < IMGP_BG_SD_DEV * IMGP_BG_SD_DEV)
    {
      memcpy(bg, img, IMG_SIZE);
      have_bg = true;
      printf("Calibrated! Please put your finger on the sensor...\n");
    }
  }
  else
  {
    if (sd_dev_sq > IMGP_SD_DEV * IMGP_SD_DEV)
    {
      int diff[IMG_SIZE];
      int min = 255;
      int max = 0;

      for (int i = 0; i < IMG_SIZE; i++)
      {
        diff[i] = (int)bg[i] - (int)img[i];
        if (diff[i] < min)
          min = diff[i];
        if (diff[i] > max)
          max = diff[i];
      }

      int range = max - min;
      if (range == 0)
        range = 1;

      int count_ridges = 0;
      for (int i = 0; i < IMG_SIZE; i++)
      {
        int normalized = ((diff[i] - min) * 255) / range;

        if (normalized < 0)
          normalized = 0;
        else if (normalized > 255)
          normalized = 255;

        img[i] = (unsigned char)normalized;
        if (img[i] < 150)
          count_ridges++;
      }

      double dark_portion = (double)count_ridges / IMG_SIZE;
      bool finger_present = dark_portion > IMGP_DARK_PORTION;
      printf(">> IMAGE: %d (%.2f)\n", finger_present, dark_portion);

      if (finger_present)
      {
        printf("Saving image to file...\n");
        FILE *fp = fopen("output.bin", "wb");
        if (fp)
        {
          fwrite(img, sizeof(unsigned char), IMG_SIZE, fp);
          printf("Saved.\n");
        }

        return LIBUSB_SUCCESS;
      }
    }
  }

  return 1;
}

int main()
{
  if ((__status = open_egis0576()) != LIBUSB_SUCCESS)
    goto CLEANUP;

  if ((__status = init_sequence()) != LIBUSB_SUCCESS)
    goto CLEANUP;

  printf("Please lift your finger until instructed otherwise. Calibrating...\n");

  int i = 0;
  while (true)
  {
    if (i++ != 0 && (__status = repeat_sequence()) != LIBUSB_SUCCESS)
      goto CLEANUP;

    if ((__status = save_image()) != LIBUSB_SUCCESS)
      goto CLEANUP;

    if ((__status = verify_image()) == LIBUSB_SUCCESS)
      break;

    usleep(50000);
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
