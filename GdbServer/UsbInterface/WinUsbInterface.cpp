
#include "WinUsbInterface.h"
#include <SetupAPI.h>
#include <winusb.h>
#include <Usb100.h>
#include <stdio.h>


DEFINE_GUID(WinUSB_GUID, 0x88bae032, 0x5a81, 0x49f0, 0xbc, 0x3d, 0xa4, 0xff, 0x13, 0x82, 0x16, 0xd6);

#pragma comment(lib, "SetupAPI.lib")
#pragma comment(lib, "WinUsb.lib")
#pragma comment(lib, "Ws2_32.lib")



/* USB Descriptor Types - Table 9-5 */
#define USB_DT_DEVICE				        1
#define USB_DT_CONFIGURATION			    2
#define USB_DT_STRING				        3
#define USB_DT_INTERFACE		    	    4
#define USB_DT_ENDPOINT				        5
#define USB_DT_DEVICE_QUALIFIER			    6
#define USB_DT_OTHER_SPEED_CONFIGURATION	7
#define USB_DT_INTERFACE_POWER			    8
/* From ECNs */
#define USB_DT_OTG				            9
#define USB_DT_DEBUG				        10
#define USB_DT_INTERFACE_ASSOCIATION		11

#define USB_LANGID_ENGLISH_US               (0x409)


/* USB Standard Device Descriptor - Table 9-8 */
union DeviceDescriptorUnion {
    uint32_t AllignedData[5]; // It will be alligned to 32bit, and DeviceDescriptor, will be alligned
    struct DeviceDescriptor {
        uint8_t bLength;			// 1
        uint8_t bDescriptorType;	// 2
        uint16_t bcdUSB;			// 4
        uint8_t bDeviceClass;		// 5
        uint8_t bDeviceSubClass;	// 6
        uint8_t bDeviceProtocol;	// 7
        uint8_t bMaxPacketSize0;	// 8
        uint16_t idVendor;			// 10
        uint16_t idProduct;			// 12
        uint16_t bcdDevice;			// 14
        uint8_t iManufacturer;		// 15
        uint8_t iProduct;			// 16
        uint8_t iSerialNumber;		// 17
        uint8_t bNumConfigurations;	// 18
    } Descriptor;// __attribute__((packed));
};


#include <iostream>

WinUsbInterface::WinUsbInterface(class WinUsbInterfaceFabric& fabric, std::wstring deviceName) : _fabric(fabric) {
    const WCHAR* name = deviceName.c_str();
    _WinUsbIfaceHandle = 0;
    _WinUsbDeviceHandle = INVALID_HANDLE_VALUE;

    _vendorId = 0;
    _productId = 0;
    memset(_productName, 0, sizeof(_productName));
    memset(_manufacturerName, 0, sizeof(_manufacturerName));
    memset(_serialNumber, 0, sizeof(_serialNumber));

    do {

        // Open WinUSB device
        _WinUsbDeviceHandle = CreateFileW(
            name,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_ALWAYS,
            FILE_FLAG_OVERLAPPED,
            NULL);

        if (_WinUsbDeviceHandle == INVALID_HANDLE_VALUE) break;

        // Open WinUSB handle
        BOOL usbDeviceOpened = WinUsb_Initialize(_WinUsbDeviceHandle, &_WinUsbIfaceHandle);
        if (!usbDeviceOpened) {
            CloseHandle(_WinUsbDeviceHandle);
            _WinUsbDeviceHandle = INVALID_HANDLE_VALUE;
        }

        // Get Device Descriptor data
        DeviceDescriptorUnion DeviceDescriptorData;
        ULONG RealDeviceDescriptorSize = 0;

        BOOL deviceDescriptorGot = WinUsb_GetDescriptor(_WinUsbIfaceHandle, USB_DT_DEVICE, 0, USB_LANGID_ENGLISH_US, (UCHAR*)&DeviceDescriptorData, sizeof(DeviceDescriptorData), &RealDeviceDescriptorSize);
        if (!deviceDescriptorGot) break;

        _vendorId = DeviceDescriptorData.Descriptor.idVendor;
        _productId = DeviceDescriptorData.Descriptor.idProduct;


        // Get Manufacturer Name
        WCHAR ManufacturerName[maxStringSize];
        ULONG realManufacturerNameDescrSize = 0;
        memset(ManufacturerName, 0, sizeof(ManufacturerName));

        BOOL manufacturerNameDescriptorGot = WinUsb_GetDescriptor(_WinUsbIfaceHandle, USB_DT_STRING, DeviceDescriptorData.Descriptor.iManufacturer, USB_LANGID_ENGLISH_US, (PUCHAR)&ManufacturerName, sizeof(ManufacturerName), &realManufacturerNameDescrSize);
        if (!manufacturerNameDescriptorGot) break;

        // Get Product Name
        WCHAR ProductName[maxStringSize];
        ULONG realProductNameDescrSize = 0;
        memset(ProductName, 0, sizeof(ProductName) );

        BOOL productNameDescriptorGot = WinUsb_GetDescriptor(_WinUsbIfaceHandle, USB_DT_STRING, DeviceDescriptorData.Descriptor.iProduct, USB_LANGID_ENGLISH_US, (PUCHAR)&ProductName, sizeof(ProductName), &realProductNameDescrSize);
        if (!productNameDescriptorGot) break;

        // Get Serail Number
        WCHAR SerialNumber[maxStringSize];
        ULONG realSerialNumberDescrSize = 0;
        memset(SerialNumber, 0, sizeof(SerialNumber));
        BOOL serialNumberDescriptorGot = WinUsb_GetDescriptor(_WinUsbIfaceHandle, USB_DT_STRING, DeviceDescriptorData.Descriptor.iSerialNumber, USB_LANGID_ENGLISH_US, (PUCHAR)&SerialNumber, sizeof(SerialNumber), &realSerialNumberDescrSize);
        if (!serialNumberDescriptorGot) break;

        wsprintfA( _manufacturerName, "%ls", &ManufacturerName[1] );
        wsprintfA( _productName, "%ls", &ProductName[1] );
        wsprintfA( _serialNumber, "%ls", &SerialNumber[1] );

        return;

    } while (false);

    if (0 != _WinUsbIfaceHandle) {
        WinUsb_Free(_WinUsbIfaceHandle);
        _WinUsbIfaceHandle = 0;
    }

    if (INVALID_HANDLE_VALUE != _WinUsbDeviceHandle) {
        CloseHandle(_WinUsbDeviceHandle);
        _WinUsbDeviceHandle = INVALID_HANDLE_VALUE;
    }

}

int WinUsbInterface::getVendorId() {
    return _vendorId;
}

int WinUsbInterface::getProductId() {
    return _productId;
}

const char* WinUsbInterface::getSerialNumber() {
    return _serialNumber;
}
const char* WinUsbInterface::getManufacturerName() {
    return _manufacturerName;
}
const char* WinUsbInterface::getProductName() {
    return _productName;
}

WinUsbInterface::~WinUsbInterface() {
    if (_WinUsbDeviceHandle != INVALID_HANDLE_VALUE) {
        //_WinUsbIfaceHandle
        WinUsb_Free(_WinUsbIfaceHandle);
        CloseHandle(_WinUsbDeviceHandle);
        _WinUsbDeviceHandle = INVALID_HANDLE_VALUE;
    }
}

size_t WinUsbInterface::writePacket(uint8_t* out, size_t size) {
    ULONG txSize = size;
    BOOL isSent = WinUsb_WritePipe(_WinUsbIfaceHandle, outEndpointId, out, txSize, &txSize, NULL);
    return isSent ? txSize : 0;
}

size_t WinUsbInterface::readPacket(uint8_t* in, size_t size) {
    ULONG rxSize = size;
    BOOL isReceived = WinUsb_ReadPipe(_WinUsbIfaceHandle, inEndpointId, in, rxSize, &rxSize, NULL);
    return isReceived ? rxSize : 0;
}

size_t WinUsbInterfaceFabric::getInterfaceCount() {
    return _targetList.size();
}

WinUsbInterface* WinUsbInterfaceFabric::getInterface(size_t index) {
    size_t searchIndex = 0;
    if (index >= _targetList.size()) return nullptr;
    for (auto it = _targetList.begin(); it != _targetList.end(); ++it) {
        if (index == searchIndex) {
            return it->get();
        }
        searchIndex++;
    }
    return nullptr;
}

void WinUsbInterfaceFabric::updateInterfaces() {
    _targetList.clear();
    std::list<std::wstring> devNames = getWinUsbDeviceNameList();
    for (auto devName : devNames) {
        auto winUsbDevice = std::make_shared<WinUsbInterface>(*this, devName);
        _targetList.emplace_back(winUsbDevice);
    }
}

std::list<std::wstring> WinUsbInterfaceFabric::getWinUsbDeviceNameList() {
    static constexpr const size_t MaxStringSize = 2048;
    std::list<std::wstring> result;
    HANDLE devInfoHandle = SetupDiGetClassDevs(&WinUSB_GUID, NULL, NULL, DIGCF_PRESENT);
    if (devInfoHandle == nullptr) return result;
    SP_DEVINFO_DATA deviceInfo;
    SP_INTERFACE_DEVICE_DATA deviceData;
    deviceInfo.cbSize = sizeof(deviceInfo);
    deviceData.cbSize = sizeof(deviceData);
    int usbIfaceIndex = 0;
    while (SetupDiEnumDeviceInfo(devInfoHandle, usbIfaceIndex, &deviceInfo))
    {
        usbIfaceIndex++;
        WCHAR regKeyName[MaxStringSize];
        WCHAR regKeyValue[MaxStringSize];

        HKEY hRegKey = SetupDiOpenDevRegKey(devInfoHandle, &deviceInfo, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE);
        if (hRegKey == INVALID_HANDLE_VALUE) continue;

        for (DWORD regPairIndex = 0; ; regPairIndex++) {
            DWORD nValueNameLen = MaxStringSize;
            DWORD nValueLen = MaxStringSize;
            DWORD dwType = 0;

            LSTATUS keyAccessStatus = RegEnumValueW(hRegKey, regPairIndex,
                regKeyName, &nValueNameLen,
                NULL, &dwType,
                (LPBYTE)regKeyValue, &nValueLen);
            if (keyAccessStatus != 0) break;

            if (wcsncmp(L"SymbolicName", regKeyName, 12) == 0) {
                std::wstring str(regKeyValue);
                result.push_back(str);
            }
        }
        RegCloseKey(hRegKey);
    }
    SetupDiDestroyDeviceInfoList(devInfoHandle);
    return result;
}

WinUsbInterfaceFabric::~WinUsbInterfaceFabric() {
    _targetList.clear();
}
