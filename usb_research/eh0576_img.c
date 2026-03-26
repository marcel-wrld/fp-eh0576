#include "eh0576.h"

double get_variance(unsigned char *img)
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
    diff[i] = (int)bg[i] - (int)img[i];
    if (diff[i] < min)
      min = diff[i];
    if (diff[i] > max)
      max = diff[i];
  }

  max -= IMGP_CONTRAST;
  min += IMGP_CONTRAST;
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
    if (img[i] < 170)
      count_ridges++;
  }

  *dark_portion = (double)count_ridges / IMG_SIZE;
}
