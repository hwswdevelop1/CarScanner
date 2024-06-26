/*
 * Usb.h
 *
 *  Created on: 18 февр. 2021 г.
 *      Author: Evgeny
 */
#pragma once

#include <Types.h>
#include <libopencm3/stm32/st_usbfs.h>
#include <libopencm3/usb/usbd.h>

#define BULK_EP_MAXPACKET 		64
#define USB_DATA_HEAD_SIZE      4

void usbDataInCallback(usbd_device *usbd_dev, uint8_t ep);
void usbDataOutCallback(usbd_device *usbd_dev, uint8_t ep);
void usbdInit();

void usbSendPacket( uint8_t* data, size_t size );
