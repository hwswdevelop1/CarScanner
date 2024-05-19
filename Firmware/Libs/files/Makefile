######################################################
# Generic Makefile (based on gcc)
# ----------------------------------------------------

######################################
# target
######################################
TARGET = libfiles

######################################
# building variables
######################################
# debug build?
DEBUG = 0
# optimization
OPT = -O0


#######################################
# paths
#######################################
# Build path
BUILD_DIR = lib
GCC_PATH=C:\Tools/arm-gcc/bin

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
AR = $(GCC_PATH)/$(PREFIX)ar
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
LD = $(PREFIX)ld
CPP = $(PREFIX)g++
AR = $(PREFIX)ar
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S
 
#######################################
# CFLAGS
#######################################
# cpu
CPU = -mcpu=cortex-m3

#fpu
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
C_DEFS =
CPP_DEFS = 


ASM_INCLUDES = 
C_INCLUDES =  \
-I./inc
CPP_INCLUDES =  \
-I./inc

ASM_SOURCES =
C_SOURCES =  
CPP_SOURCES = \
src/files.cpp


# compile gcc flags
ASM_FLAGS = $(MCU) $(ASM_DEFS) $(ASM_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections
C_FLAGS += $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections
CPP_FLAGS += $(MCU) $(CPP_DEFS) $(CPP_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections -fno-exceptions -fno-rtti

LDFLAGS= -nostdlib

ifeq ($(DEBUG), 1)
C_FLAGS += $(OPT)
CPP_FLAGS += $(OPT)
endif

# default action: build all
all: $(BUILD_DIR) $(BUILD_DIR)/$(TARGET).a

#######################################
# build the application
#######################################
# list of objects
C_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.c.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of C++ program objects
CPP_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(CPP_SOURCES:.cpp=.cpp.o)))
vpath %.cpp $(sort $(dir $(CPP_SOURCES)))
# list of ASM program objects
ASM_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.asm.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.c.o: %.c Makefile | $(BUILD_DIR)
	$(CC) $(C_FLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) -c  $< -o $@
	
$(BUILD_DIR)/%.cpp.o: %.cpp Makefile | $(BUILD_DIR)
	$(CPP) $(CPP_FLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.cpp=.lst)) -c $< -o $@

$(BUILD_DIR)/%.asm.o: %.s Makefile | $(BUILD_DIR)
	$(AS) $(ASM_FLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.s=.lst)) -c $< -o $@	

$(BUILD_DIR)/$(TARGET).a: $(ASM_OBJECTS) $(C_OBJECTS) $(CPP_OBJECTS) Makefile
	$(AR) -r $@ $(ASM_OBJECTS) $(C_OBJECTS) $(CPP_OBJECTS)
	$(SZ) $@

$(BUILD_DIR):
	mkdir $@

#######################################
# clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)
  
