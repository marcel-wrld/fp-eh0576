#include "eh0576.h"

double get_sd_dev_sq(unsigned char *img)
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

  return sd / IMG_SIZE;
}

void normalize_img(unsigned char *bg, unsigned char *img, double *dark_portion)
{
  int diff[IMG_SIZE];
  int min = 255;
  int max = 0;

  for (int i = 0; i < IMG_SIZE; i++)
  {
    diff[i] = (int)img[i] - (int)bg[i];
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
    int normalized = 255 - (((diff[i] - min) * 255) / range);

    if (normalized < 10)
      normalized = 0;
    else if (normalized > 245)
      normalized = 255;

    img[i] = (unsigned char)normalized;
    if (img[i] > 170)
      count_ridges++;
  }

  *dark_portion = (double)count_ridges / IMG_SIZE;
}

void upscale_2x(unsigned char *src_img, unsigned char *dst_img)
{
  for (int y = 0; y < IMG_HEIGHT_2X; y++)
  {
    int src_y = y / 2;

    for (int x = 0; x < IMG_WIDTH_2X; x++)
    {
      int src_x = x / 2;
      dst_img[y * IMG_WIDTH_2X + x] = src_img[src_y * IMG_WIDTH + src_x];
    }
  }
}
