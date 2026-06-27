PROJECT_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
PROJECT_NAME     = test-ec11

PORT             = /dev/ttyUSB0
BAUD_RATE        = 115200
PROGRAMMER       = arduino
MCU              = atmega328p
F_CPU            = 16000000
BOARD            = nano

#PORT             = /dev/ttyACM0
#BAUD_RATE        = 57600
#PROGRAMMER       = avr109
#MCU              = atmega32u4
#F_CPU            = 16000000
#BOARD            = promicro

TARGET           = $(PROJECT_NAME)

BASE_MAKEFILE    = $(dir $(PROJECT_MAKEFILE))/../Base.mk

include $(BASE_MAKEFILE)
