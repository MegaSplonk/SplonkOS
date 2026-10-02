PROJECT_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
PROJECT_NAME     = test-matrix

PORT             = /dev/ttyUSB0
BAUD_RATE        = 115200
PROGRAMMER       = arduino
MCU              = atmega328p
F_CPU            = 16000000
BOARD            = rgb_test
PINOUT           = nano

TARGET           = $(PROJECT_NAME)

BASE_MAKEFILE    = $(dir $(PROJECT_MAKEFILE))/../Base.mk

include $(BASE_MAKEFILE)
