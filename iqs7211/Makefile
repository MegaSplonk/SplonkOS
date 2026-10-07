PROJECT_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
PROJECT_NAME     = test-iqs7211e
PROJECT_USE_CPP  = y

PORT             = /dev/ttyACM0
BAUD_RATE        = 57600
PROGRAMMER       = avr109
MCU              = atmega32u4
F_CPU            = 16000000
BOARD            = iqs7211e_test
PINOUT           = promicro

TARGET           = $(PROJECT_NAME)

BASE_MAKEFILE    = $(dir $(PROJECT_MAKEFILE))/../Base.mk

include $(BASE_MAKEFILE)


viewer:
	picocom -b $(SERIAL_BAUD_RATE) $(SERIAL_PORT) | tee /dev/tty /dev/stdout | python3 viewer.py

WARNING_REMOVE = -Wno-inline -Wno-missing-declarations -Wno-conversion \
				 -Wno-shadow -Wno-sign-conversion -Wno-unused-parameter \
				 -Wno-cast-qual -Wno-switch -Wno-missing-braces
CXXFLAGS_ADD   = $(WARNING_REMOVE) -Wno-volatile
CFLAGS_ADD     = $(WARNING_REMOVE)
