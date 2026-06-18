PORT             = /dev/ttyUSB0
BAUD_RATE        = 115200
PROGRAMMER       = arduino
PROJECT_NAME     = test
PROJECT_FILELIST = Filelist.$(PROJECT_NAME).mk
MCU              = atmega328p
F_CPU            = 16000000

OBJ_DIR          = $(shell realpath -m ../build/)/$(PROJECT_NAME)
SRC_DIR          = $(shell realpath -m ./src)
INC_DIR          = $(shell realpath -m ./include)

TARGET           = $(PROJECT_NAME)

include ../Base.mk
