#include <stdio.h>
#include <unistd.h>

#include <libusb-1.0/libusb.h>

#include "eh0576.h"

libusb_context *ctx = NULL;
libusb_device_handle *h = NULL;
struct libusb_device *dev = NULL;

int open_and_handshake()
{
  libusb_init(&ctx);

  // Open device
  h = libusb_open_device_with_vid_pid(ctx, VID, PID);
  if (!h)
  {
    printf("Could not open device.\n");
    dispose();
    return -1;
  }

  dev = libusb_get_device(h);

  // Mimick windows setup
  if (perform_setup() != LIBUSB_SUCCESS)
  {
    dispose();
    return -1;
  }

  // Do proprietary handshake
  if (perform_handshake() != LIBUSB_SUCCESS)
  {
    dispose();
    return -1;
  }

  return LIBUSB_SUCCESS;
}

void dispose()
{
  if (h)
  {
    libusb_close(h);
  }

  if (ctx)
  {
    libusb_exit(ctx);
  }
}

void print_data(unsigned char *data, int size)
{
  for (int i = 0; i < size; i++)
  {
    printf("%X", data[i]);
  }

  printf(" '");
  for (int i = 0; i < size; i++)
  {
    printf("%c", data[i]);
  }
  printf("'\n");
}

int perform_setup()
{
  struct libusb_device_descriptor desc_dev;

  // GET DESCRIPTOR Request DEVICE
  int res = libusb_get_device_descriptor(dev, &desc_dev);
  if (res != LIBUSB_SUCCESS)
  {
    printf("Could not get descriptor for device: %s\n", libusb_error_name(res));
    return -1;
  }

  // printf("Received device descriptor.\n");

  // GET DESCRIPTOR Request CONFIGURATION
  struct libusb_config_descriptor *desc_conf;
  res = libusb_get_config_descriptor(dev, 0, &desc_conf);
  if (res != LIBUSB_SUCCESS)
  {
    printf("Could not get descriptor for config: %s\n", libusb_error_name(res));
    return -1;
  }

  // printf("Received config descriptor.\n");
  libusb_free_config_descriptor(desc_conf);

  // GET DESCRIPTOR Request STRING (SerialNumber)
  unsigned char setup_desc_serialno[255];
  int serialno_len
    = libusb_get_string_descriptor_ascii(h, desc_dev.iSerialNumber, setup_desc_serialno, 255);
  if (serialno_len <= LIBUSB_SUCCESS)
  {
    printf("Could not get serial number: %s\n", libusb_error_name(res));
    return -1;
  }

  // printf("Received serial number.\n");
  // print_data(setup_desc_serialno, serialno_len);

  res = libusb_set_configuration(h, 1);
  if (res != LIBUSB_SUCCESS)
  {
    printf("Could not set config: %s\n", libusb_error_name(res));
    return -1;
  }

  return LIBUSB_SUCCESS;
}

int perform_handshake()
{
  unsigned char handshake_1[16];
  int res = libusb_control_transfer(h, 0xc0, 4, 0x0000, 0x0004, handshake_1, 16, TIMEOUT);
  if (res < LIBUSB_SUCCESS)
  {
    printf("Could not execute FIRST handshake: %s\n", libusb_error_name(res));
    return -1;
  }

  // printf("Received response from FIRST handshake.\n");

  unsigned char handshake_2[40];
  res = libusb_control_transfer(h, 0xc0, 4, 0x0000, 0x0004, handshake_2, 40, TIMEOUT);
  if (res < LIBUSB_SUCCESS)
  {
    printf("Could not execute SECOND handshake: %s\n", libusb_error_name(res));
    return -1;
  }

  // printf("Received response from SECOND handshake.\n");

  // printf("Handshake completed.\n");
  return LIBUSB_SUCCESS;
}

int send_egis_pkt(unsigned char *seg_buf, const int seg_len, unsigned char *resp_buf,
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

int send_egis_pkt_t(unsigned char *seg_buf, const int seg_len, unsigned char *resp_buf,
                    const int resp_len, int *rcvd, int delay)
{
  int sent = 0;
  *rcvd = 0;

  int r = libusb_bulk_transfer(h, 0x01, seg_buf, seg_len, &sent, TIMEOUT);
  if (r != LIBUSB_SUCCESS)
  {
    printf("Could not sent data: %s\n", libusb_error_name(r));
    return -1;
  }

  usleep(delay);

  r = libusb_bulk_transfer(h, 0x82, resp_buf, resp_len, rcvd, TIMEOUT);
  if (r != LIBUSB_SUCCESS)
  {
    printf("Could not receive data: %s\n", libusb_error_name(r));
    return -1;
  }

  return LIBUSB_SUCCESS;
}
