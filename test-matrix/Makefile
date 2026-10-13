PROJECT_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
PROJECT_NAME     = test-matrix

PORT             = /dev/ttyUSB0
BAUD_RATE        = 57600
PROGRAMMER       = serialupdi
MCU              = attiny1614
F_CPU            = 20000000
PINOUT           = attiny
BOARD            = matrix_test

TARGET           = $(PROJECT_NAME)

BASE_MAKEFILE    = $(dir $(PROJECT_MAKEFILE))/../Base.mk

include $(BASE_MAKEFILE)
