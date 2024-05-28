// WinUsbTest.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <stdio.h>
#include <iostream>
#include "UsbInterface/WinUsbInterface.h"


#include "processthreadsapi.h"
#include "UsbPackets.h"

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
   

    const size_t maxStrSize = maxPacketSize * 3 + 64;
    char* str = new char[maxStrSize];
    size_t offs = 0;
    uint32_t frameNo = 0;
    while (p->threadActive) {
        size_t rxSize = 0;
        try {
            rxSize = t->readPacket(buffer, maxPacketSize);
        } catch(...) {
            rxSize = 0;
        };
        if (0 == rxSize) break;

        volatile LinDataToUsbHead* const linFrameHead = reinterpret_cast<LinDataToUsbHead*>(&buffer[0]);
        volatile CanDataToUsbHead* const canFrameHead = reinterpret_cast<CanDataToUsbHead*>(&buffer[0]);

        size_t headSize = usbPacketHeadSize(linFrameHead->id);
        size_t dataSize = rxSize - headSize;
        if ( UsbPacketId::LinDataToUsb == linFrameHead->id ) {
            offs = snprintf(&str[0], maxStrSize, "%4d. LIN RX (Ts: %10u uS, Dur: %6d uS, Size: %2d byte(s):", frameNo, linFrameHead->startTs, (linFrameHead->endTs - linFrameHead->startTs), dataSize );
            for (size_t index = usbPacketHeadSize(linFrameHead->id); index < rxSize; index++) {
                offs += snprintf(&str[offs], maxStrSize, " %02x", buffer[index]);
            }
        }
        if ( UsbPacketId::CanDataToUsb == canFrameHead->id ) {
            offs = snprintf(&str[0], maxStrSize, "%4d. Can: (Ts: %10u uS): ", frameNo, canFrameHead->ts);
            offs += snprintf(&str[offs], maxStrSize, "ext=%1d, id=0x%08X, rtr=%1d, ", canFrameHead->ext, canFrameHead->frId, canFrameHead->rtr);
            offs += snprintf(&str[offs], maxStrSize, "len: %1d", canFrameHead->len);
            for (size_t index = 0; index < canFrameHead->len; index++) {
                offs += snprintf(&str[offs], maxStrSize, " %02x", canFrameHead->data[index]);
            }
        }

        str[offs] = 0;

        if ( ( 0 == (frameNo % 1000) ) || true ) {
            printf("%s\r\n\r", str);
        }
        frameNo++;

    }
    delete[] str;
    delete[] buffer;
    return 0;
}


void setTimestamp( WinUsbInterface* const iface, const time_us_t ts ) {
    UsbToTimerHead frame;
    frame.id = UsbPacketId::UsbToTimer;
    frame.ts = ts;
    uint8_t* const buf = reinterpret_cast<uint8_t* const>(&frame);
    const size_t sendSize = sizeof(frame);
    iface->writePacket( buf, sendSize);
}

void linSetRxEn(WinUsbInterface* const iface, bool enable ) {
    UsbToLinRxOnOffHead frame;
    frame.id = UsbPacketId::UsbToLinRxOnOff;
    frame.rx = (enable) ? LinRxOnOff::On : LinRxOnOff::Off;
    uint8_t* const buf = reinterpret_cast<uint8_t* const>(&frame);
    const size_t sendSize = sizeof(frame);
    iface->writePacket(buf, sendSize);
}

void linSetRxSize(WinUsbInterface* const iface, uint8_t size) {
    UsbToLinMaxRxSizeHead frame;
    frame.size = size;
    uint8_t* const buf = reinterpret_cast<uint8_t* const>(&frame);
    const size_t sendSize = sizeof(frame);
    iface->writePacket(buf, sendSize);
}

void linSendFrame( WinUsbInterface* const iface, const bool waitTs, const time_us_t ts, bool sendBreak, const uint8_t* const data, const size_t size ) {
    static constexpr const size_t LinMaxFrameSize = 2 + 9;
    struct Frame {
        UsbToLinDataHead head;
        uint8_t          data[LinMaxFrameSize];
    } ;

    Frame frame;
    frame.head.id = UsbPacketId::UsbToLinData;
    frame.head.mode = (waitTs) ? WaitMode::WaitTimestamp : WaitMode::DontWaitTimestamp;
    frame.head.frameType = (sendBreak) ? LinFrameType::BreakAndData : LinFrameType::Data;
    frame.head.ts = ts;

    const size_t copySize = (LinMaxFrameSize < size) ? LinMaxFrameSize : size;
    memcpy(&frame.data[0], data, copySize);
    uint8_t* const buf = reinterpret_cast<uint8_t* const>(&frame);
    const size_t sendSize = copySize + sizeof(UsbToLinDataHead);
    iface->writePacket( buf, sendSize );
}

void linSetBaud(WinUsbInterface* const iface, uint16_t baud) {
    UsbToLinBaudHead frame;
    frame.baud = baud;
    uint8_t* const buf = reinterpret_cast<uint8_t* const>(&frame);
    const size_t sendSize = sizeof(frame);
    iface->writePacket(buf, sendSize);
}

void linSetRxTimeout(WinUsbInterface* const iface, time_us_t timeout ) {
    UsbToLinTimeoutHead frame;
    frame.timeout = timeout;
    uint8_t* const buf = reinterpret_cast<uint8_t* const>(&frame);
    const size_t sendSize = sizeof(frame);
    iface->writePacket(buf, sendSize);
}

void linSetAnswer(WinUsbInterface* const iface,
    const uint8_t index,
    const uint8_t protectedId,
    uint8_t* const data,
    uint8_t* const increment,
    const size_t size,
    const LinCrcType crcType ) {

    struct Frame {
        UsbToLinAnswerHead head;
        uint8_t data[18];
    };
    Frame frame;

    frame.head.index = index;
    frame.head.protectedId = protectedId;
    frame.head.size = size;
    frame.head.crc = crcType;
    memcpy( &(frame.data[0]), data, size );
    memcpy( &(frame.data[size]), increment, size );
   
    uint8_t* const buf = reinterpret_cast<uint8_t* const>(&frame);
    const size_t sendSize = (size * 2) + sizeof(UsbToLinAnswerHead);
    iface->writePacket(buf, sendSize);
}

void canSendFrame( WinUsbInterface* const iface,                  
                   const bool waitTs, 
                   const time_us_t ts,
                   const uint8_t canId,
                   const uint32_t frameId,
                   const bool rtr,
                   const uint8_t  len,
                   uint8_t* const data
                    ) {
    UsbDataToCanHead frame;
    frame.canId = canId;
    frame.frId = frameId;
    frame.len = len;
    frame.rtr = rtr;
    frame.wait = (waitTs) ? WaitMode::WaitTimestamp : WaitMode::DontWaitTimestamp;
    frame.ts = ts;

    memcpy( &(frame.data[0]), data, len );

    uint8_t* const buf = reinterpret_cast<uint8_t* const>(&frame);
    const size_t sendSize =  sizeof(UsbDataToCanHead);
    iface->writePacket(buf, sendSize);
}


uint8_t calcLinId(const uint8_t id) {
    const uint8_t newId = id & 0x3F;
    const uint8_t p0 = (id ^ (id >> 1) ^ (id >> 2) ^ (id >> 4)) & 0x1;
    const uint8_t p1 = ~(((id >> 1) ^ (id >> 3) ^ (id >> 4) ^ (id >> 5))) & 0x1;
    const uint8_t resId = (p1 << 7) | (p0 << 6) | newId;
    return resId;
}

int main(int argc, char* argv[] ) {   
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
            HANDLE hThread = CreateThread(NULL, 0, recvThread, (LPVOID)&params, 0, &ThreadId);
            SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);



            //setTimestamp(t, 0x70000000);
            //Sleep(100);

            setTimestamp(t, 0);
            linSetBaud(t, 10000);
            linSetRxTimeout(t, 2000);
            linSetRxSize(t, 16);

            uint8_t answer[9] = { 0x55, 0x93, 0xE5 ,0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };
            uint8_t incr[9] = { 0x01 , 0x02, 0x00 ,0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

            linSetAnswer(t, 0, 0x03, answer, incr, 9, LinCrcType::Std2x);

            linSetRxEn(t, true);

            std::cout << "Sending Lin Packets" << std::endl;

            setTimestamp(t, 0);
            time_us_t txTime = 1000;

            uint32_t count = 0;


#if 0
            for (int num = 0; num < 2000; num++) {

                for (int i = 2; i <= 4; i++) {
                    uint8_t protectedId = calcLinId(i);
                    uint8_t linOutPacket[32] = { 0x55, protectedId, 0x80, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00 };
                    txTime += 20000;
                    linSendFrame(t, true, txTime, true, linOutPacket, 2);
                    //std::cout << count << std::endl;
                    count++;
        }
}

            Sleep(2000);
#else

#if 0
            for (int num = 0; num < 20000; num++) {
                uint8_t canOutPacket[8] = { 0x80, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, num };
                canSendFrame(t, true, txTime, (num & 0x01),0xAAA, false, 8, canOutPacket);
                txTime += 1000;
            
            }
#endif

#endif
            Sleep(200000);

            params.threadActive = false;
            Sleep(100);
            CloseHandle(hThread);
           

        }

    }
    else {
        std::cout << "No WinUSB interfaces found" << std::endl;
    }
    return  0;
}
