PROJECT_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
PROJECT_NAME     = test-usb

PORT             = /dev/ttyACM0
BAUD_RATE        = 57600
PROGRAMMER       = avr109
MCU              = atmega32u4
F_CPU            = 16000000
BOARD            = promicro

SERIAL_PORT      = /dev/ttyUSB0
SERIAL_BAUD_RATE = 57600

TARGET           = $(PROJECT_NAME)

BASE_MAKEFILE    = $(dir $(PROJECT_MAKEFILE))/../Base.mk

include $(BASE_MAKEFILE)
