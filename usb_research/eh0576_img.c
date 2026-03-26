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

void upscale2x_bilinear_img(unsigned char *src_img, unsigned char *dst_img)
{
  for (int y = 0; y < IMG_HEIGHT_2X; y++)
  {
    int src_y = y / 2;
    int src_y_next = (src_y + 1 < IMG_HEIGHT) ? src_y + 1 : src_y;

    for (int x = 0; x < IMG_WIDTH_2X; x++)
    {
      int src_x = x / 2;
      int src_x_next = (src_x + 1 < IMG_WIDTH) ? src_x + 1 : src_x;

      int p00 = src_img[src_y * IMG_WIDTH + src_x];
      int p10 = src_img[src_y * IMG_WIDTH + src_x_next];
      int p01 = src_img[src_y_next * IMG_WIDTH + src_x];
      int p11 = src_img[src_y_next * IMG_WIDTH + src_x_next];

      int final_val;

      int x_mod_2 = x % 2;
      int y_mod_2 = y / 2;
      if (x_mod_2 == 0 && y_mod_2 == 0)
      {
        final_val = p00;
      }
      else if (x_mod_2 != 0 && y_mod_2 == 0)
      {
        final_val = (p00 + p10) / 2;
      }
      else if (x_mod_2 == 0 && y_mod_2 != 0)
      {
        final_val = (p00 + p01) / 2;
      }
      else
      {
        final_val = (p00 + p10 + p01 + p11) / 4;
      }

      dst_img[y * (IMG_WIDTH_2X) + x] = (unsigned char)final_val;
    }
  }
}
