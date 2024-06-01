##########################################################################################################################
# File automatically-generated by tool: [projectgenerator] version: [3.19.2] date: [Thu Feb 29 00:21:01 MSK 2024] 
##########################################################################################################################

# ------------------------------------------------
# Generic Makefile (based on gcc)
#
# ChangeLog :
#	2017-02-10 - Several enhancements + project update mode
#   2015-07-22 - first version
# ------------------------------------------------

######################################
# target
######################################
TARGET = Firmware


######################################
# building variables
######################################
# debug build?
DEBUG = 1
# optimization
OPT = -O1


#######################################
# paths
#######################################
# Build path
BUILD_DIR = build
GCC_PATH=C:/Tools/arm-gcc/bin

#######################################
# binaries
#######################################
PREFIX = arm-none-eabi-
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
LD = $(GCC_PATH)/$(PREFIX)ld
CPP = $(GCC_PATH)/$(PREFIX)g++
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
LD = $(PREFIX)ld
CPP = $(PREFIX)g++
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S
 
#######################################
# CFLAGS
#######################################
# cpu
CPU = -mcpu=cortex-m3

# fpu
#FPU = -mfpu=fpv4-sp-d16
FPU = 

# float-abi
#FLOAT-ABI = -mfloat-abi=hard
FLOAT-ABI = 

# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# macros for gcc

# ASM defines
ASM_DEFS = 
# C defines
C_DEFS =
# CPP defines
CPP_DEFS =

# AS includes
ASM_INCLUDES = 
# C includes
C_INCLUDES = 
# CPP includes
CPP_INCLUDES = 

# ASM includes
ASM_SOURCES = 
# C sources
C_SOURCES =
# CPP sources
CPP_SOURCES = 

CORE_C_INCLUDES = \
-I. \
-I./libopencm3/include \
-I./App \
-I./Libs/mem/inc

CORE_CPP_INCLUDES = \
-I. \
-I./libopencm3/include \
-I./App \
-I./Libs/mem/inc

CORE_ASM_SOURCES = 

CORE_CPP_SOURCES =

CORE_C_SOURCES = \
libopencm3/lib/cm3/nvic.c \
libopencm3/lib/cm3/systick.c \
libopencm3/lib/cm3/assert.c \
libopencm3/lib/stm32/f1/gpio.c \
libopencm3/lib/stm32/f1/rcc.c \
libopencm3/lib/stm32/f1/timer.c \
libopencm3/lib/stm32/f1/flash.c \
libopencm3/lib/stm32/common/desig_common_v1.c \
libopencm3/lib/stm32/common/flash_common_all.c \
libopencm3/lib/stm32/common/flash_common_f.c \
libopencm3/lib/stm32/common/flash_common_f01.c \
libopencm3/lib/stm32/common/rcc_common_all.c \
libopencm3/lib/stm32/common/gpio_common_all.c \
libopencm3/lib/stm32/common/usart_common_all.c \
libopencm3/lib/stm32/common/usart_common_f124.c \
libopencm3/lib/stm32/common/timer_common_all.c \
libopencm3/lib/stm32/common/st_usbfs_core.c \
libopencm3/lib/stm32/st_usbfs_v2.c \
libopencm3/lib/stm32/can.c \
libopencm3/lib/usb/usb.c \
libopencm3/lib/usb/usb_f107.c \
libopencm3/lib/usb/usb_dwc_common.c \
libopencm3/lib/usb/usb_control.c \
libopencm3/lib/usb/usb_standard.c \

APP_ASM_SOURCES= \
App/STM32F105xx.s \
App/IntLock.s


APP_CPP_SOURCES= \
App/Lin.cpp \
App/Can.cpp \
App/Usb.cpp \
App/Main.cpp \
App/SystemInit.cpp \
App/SystemTimer.cpp \
App/UsbPacketBuffer.cpp



C_INCLUDES   += $(CORE_C_INCLUDES) 
CPP_INCLUDES += $(CORE_CPP_INCLUDES)
C_SOURCES    += $(CORE_C_SOURCES)
CPP_SOURCES  += $(CORE_CPP_SOURCES) $(APP_CPP_SOURCES)
ASM_SOURCES  += $(CORE_ASM_SOURCES) $(APP_ASM_SOURCES)

C_DEFS 		+= -DSTM32F1 -DOTG_FS -D"NVIC_IRQ_COUNT=84"
CPP_DEFS 	+= -DSTM32F1 -DOTG_FS -D"NVIC_IRQ_COUNT=84"

# compile gcc flags
ASMFLAGS = $(MCU) $(ASM_DEFS) $(ASM_INCLUDES) $(OPT) -Wall  -fdata-sections -ffunction-sections
CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections -specs=nosys.specs
CPPFLAGS = $(MCU) $(CPP_DEFS) $(CPP_INCLUDES) $(OPT) -Wall -std=c++17 -fno-exceptions -fno-rtti -Wall -fdata-sections -ffunction-sections
LDFLAGS = -static -nostdlib


CFLAGS		+= -fsingle-precision-constant -Wfloat-conversion
CPPFLAGS    += -fsingle-precision-constant -Wfloat-conversion
# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"
CPPFLAGS+= -MMD -MP -MF"$(@:%.o=%.d)"


ifeq ($(DEBUG), 1)
ASMFLAGS += -g -gdwarf-2
CFLAGS += -g -gdwarf-2
CPPFLAGS += -g -gdwarf-2
endif

#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = STM32F105RBT6_FLASH.ld
LIBDIR = \
-L./Libs/cpp/lib \
-L./Libs/fakelibc/lib \
-L./Libs/fakelibgcc/lib \
-L./Libs/fakelibm/lib \
-L./Libs/math/lib \
-L./Libs/mem/lib \

LIBS = -lmem

LDFLAGS += -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Map=$(BUILD_DIR)/$(TARGET).map

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin


#######################################
# build the application
#######################################
# list of objects
C_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.c.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
ASM_OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.s.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))
# list of C++ program objects
CPP_OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(CPP_SOURCES:.cpp=.cpp.o)))
vpath %.cpp $(sort $(dir $(CPP_SOURCES)))


$(BUILD_DIR)/%.s.o: %.s Makefile | $(BUILD_DIR)
	$(AS) $(ASMFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.s=.lst)) -c $< -o $@

$(BUILD_DIR)/%.c.o: %.c Makefile | $(BUILD_DIR) 
	$(CC) $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) -c $< -o $@

$(BUILD_DIR)/%.cpp.o: %.cpp Makefile | $(BUILD_DIR) 
	$(CPP) $(CPPFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.cpp=.lst)) -c $< -o $@	

$(BUILD_DIR)/$(TARGET).elf: $(ASM_OBJECTS) $(C_OBJECTS) $(CPP_OBJECTS) Makefile
	$(LD) $(OBJECTS) $(ASM_OBJECTS) $(C_OBJECTS) $(CPP_OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@
	
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@	
	
$(BUILD_DIR):
	mkdir $@		

#######################################
# clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)
  
#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)

# *** EOF ***