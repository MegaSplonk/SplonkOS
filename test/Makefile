# Makefile
MCU=atmega328p
F_CPU=16000000
CC=avr-gcc
OBJCOPY=avr-objcopy
WFLAGS=-Wall -Wextra
CFLAGS=--std=c23 -g -Os -mmcu=$(MCU) -ffunction-sections -fdata-sections
CPPFLAGS=-DF_CPU=$(F_CPU) -MMD -Iinclude
IFLAGS=
LDFLAGS=-Wl,--gc-sections
TARGET=main
SERIAL=-P /dev/ttyUSB0 -b 115200


SRC_DIR=src
OBJ_DIR=build

PROJECT_FILELIST = Filelist.mk
-include $(PROJECT_FILELIST)

SRC=$(addsuffix .c,$(addprefix $(SRC_DIR)/,$(SRC_FILES)))
OBJ=$(addsuffix .o,$(addprefix $(OBJ_DIR)/,$(SRC_FILES)))
DEP=$(addsuffix .d,$(addprefix $(OBJ_DIR)/,$(SRC_FILES)))

ELF_FILE=$(OBJ_DIR)/$(TARGET).elf
HEX_FILE=$(OBJ_DIR)/$(TARGET).hex

all: flash

re: fclean all

fclean: clean
	rm -f $(HEX_FILE)
	rm -f $(ELF_FILE)
clean:
	rm -rf $(OBJ_DIR)

hex: $(HEX_FILE)

flash: hex
	avrdude -p $(MCU) -c arduino -U flash:w:$(HEX_FILE):i $(SERIAL) 


$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c 
	@mkdir -p $(shell dirname $@)
	$(CC) $(CPPFLAGS) $(WFLAGS) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(ELF_FILE): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ) -o $@

$(HEX_FILE): $(ELF_FILE)
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

print:
	@echo $(SRC)
	@echo $(OBJ)
	@echo $(ELF_FILE)
	@echo $(HEX_FILE)

filelist:
	@rm -f "$(PROJECT_FILELIST)"
	@printf '%-78s\\\n' "SRC_FILES =" > "$(PROJECT_FILELIST)"
	@find "$(SRC_DIR)" -type f -name '*.c' \
		| sed -e 's|^$(SRC_DIR)/||' -e 's|\.c$$||' \
		| sort \
		| while read f; do printf '%-78s\\\n' "$$f"; done >> "$(PROJECT_FILELIST)"
	@echo "" >> "$(PROJECT_FILELIST)"

-include $(DEP);

