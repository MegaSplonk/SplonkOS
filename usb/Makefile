PORT             = /dev/ttyACM0
BAUD_RATE        = 57600
PROGRAMMER       = avr109
PROJECT_NAME     = usb-test
PROJECT_FILELIST = Filelist.$(PROJECT_NAME).mk
MCU              = atmega32u4
F_CPU            = 16000000

OBJ_DIR          = $(shell realpath -m ../build/)/$(PROJECT_NAME)
SRC_DIR          = $(shell realpath -m ./src)
INC_DIR          = $(shell realpath -m ./include)

TARGET           = $(PROJECT_NAME)

include ../Base.mk
