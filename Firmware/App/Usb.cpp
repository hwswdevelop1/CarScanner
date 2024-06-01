/*
 * Usb.cpp
 *      Author: Evgeny Sobolev 09.02.1984 y.b.
 *      Tel: +79003030374
 *      e-mail: hwswdevelop@gmail.com
 */


#include "Usb.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/usb/dwc/otg_fs.h>
#include "usb_common.h"
#include "UsbPacketBuffer.h"
#include "mem.h"
#include "Types.h"
#include "IntLock.h"

static constexpr const uint8_t usbInEndpointId = 0x81;
static constexpr const uint8_t usbOutEndpointId = 0x01;


usbd_device *usbDevice = nullptr;

extern "C" void USB_IRQHandler(){
	usbd_poll( usbDevice );
	nvic_clear_pending_irq( 67 );
}

struct usbd_msft_sig_descr {
	uint8_t	bLength;
	uint8_t	bType;
	uint8_t wSignature[14];
	uint8_t bVendorCode;
	uint8_t reserved;
};

struct usbd_msft_comp_id {
	uint32_t dwLength;
	uint16_t wBCD;
	uint16_t wCompabilityId;
	uint8_t  bSectionNumber;
	uint8_t  bReserved[7];
	uint8_t  bIfaceNo;
	uint8_t  bReseved1;
	uint8_t  bCompatibleId[8];
	uint8_t  bSubCompatibleId[8];
	uint8_t  bReserved2[6];
};

const usb_device_descriptor usbDeviceDescriptor = {
		.bLength = USB_DT_DEVICE_SIZE,
		.bDescriptorType = USB_DT_DEVICE,
		.bcdUSB = 0x0200,
		.bDeviceClass = 0xFF,
		.bDeviceSubClass = 0x00,
		.bDeviceProtocol = 0x00,
		.bMaxPacketSize0 = BULK_EP_MAXPACKET,
		.idVendor = 0x1111,
		.idProduct = 0x001A,
		.bcdDevice = 0x0200,
		.iManufacturer = 1,
		.iProduct = 2,
		.iSerialNumber = 3,
		.bNumConfigurations = 1,
};

const usb_device_qualifier_descriptor usbDeviceQulifier = {
	.bLength = 10,
	.bDescriptorType = USB_DT_DEVICE_QUALIFIER,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0xFF,
	.bDeviceSubClass = 0x00,
	.bDeviceProtocol = 0x00,
	.bMaxPacketSize0 = BULK_EP_MAXPACKET,
	.bNumConfigurations = 1,
	.bReserved = 0
};


const usb_endpoint_descriptor usbEndpointDescr[] = {
	{
		.bLength = USB_DT_ENDPOINT_SIZE,
		.bDescriptorType = USB_DT_ENDPOINT,
		.bEndpointAddress = usbOutEndpointId,
		.bmAttributes = USB_ENDPOINT_ATTR_BULK,
		.wMaxPacketSize = BULK_EP_MAXPACKET,
		.bInterval = 1,
	},
	{
		.bLength = USB_DT_ENDPOINT_SIZE,
		.bDescriptorType = USB_DT_ENDPOINT,
		.bEndpointAddress = usbInEndpointId,
		.bmAttributes = USB_ENDPOINT_ATTR_BULK,
		.wMaxPacketSize = BULK_EP_MAXPACKET,
		.bInterval = 1,
	}
};

const usb_interface_descriptor usbIfaceDescr[] = {
	{
		.bLength = USB_DT_INTERFACE_SIZE,
		.bDescriptorType = USB_DT_INTERFACE,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = 0xFF,
		.bInterfaceSubClass = 0x00,
		.bInterfaceProtocol = 0x00,
		.iInterface = 5,
		.endpoint = usbEndpointDescr
	}
};

const usb_interface usbIfaces[] = {
	{
		.cur_altsetting = 0,
		.num_altsetting = 1,
		.iface_assoc = 0,
		.altsetting = usbIfaceDescr,
	}
};

const usb_config_descriptor usbConfigDescriptor =
{
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 1,
	.bConfigurationValue = 0,
	.iConfiguration = 4, /* string index */
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,
	.interface = usbIfaces,
};

const usbd_msft_sig_descr usbMsftSigStr = {
	.bLength = 0x12,
	.bType = USB_DT_STRING,
	.wSignature = { 'M', 0x00, 'S', 0x00, 'F', 0x00, 'T', 0x00, '1', 0x00, '0', 0x00, '0', 0x00 },
	.bVendorCode = 0x40,
	.reserved = 0x00
};

const usbd_msft_comp_id usbMsftWinUsb = {
	.dwLength = 0x28,
	.wBCD = 0x0100,
	.wCompabilityId = 0x0004,
	.bSectionNumber = 0x01,
	.bReserved = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	.bIfaceNo = 0x00,
	.bReseved1 = 0x01,
	.bCompatibleId = { 'W', 'I', 'N', 'U', 'S', 'B', 0, 0 },
	.bSubCompatibleId = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	.bReserved2 = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

const char *usbStrings[] = {
	"hwswdevelop@gmail.com",
	"Car Scanner",
	"00000000",
	"Car Scanner",
	"Car Scanner",
	"Car Scanner",
	"Car Scanner",
};

uint8_t usbControlBuffer[5*64];


void usbSetConfigCallback(usbd_device *usbd_dev, uint16_t wValue){

	((void)wValue);

	// PC to Device endpoint
	usbd_ep_setup(usbd_dev, usbOutEndpointId, USB_ENDPOINT_ATTR_BULK, BULK_EP_MAXPACKET,
				usbDataOutCallback);

	// Device to PC endpoint
	usbd_ep_setup(usbd_dev, usbInEndpointId, USB_ENDPOINT_ATTR_BULK, BULK_EP_MAXPACKET,
			usbDataInCallback);

	// Disable Transmit by default
	usbd_ep_nak_set( usbd_dev, usbInEndpointId, true );

}


usbd_request_return_codes usbControlCallback(
		_usbd_device *usbd_dev,
		struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
		usbd_control_complete_callback *complete){

	if ( req->bmRequestType == USB_REQ_TYPE_IN ) {

		uint8_t bDescriptorType = (req->wValue >> 8) & 0xFF;
		uint8_t bDescriptorIndex = req->wValue & 0xFF;

		if ( req->bRequest == USB_REQ_GET_DESCRIPTOR ) {

			if  ( (bDescriptorType == USB_DT_DEVICE_QUALIFIER) && (bDescriptorIndex == 0) ){
				*len = usbDeviceQulifier.bLength;
				*buf = (uint8_t*)&usbDeviceQulifier;
				return USBD_REQ_HANDLED;
			}

			if ( (bDescriptorType == USB_DT_STRING) && (bDescriptorIndex == 0xEE) ){
				*buf = (uint8_t*)&usbMsftSigStr;
				*len = sizeof(usbMsftSigStr);
				return USBD_REQ_HANDLED;
			}

		}
	} else if ( req->bmRequestType ==  (USB_REQ_TYPE_IN | USB_REQ_TYPE_VENDOR) ){
		if (req->bRequest == usbMsftSigStr.bVendorCode){
			if ( req->wIndex == 0x0004 ){
				*buf = (uint8_t*)&usbMsftWinUsb;
				*len = (req->wLength < sizeof(usbMsftWinUsb) ) ? req->wLength : sizeof(usbMsftWinUsb);
				return USBD_REQ_HANDLED;
			}
		}
	}
	return USBD_REQ_NEXT_CALLBACK;
}

void usbDataOutCallback(usbd_device *usbd_dev, uint8_t ep){
	//int_lock_t key = intLock();
	do {
		auto p = usbBufferAlloc( PoolType::UsbToPeriph, BULK_EP_MAXPACKET );
		if ( InvalidIndex == p ) {
			usbd_ep_nak_set( usbDevice, ep, true );
			break;
		}
		auto data = usbBufferGetDataPtr( PoolType::UsbToPeriph, p );
		if ( nullptr == data ) break;

		const size_t size = usbBufferGetSize( PoolType::UsbToPeriph, p );

		// Read next USB packet, otherwise send NAK packet (check it later) ???
		const size_t readSize = usbd_ep_read_packet( usbd_dev, ep, data, size );
		if ( 0 == readSize ){
			usbBufferFree( PoolType::UsbToPeriph, p );
		} else {
			usbBufferUpdateSize( PoolType::UsbToPeriph, p, readSize );
			usbBufferCommit( PoolType::UsbToPeriph, p );
		}

		auto newAllocIndex = usbBufferAlloc( PoolType::UsbToPeriph, BULK_EP_MAXPACKET, true );
		if ( InvalidIndex == newAllocIndex) {
			usbd_ep_nak_set( usbDevice, ep, true );
		}

	} while(false);
	//intUnlock(key);
}


static volatile bool _usbTxInProgress = false;

void usbDataInCallback(usbd_device *usbd_dev, uint8_t ep) {
	//int_lock_t key = intLock();
	const PoolType pool = PoolType::PeriphToUsb;
	const IndexType p = usbBufferGet( pool );
	if  ( InvalidIndex != p ) {
		uint8_t* const data = usbBufferGetDataPtr( pool, p );
		size_t size = usbBufferGetSize( pool, p );
		usbBufferCheckout( pool, p );
		usbd_ep_write_packet( usbd_dev, ep, data, size );
		usbBufferFree( pool, p );
	} else {
		_usbTxInProgress = false;
	}
	//intUnlock(key);
}

void usbSendPacket() {
	int_lock_t key = intLock();
	if (!_usbTxInProgress) {
		const PoolType pool = PoolType::PeriphToUsb;
		const IndexType p = usbBufferGet( pool );
		if ( InvalidIndex != p ) {
			_usbTxInProgress = true;
			usbBufferCheckout( pool, p );
			uint8_t* const data = usbBufferGetDataPtr( pool, p );
			const size_t size = usbBufferGetSize( pool, p );
			usbd_ep_write_packet( usbDevice, usbInEndpointId, data, size );
			usbBufferFree( pool, p );
		}
	}
	intUnlock(key);
}


void usbSetAck() {
	usbd_ep_nak_set( usbDevice, usbOutEndpointId, false );
}

extern const struct _usbd_driver stm32f107_usb_driver;

#include "../libopencm3/lib/usb/usb_dwc_common.h"

void usbdInit(){

	usbDevice = usbd_init(  &stm32f107_usb_driver,
				&usbDeviceDescriptor,
				&usbConfigDescriptor,
				usbStrings,
				7,
				usbControlBuffer,
				sizeof(usbControlBuffer)
				);

	usbd_register_set_config_callback( usbDevice, usbSetConfigCallback );
	usbd_register_control_callback( usbDevice, 0x00, 0x00, usbControlCallback );
	dwc_disconnect( usbDevice, false );

}

