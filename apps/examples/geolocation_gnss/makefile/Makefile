# --- The Clear BSD License ---
# Copyright Semtech Corporation 2021. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted (subject to the limitations in the disclaimer
# below) provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the Semtech corporation nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
# THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
# CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
# NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SEMTECH CORPORATION BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

######################################
# target
######################################
TOP_DIR = ../../../..

APP = geolocation_gnss

APP_TRACE ?= yes
MODEM_TRACE ?= yes
MCU_BOARD ?= NUCLEO_L476RG
RADIO_BOARD ?= LR1110MB1DJS
MIDDLEWARE ?= yes
MIDDLEWARE_TRACE ?= yes

TOOLCHAIN_MAKEFILE = $(TOP_DIR)/toolchain/gcc/toolchain.mk
PROJECTS_COMMON_MAKEFILE = $(TOP_DIR)/apps/common/apps_modem_common.mk
SHIELD_MAKEFILE = $(TOP_DIR)/shields/radio_shield.mk

######################################
# building variables
######################################
# debug build?
DEBUG ?= yes
# optimization
OPT ?= -O0

#######################################
# paths
#######################################

# Build path
BUILD_DIR = ./build

######################################
# source
######################################

# C sources

C_SOURCES +=  \
../main_$(APP).c \
$(TOP_DIR)/geolocation_middleware/common/mw_common.c \
$(TOP_DIR)/geolocation_middleware/gnss/src/gnss_helpers.c \
$(TOP_DIR)/geolocation_middleware/gnss/src/gnss_queue.c \
$(TOP_DIR)/geolocation_middleware/gnss/src/gnss_middleware.c \

# C includes
C_INCLUDES += \
-I$(TOP_DIR)/geolocation_middleware/bsp \
-I$(TOP_DIR)/geolocation_middleware/common  \
-I$(TOP_DIR)/geolocation_middleware/gnss/src

# Add the macro debug trace definition
ifeq ($(MIDDLEWARE_TRACE),yes)
C_DEFS += \
    -DMW_DBG_TRACE=1
endif

ifeq ($(APP_TRACE),no)
C_DEFS += \
    -DMW_DBG_TRACE=0
endif

#######################################
# include
#######################################

# LoRa Basics Modem
LORA_BASICS_MODEM = $(TOP_DIR)/lora_basics_modem/lora_basics_modem

include $(PROJECTS_COMMON_MAKEFILE)
include $(SHIELD_MAKEFILE)
include $(TOOLCHAIN_MAKEFILE)

#######################################
# build the application
#######################################

.PHONY: all target

all: target

target: $(BUILD_DIR)/$(APP).elf $(BUILD_DIR)/$(APP).bin

.DEFAULT_GOAL:= target

## For the main application
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile $(PLATFORM_MAKEFILE) | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile $(PLATFORM_MAKEFILE) | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(APP).elf: $(OBJECTS) $(LBM_LIB) Makefile | $(BUILD_DIR)
	$(CC) $(OBJECTS) $(LBM_LIB) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

$(BUILD_DIR):
	mkdir $@

print-%  : ; @echo $* = $($*)

#######################################
# clean up
#######################################
clean: clean_lbm
	-rm -fR $(BUILD_DIR)

#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)

# *** EOF ***
