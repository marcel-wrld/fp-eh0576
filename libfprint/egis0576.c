/*
 * Egis Technology Inc. (aka. LighTuning) 0576 driver for libfprint
 * Copyright (C) 2026 Marcel (Sprayxe) <sprayxe.marcel@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#define FP_COMPONENT "egis0576"

#include "egis0576.h"

#include "drivers_api.h"

/* Sequence types */
typedef enum
{
  SEQ_INIT,
  SEQ_REPEAT,
  SEQ_POLL,
  SEQ_IMAGE
} seq_types;

/* SSM States */
enum sm_states
{
  DEV_OPEN,
  DEV_START,
  DEV_REQ,
  DEV_RESP,
  DEV_FULFILLED,
  NUM_STATES
};

/* Image process results */
#define INVALID_BUFFER -1

/* Struct */
struct _FpDeviceEgis0576
{
  FpImageDevice parent;

  gboolean running;
  gboolean stop;

  gboolean has_background;
  guchar background[EGIS0576_IMG_SIZE];

  seq_types seq_type;
  int seq_pkt_index;
  Egis0576Pkt last_sent_pkt;
};
G_DECLARE_FINAL_TYPE(FpDeviceEgis0576, fpi_device_egis0576, FPI, DEVICE_EGIS0576, FpImageDevice);
G_DEFINE_TYPE(FpDeviceEgis0576, fpi_device_egis0576, FP_TYPE_IMAGE_DEVICE);

/*
 * ========================
 * Processing
 * ========================
 */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

static void normalize_img(guchar *bg, guchar *img, double *dark_portion)
{
  // Find diffs, min and max
  int diff[EGIS0576_IMG_SIZE];
  int min = 255;
  int max = 0;
  for (int i = 0; i < EGIS0576_IMG_SIZE; i++)
  {
    diff[i] = (int)img[i] - (int)bg[i];
    if (diff[i] < min)
      min = diff[i];
    if (diff[i] > max)
      max = diff[i];
  }

  int range = max - min;
  if (range == 0)
    range = 1;  // Prevent division by zero

  // Adjust contrast / normalize
  int count_ridges = 0;
  for (int i = 0; i < EGIS0576_IMG_SIZE; i++)
  {
    int normalized = 255 - (((diff[i] - min) * 255) / range);

    if (normalized < 0)
      normalized = 0;
    else if (normalized > 255)
      normalized = 255;

    img[i] = (unsigned char)normalized;
    if (img[i] > 170)
      count_ridges++;
  }

  *dark_portion = (double)count_ridges / EGIS0576_IMG_SIZE;
}

static void upscale2x(guchar *src_img, guchar *dst_img)
{
  for (int y = 0; y < EGIS0576_IMG_HEIGHT_2X; y++)
  {
    int src_y = y / 2;

    for (int x = 0; x < EGIS0576_IMG_WIDTH_2X; x++)
    {
      int src_x = x / 2;
      dst_img[y * EGIS0576_IMG_WIDTH_2X + x] = src_img[src_y * EGIS0576_IMG_WIDTH + src_x];
    }
  }
}

static void process_finger(FpDevice *dev, FpiUsbTransfer *transfer)
{
  FpImageDevice *img_self = FP_IMAGE_DEVICE(dev);
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576(dev);

  guchar *img = transfer->buffer;

  gint sd_dev_sq = fpi_std_sq_dev(img, EGIS0576_IMG_SIZE);
  if (!self->has_background)
  {
    /* Background has been gathered, user can put finger on sensor. */
    if (sd_dev_sq < EGIS0576_BG_SD_DEV * EGIS0576_BG_SD_DEV)
    {
      memcpy(self->background, img, EGIS0576_IMG_SIZE);
      self->has_background = TRUE;

      self->seq_type = SEQ_REPEAT;
      fpi_ssm_next_state_delayed(transfer->ssm, 50);

      fpi_device_report_finger_status(dev, FP_FINGER_STATUS_NEEDED);
      return;
    }

    /* User should remove finger so the driver can grab a clear image. */
    fpi_image_device_retry_scan(img_self, FP_DEVICE_RETRY_REMOVE_FINGER);
    return;
  }

  gboolean finger_present = FALSE;
  double dark_portion = -1;
  if (sd_dev_sq > EGIS0576_SD_DEV * EGIS0576_SD_DEV)
  {
    normalize_img(self->background, img, &dark_portion);
    finger_present = dark_portion > EGIS0576_DARK_PORTION;
  }

  fp_dbg("Finger status (present, sd dev sq, dark port) : "
         "%d , %d, %.2f",
         finger_present, sd_dev_sq, dark_portion);

  if (!finger_present)
  {
    self->seq_type = SEQ_REPEAT;
    fpi_image_device_report_finger_status(img_self, FALSE);
    fpi_ssm_next_state_delayed(transfer->ssm, 50);
    return;
  }

  FpImage *fpImg = fp_image_new(EGIS0576_IMG_WIDTH_2X, EGIS0576_IMG_HEIGHT_2X);

  /* Sensor returns full image */
  // memcpy(fpImg->data, img, EGIS0576_IMG_SIZE);

  upscale2x(img, fpImg->data);

  fpi_image_device_report_finger_status(img_self, TRUE);
  fpi_image_device_image_captured(img_self, fpImg);

  fpi_ssm_next_state_delayed(transfer->ssm, 50);
}

static void process_poll_transfer(FpDevice *dev, FpiUsbTransfer *transfer)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576(dev);

  if (transfer->actual_length < 6)
  {
    GError *error
      = fpi_device_error_new_msg(FP_DEVICE_ERROR_DATA_INVALID, "Device reported invalid poll.");
    fpi_ssm_mark_failed(transfer->ssm, error);
    g_error_free(error);
    return;
  }

  if ((transfer->buffer[6] & 0x01) == 0x01)
  {
    self->seq_type = SEQ_IMAGE;
    fpi_ssm_jump_to_state(transfer->ssm, DEV_REQ);
    return;
  }

  self->seq_pkt_index += 1;
  if (self->seq_pkt_index < EGIS0576_POLL_COUNT)
  {
    fpi_ssm_jump_to_state(transfer->ssm, DEV_REQ);
    return;
  }

  GError *error
    = fpi_device_error_new_msg(FP_DEVICE_ERROR_GENERAL, "Device exceeded maximum poll count.");
  fpi_ssm_mark_failed(transfer->ssm, error);
  g_error_free(error);
}

/*
 * Returns PIMG_INVALID_DATA if the transfer buffer was invalid (e.g. all
 * zeros).
 * Else it will return 0 if no finger is present or 1 if a finger is present.
 */
static void process_image_transfer(FpDevice *dev, FpiUsbTransfer *transfer)
{
  guchar *buffer = transfer->buffer;
  gssize buffer_len = transfer->actual_length;

  if (buffer_len != EGIS0576_IMG_SIZE)
  {
    GError *error = fpi_device_error_new_msg(
      FP_DEVICE_ERROR_DATA_INVALID, "Device image data size does not match expected size.");
    fpi_ssm_mark_failed(transfer->ssm, error);
    g_error_free(error);
    return;
  }

  uint sum = 0;
  /* Roughly check whether the buffer is empty aka invalid. */
  for (int i = 0; i < MIN(buffer_len, 255); i++)
  {
    sum += buffer[i];
  }

  /* No/invalid data was present. */
  if (sum == 0)
  {
    GError *error
      = fpi_device_error_new_msg(FP_DEVICE_ERROR_DATA_INVALID, "Device reported invalid data.");
    fpi_ssm_mark_failed(transfer->ssm, error);
    g_error_free(error);
    return;
  }

  process_finger(dev, transfer);
}

/*
 * ========================
 * I / O
 * ========================
 */
static void cmd_resp_cb(FpiUsbTransfer *transfer, FpDevice *dev, gpointer user_data, GError *error)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576(dev);

  if (error)
  {
    fp_dbg("During the %d sequence an error occurred at pkt index %d", self->seq_type,
           self->seq_pkt_index);
    fpi_ssm_mark_failed(transfer->ssm, error);
    return;
  }

  switch (self->seq_type)
  {
  /* not processed */
  case SEQ_INIT:
  case SEQ_REPEAT:
    fpi_ssm_jump_to_state(transfer->ssm, DEV_REQ);
    break;

  case SEQ_POLL:
    process_poll_transfer(dev, transfer);
    break;

  case SEQ_IMAGE:
    process_image_transfer(dev, transfer);
    break;
  }
}

static void recv_cmd_resp(FpiSsm *ssm, FpDevice *dev, Egis0576Pkt last_pkt)
{
  FpiUsbTransfer *transfer = fpi_usb_transfer_new(dev);

  fpi_usb_transfer_fill_bulk(transfer, EGIS0576_EPIN, last_pkt.res_len);

  transfer->ssm = ssm;

  fpi_usb_transfer_submit(transfer, EGIS0576_TIMEOUT, NULL, cmd_resp_cb, NULL);
}

static void send_cmd_req(FpiSsm *ssm, FpDevice *dev, Egis0576Pkt pkt)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576(dev);
  FpiUsbTransfer *transfer = fpi_usb_transfer_new(dev);

  self->last_sent_pkt = pkt;
  fpi_usb_transfer_fill_bulk_full(transfer, EGIS0576_EPOUT, pkt.cmd, pkt.len, NULL);

  transfer->ssm = ssm;
  transfer->short_is_error = TRUE;

  fpi_usb_transfer_submit(transfer, EGIS0576_TIMEOUT, NULL, fpi_ssm_usb_transfer_cb, NULL);
}

static gboolean init_repeat_last_pkt(FpDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576(dev);

  int type = self->seq_type;
  int index = self->seq_pkt_index;

  return (type == SEQ_INIT && index == EGIS0576_INIT_PACKETS_LENGTH - 1)
         || (type == SEQ_REPEAT && index == EGIS0576_REPEAT_PACKETS_LENGTH - 1);
}

static void recv_cmd(FpiSsm *ssm, FpDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576(dev);
  Egis0576Pkt last_pkt = self->last_sent_pkt;

  switch (self->seq_type)
  {
  case SEQ_INIT:
  case SEQ_REPEAT:
    if (!init_repeat_last_pkt(dev))
    {
      recv_cmd_resp(ssm, dev, last_pkt);
      self->seq_pkt_index += 1;
    }
    else
    {
      self->seq_pkt_index = 0;
      self->seq_type = SEQ_POLL;
      fpi_ssm_jump_to_state(ssm, DEV_REQ);
    }

    break;

  case SEQ_POLL:
  case SEQ_IMAGE:
    recv_cmd_resp(ssm, dev, last_pkt);
    break;
  }
}

static void send_cmd(FpiSsm *ssm, FpDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576(dev);

  switch (self->seq_type)
  {
  case SEQ_INIT:
    send_cmd_req(ssm, dev, EGIS0576_INIT_PACKETS[self->seq_pkt_index]);
    break;

  case SEQ_REPEAT:
    send_cmd_req(ssm, dev, EGIS0576_REPEAT_PACKETS[self->seq_pkt_index]);
    break;

  case SEQ_POLL:
    send_cmd_req(ssm, dev, EGIS0576_POLL_PACKET);
    break;

  case SEQ_IMAGE:
    send_cmd_req(ssm, dev, EGIS0576_IMAGE_PACKET);
    break;
  }
}

static void ssm_run_state(FpiSsm *ssm, FpDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576(dev);

  switch (fpi_ssm_get_cur_state(ssm))
  {
  case DEV_OPEN:
    self->seq_type = SEQ_INIT;
    fpi_ssm_jump_to_state(ssm, DEV_START);
    break;

  case DEV_START:
    if (self->stop)
    {
      fp_dbg("Deactivating device, marking completed.");
      fpi_ssm_mark_completed(ssm);
      return;
    }

    self->seq_pkt_index = 0;
    fpi_ssm_jump_to_state(ssm, DEV_REQ);
    break;

  case DEV_REQ:
    send_cmd(ssm, dev);
    break;

  case DEV_RESP:
    recv_cmd(ssm, dev);
    break;

  case DEV_FULFILLED:
    fpi_ssm_jump_to_state(ssm, DEV_START);
    break;

  default:
    g_assert_not_reached();
  }
}

/*
 * ========================
 * SETUP
 * ========================
 */

static void sm_cb(FpiSsm *ssm, FpDevice *dev, GError *error)
{
  FpImageDevice *img_dev = FP_IMAGE_DEVICE(dev);
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576(dev);

  self->running = FALSE;

  if (error && !self->stop)
    fpi_image_device_session_error(img_dev, error);
  else if (error)
    g_error_free(error);

  if (self->stop)
    fpi_image_device_deactivate_complete(img_dev, NULL);
}

/*
 * Device activate
 */
static void dev_activate(FpImageDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576(dev);
  FpiSsm *ssm = fpi_ssm_new(FP_DEVICE(dev), ssm_run_state, NUM_STATES);

  self->stop = FALSE;

  fpi_ssm_start(ssm, sm_cb);

  self->running = TRUE;

  fpi_image_device_activate_complete(dev, NULL);
}

/*
 * Img open
 */
static void dev_init(FpImageDevice *dev)
{
  GError *error = NULL;

  g_usb_device_claim_interface(fpi_device_get_usb_device(FP_DEVICE(dev)), EGIS0576_INTF, 0, &error);

  fpi_image_device_open_complete(dev, error);
}

/*
 * Img close
 */
static void dev_deinit(FpImageDevice *dev)
{
  GError *error = NULL;

  g_usb_device_release_interface(fpi_device_get_usb_device(FP_DEVICE(dev)), EGIS0576_INTF, 0,
                                 &error);

  fpi_image_device_close_complete(dev, error);
}

/*
 * Device deactivate
 */
static void dev_deactivate(FpImageDevice *dev)
{
  FpDeviceEgis0576 *self = FPI_DEVICE_EGIS0576(dev);

  if (self->running)
    self->stop = TRUE;
  else
    fpi_image_device_deactivate_complete(dev, NULL);
}

/*
 * Driver ID
 */
static const FpIdEntry id_table[] = {
  {
    .vid = 0x1c7a,
    .pid = 0x0576,
  },
  {
    .vid = 0,
    .pid = 0,
  },
};

static void fpi_device_egis0576_init(FpDeviceEgis0576 *self)
{
}

static void fpi_device_egis0576_class_init(FpDeviceEgis0576Class *klass)
{
  FpDeviceClass *dev_class = FP_DEVICE_CLASS(klass);
  FpImageDeviceClass *img_class = FP_IMAGE_DEVICE_CLASS(klass);

  dev_class->id = "egis0576";
  dev_class->full_name = "Egis Technology Inc. (aka. LighTuning) 0576";
  dev_class->type = FP_DEVICE_TYPE_USB;
  dev_class->id_table = id_table;
  dev_class->scan_type = FP_SCAN_TYPE_PRESS;
  dev_class->nr_enroll_stages = 5;

  img_class->img_open = dev_init;
  img_class->img_close = dev_deinit;
  img_class->activate = dev_activate;
  img_class->deactivate = dev_deactivate;

  img_class->img_width = EGIS0576_IMG_WIDTH_2X;
  img_class->img_height = EGIS0576_IMG_HEIGHT_2X;

  // img_class->bz3_threshold = 10; /* security issue */
}
