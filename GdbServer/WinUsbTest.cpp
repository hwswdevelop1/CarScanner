// WinUsbTest.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include "UsbInterface/WinUsbInterface.h"






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


            std::cout << "Sending Lin Packets" << std::endl;
            
            for (int i = 0; i < 10000; i++) {
                uint8_t linOutPacket[32] = { 0x00, 0x00, 0x55, 0x2A, 0x00, i, 0x00 };
                uint8_t linInPacket[32] = { 0x00 };

                const size_t txSize = t->writePacket((uint8_t*)&linOutPacket[0], 4);
                std::cout << std::endl;
                std::cout << i << " Sent: " << txSize << std::endl;
                std::cout << "Data:";
                for (int ind = 0; ind < txSize; ind++) {
                    std::cout << " " << ((int)linOutPacket[ind]);
                }
                std::cout << std::endl;

                //Sleep(1);

                const size_t rxSize = t->readPacket((uint8_t*)&linInPacket[0], 64);
                std::cout << i << " Recv: " << rxSize << std::endl;
                std::cout << "Data:";
                for (int ind = 0; ind < rxSize; ind++) {
                    std::cout << " " << (int)(linInPacket[ind]);
                }
                std::cout << std::endl;

                const size_t rxSize1 = t->readPacket((uint8_t*)&linInPacket[0], 64);
                std::cout << i << " Recv: " << rxSize1 << std::endl;
                std::cout << "Data:";
                for (int ind = 0; ind < rxSize1; ind++) {
                    std::cout << " " << (int)(linInPacket[ind]);
                }
                std::cout << std::endl;

            }
            
        }

    }
    else {
        std::cout << "No WinUSB interfaces found" << std::endl;
    }
    return  0;
}
