#pragma once

#include <Windows.h>
#include <winusb.h>
#include <string>
#include <list>
#include <memory>

class WinUsbInterface {
    static constexpr const uint8_t outEndpointId = 0x01;
    static constexpr const uint8_t inEndpointId = 0x81;
    static constexpr const size_t maxWinUsbPacketSize = 64;
    static constexpr const size_t maxStringSize = 128;
public:
    const size_t getMaxPacketSize() { return maxWinUsbPacketSize; };
    WinUsbInterface(class WinUsbInterfaceFabric& fabric, std::wstring deviceName);
    ~WinUsbInterface();
    size_t writePacket(uint8_t* out, size_t size);
    size_t readPacket(uint8_t* in, size_t size);
    int getVendorId();
    int getProductId();
    const char* getSerialNumber();
    const char* getManufacturerName();
    const char* getProductName();
    
private:
    int _vendorId;
    int _productId;
    char _productName[maxStringSize];
    char _manufacturerName[maxStringSize];
    char _serialNumber[maxStringSize];
    class WinUsbInterfaceFabric& _fabric;
    HANDLE _WinUsbDeviceHandle = INVALID_HANDLE_VALUE;
    WINUSB_INTERFACE_HANDLE _WinUsbIfaceHandle{};
};


class WinUsbInterfaceFabric {
public:
    size_t getInterfaceCount();
    WinUsbInterface* getInterface(size_t index);
    void updateInterfaces();
    ~WinUsbInterfaceFabric();
private:
    std::list<std::wstring> getWinUsbDeviceNameList();
    std::list<std::shared_ptr<WinUsbInterface>> _targetList;
};

