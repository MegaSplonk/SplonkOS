PROJECT_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
PROJECT_NAME     = test-attiny

PORT             = /dev/ttyUSB0
BAUD_RATE        = 115200
PROGRAMMER       = serialupdi
MCU              = attiny1614
F_CPU            = 20000000UL
BOARD            = attiny

TARGET           = $(PROJECT_NAME)

BASE_MAKEFILE    = $(dir $(PROJECT_MAKEFILE))/../Base.mk

# USE_LIB        = 0

include $(BASE_MAKEFILE)
