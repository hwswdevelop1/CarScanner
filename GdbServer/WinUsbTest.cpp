// WinUsbTest.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <stdio.h>
#include <iostream>
#include "UsbInterface/WinUsbInterface.h"


#include "processthreadsapi.h"


struct ThreadParams {
    WinUsbInterface*    iface;
    volatile bool threadActive;
};



DWORD WINAPI recvThread(LPVOID params) {
    ThreadParams* p = (ThreadParams*)params;
    if (nullptr == p) return 0;
    auto t = p->iface;
    const size_t maxPacketSize = t->getMaxPacketSize();
    uint8_t* buffer = new uint8_t[maxPacketSize];
    const size_t maxStrSize = maxPacketSize * 3 + 32;
    char* str = new char[maxStrSize];
    size_t offs = 0;
    while (p->threadActive) {
        size_t rxSize = 0;
        try {
            rxSize = t->readPacket(buffer, maxPacketSize);
        } catch(...) {
            rxSize = 0;
        };
        offs = snprintf( &str[0], maxStrSize, "Recv (%02d):", (int)rxSize);
        for ( size_t index = 0; index < rxSize; index++) {
            offs += snprintf( &str[offs], maxStrSize, " %02x", buffer[index]);
        }
        str[offs] = 0;
        printf("%s\r\n\r", str);
    }
    delete[] str;
    delete[] buffer;
    return 0;
}

enum class LinFrameType : uint8_t {
    LinBreak,
    LinNoBreak,
    LinSetBaud,
    LinSetTimeout,
    LinSetAutoAnswer
};

enum class ModuleId : uint8_t {
    Lin1,
    Can1,
    Can2
};

struct UsbPacketHead {
    ModuleId 	 id;
    union ModuleSpecific {
        LinFrameType	lin;
    } type;
};

VOID LinSetBaud( WinUsbInterface* iface, ModuleId linModuleId, DWORD baud ) {
    static constexpr const size_t BaudSize = 2;
    static constexpr const size_t PacketSize = sizeof(UsbPacketHead) + BaudSize;
    uint8_t buffer[PacketSize] = {0};
    UsbPacketHead* head = reinterpret_cast<UsbPacketHead*>( & buffer[0] );
    head->id = linModuleId;
    head->type.lin = LinFrameType::LinSetBaud;
    const size_t offset = sizeof(UsbPacketHead);
    buffer[offset + 0] = baud & 0xFF;
    buffer[offset + 1] = (baud >> 8) & 0xFF;
    iface->writePacket(buffer, PacketSize);
}

VOID LinSetTimeout(WinUsbInterface* iface, ModuleId linModuleId, DWORD timeout) {
    static constexpr const size_t TimeoutSize = 2;
    static constexpr const size_t PacketSize = sizeof(UsbPacketHead) + TimeoutSize;
    uint8_t buffer[PacketSize] = { 0 };
    UsbPacketHead* head = reinterpret_cast<UsbPacketHead*>(&buffer[0]);
    head->id = linModuleId;
    head->type.lin = LinFrameType::LinSetTimeout;
    const size_t offset = sizeof(UsbPacketHead);
    buffer[offset + 0] = timeout & 0xFF;
    buffer[offset + 1] = (timeout >> 8) & 0xFF;
    iface->writePacket(buffer, PacketSize);
}

VOID LinSetAutoAnswer(WinUsbInterface* iface, ModuleId linModuleId, const uint8_t index, const uint8_t id, const uint8_t size, uint8_t* const data) {
    static constexpr const size_t PayloadSize = 12;
    static constexpr const size_t PacketSize = sizeof(UsbPacketHead) + PayloadSize;
    if ( size > 9 ) return;
    if ( index > 8 ) return;
    uint8_t buffer[PacketSize] = { 0 };
    UsbPacketHead* head = reinterpret_cast<UsbPacketHead*>(&buffer[0]);
    head->id = linModuleId;
    head->type.lin = LinFrameType::LinSetAutoAnswer;
    uint8_t* ptr = &buffer[sizeof(UsbPacketHead)];
    *ptr++ = index;
    *ptr++ = id;
    *ptr++ = size;
    for (size_t ind = 0; ind < size; ind++) {
        *ptr++ = data[ind];
    }
    iface->writePacket( buffer, PacketSize );
}


int main()
{   
    WinUsbInterfaceFabric targetFabric;
    targetFabric.updateInterfaces();
    const size_t count = targetFabric.getInterfaceCount();

    if (count > 0) {

        std::cout << "WinUSB debug interface(s) found" << std::endl;
        std::cout << "Count: " << count << std::endl << std::endl;

        for (int i = 0; i < count; i++) {
            auto t = targetFabric.getInterface(0);
            std::cout << (i + 1) << ". "
                << "VID:" << t->getVendorId()
                << " PID:" << t->getProductId()
                << " NAME:\"" << t->getProductName() << "\""
                << " VENDOR:\"" << t->getManufacturerName() << "\""
                << " SERIAL:\"" << t->getSerialNumber() << "\"" << std::endl;



            ThreadParams params;

            params.iface = t;
            params.threadActive = true;

            DWORD ThreadId;
            HANDLE hThread = CreateThread( NULL, 0, recvThread, (LPVOID)&params, 0, &ThreadId );

            std::cout << "Sending Lin Packets" << std::endl;

            const size_t maxPacketSize = t->getMaxPacketSize();
            const size_t maxStrSize = maxPacketSize * 3 + 32;
            char* str = new char[maxStrSize];


            uint8_t answer[9] = { 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 };
            LinSetAutoAnswer(t, ModuleId::Lin1, 0, 0x2a, 9, answer);
            answer[8] = 0xFF;
            LinSetAutoAnswer(t, ModuleId::Lin1, 1, 0x1a, 9, answer);

            for (int i = 0; i < 32; i++) {
                
                LinSetTimeout( t, ModuleId::Lin1, 2000 * (1 + (i & 0x03)) );
                uint8_t linOutPacket[32] = { 0x00, 0x00, 0x55, 0x1A, 0x00, i, 0x00 };
                const size_t txSize = t->writePacket((uint8_t*)&linOutPacket[0], 4);  
                size_t offs = snprintf(&str[0], maxStrSize, "Send (%02d):", (int)txSize);
                for (size_t index = 0; index < txSize; index++) {
                    offs += snprintf(&str[offs], maxStrSize, " %02x", linOutPacket[index]);
                }
                str[offs] = 0;
                printf("%s\r\n\r", str);
                Sleep(50);
            }
            delete[] str;
            Sleep(500);
            params.threadActive = false;
            Sleep(10);
            CloseHandle(hThread);
        }

    }
    else {
        std::cout << "No WinUSB interfaces found" << std::endl;
    }
    return  0;
}
