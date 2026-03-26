#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libusb-1.0/libusb.h>

#include "eh0576.h"

bool have_bg = false;
unsigned char bg[IMG_SIZE] = { 0 };
unsigned char img[IMG_SIZE] = { 0 };

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

void write_image_to_disk()
{
  printf("Saving raw image to file...\n");
  FILE *fp_raw = fopen("output_raw.bin", "wb");
  if (!fp_raw)
    return;

  fwrite(img, sizeof(unsigned char), IMG_SIZE, fp_raw);
  printf("Saved raw image.\n");
}

int verify_image()
{
  double variance = get_variance(img);
  if (!have_bg)
  {
    if (variance < IMGP_BG_VARIANCE)
    {
      memcpy(bg, img, IMG_SIZE);
      have_bg = true;
      printf("Calibrated! Please put your finger on the sensor...\n");
    }
  }
  else
  {
    if (variance > IMGP_VARIANCE)
    {
      double dark_portion;
      normalize_img(bg, img, &dark_portion);

      bool finger_present = dark_portion > IMGP_DARK_PORTION;
      printf(">> IMAGE: %d (%.2f)\n", finger_present, dark_portion);

      if (finger_present)
      {
        write_image_to_disk();
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

    // write_image_to_disk();
    // break;

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
