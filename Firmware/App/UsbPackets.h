/*
 * UsbPackets.h
 *
 *  Created on: May 25, 2024
 *      Author: Evgeny Sobolev. 02/09/1984y.b
 */

#pragma once

#include "Types.h"

enum class UsbPacketId : uint32_t {
	UsbToTimer,
	UsbToLinData,
	UsbToLinRxOnOff,
	UsbToLinBaud,
	UsbToLinTimeout,
	UsbToLinRxSize,
	UsbToLinAnswer,
	LinDataToUsb,
	CanDataToUsb,
	UsbDataToCan
};

enum class LinFrameType : uint8_t {
	BreakAndData,
	Data
};

enum class WaitMode : uint8_t {
	DontWaitTimestamp,
	WaitTimestamp
};

enum class LinRxOnOff : uint8_t {
	Off,
	On
};

enum class LinCrcType : uint8_t {
	None,
	Std1x,
	Std2x
};

struct LinDataToUsbHead {
	UsbPacketId		id = UsbPacketId::LinDataToUsb;
	time_us_t 	 	startTs = 0;
	time_us_t 	 	endTs = 0;
	LinFrameType 	frameType = LinFrameType::BreakAndData;
};

struct UsbToTimerHead {
	UsbPacketId		id = UsbPacketId::UsbToTimer;
	time_us_t		ts = 0;
};

struct UsbToLinDataHead {
	UsbPacketId  	id = UsbPacketId::UsbToLinData;
	time_us_t 	 	ts = 0;
	WaitMode		mode = WaitMode::DontWaitTimestamp;
	LinFrameType 	frameType = LinFrameType::BreakAndData;
};

struct UsbToLinRxOnOffHead {
	UsbPacketId  	id = UsbPacketId::UsbToLinData;
	LinRxOnOff		rx = LinRxOnOff::On;
};


struct UsbToLinAnswerHead {
	UsbPacketId  id = UsbPacketId::UsbToLinAnswer;
	LinCrcType   crc = LinCrcType::None;
	uint8_t		 index = 0;
	uint8_t		 protectedId = 0;
	uint8_t		 size = 0;
};

struct UsbToLinTimeoutHead {
	UsbPacketId  id = UsbPacketId::UsbToLinTimeout;
	time_us_t	 timeout = 2000;
};

struct UsbToLinMaxRxSizeHead {
	UsbPacketId  	id = UsbPacketId::UsbToLinRxSize;
	uint8_t			size = 11;
};


struct UsbToLinBaudHead {
	UsbPacketId  id = UsbPacketId::UsbToLinBaud;
	uint16_t	 baud = 9600;
};


struct CanDataToUsbHead {
	UsbPacketId  id = UsbPacketId::CanDataToUsb;
	uint32_t	 canId = 0;
	time_us_t	 ts = 0;
	uint32_t 	 frId = 0;
	uint8_t 	 ext = false;
	uint8_t 	 rtr = false;
	uint8_t 	 fmi = 0;
	uint8_t		 len = 0;
	uint8_t 	 data[8] = {0};

};

struct UsbDataToCanHead {
	UsbPacketId  id = UsbPacketId::UsbDataToCan;
	uint32_t	 canId = 0;
	time_us_t	 ts = 0;
	uint32_t 	 frId = 0;
	uint8_t 	 ext = false;
	uint8_t 	 rtr = false;
	uint8_t 	 fmi = 0;
	uint8_t		 len = 0;
	uint8_t 	 data[8] = {0};
	WaitMode	 wait;
};


struct UsbHeadSize {
	UsbPacketId 	usbPacketId;
	uint32_t		headSize;
};

constexpr const UsbHeadSize usbPacketHeadTypes[] = {
		{ UsbPacketId::UsbToTimer, sizeof(UsbToTimerHead) },
		{ UsbPacketId::UsbToLinData, sizeof(UsbToLinDataHead) },
		{ UsbPacketId::UsbToLinRxOnOff, sizeof(UsbToLinRxOnOffHead) },
		{ UsbPacketId::UsbToLinBaud, sizeof(UsbToLinBaudHead) },
		{ UsbPacketId::UsbToLinTimeout, sizeof(UsbToLinTimeoutHead) },
		{ UsbPacketId::UsbToLinRxSize, sizeof(UsbToLinMaxRxSizeHead) },
		{ UsbPacketId::UsbToLinAnswer, sizeof(UsbToLinAnswerHead) },
		{ UsbPacketId::LinDataToUsb, sizeof(LinDataToUsbHead) },
		{ UsbPacketId::CanDataToUsb, sizeof(CanDataToUsbHead) },
		{ UsbPacketId::UsbDataToCan, sizeof(UsbDataToCanHead) },
};

inline uint32_t usbPacketHeadSize(const UsbPacketId id) {
	static constexpr const uint32_t TypesCount = sizeof(usbPacketHeadTypes) / sizeof(UsbHeadSize);
	const uint32_t index = static_cast<const uint32_t>(id);
	if (index >= TypesCount) return 0;
	if (usbPacketHeadTypes[index].usbPacketId != id) return 0;
	return usbPacketHeadTypes[index].headSize;
}



