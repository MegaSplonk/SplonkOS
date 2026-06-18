# Makefile

# TOOLS
CC=avr-gcc
OBJCOPY=avr-objcopy

# FLAGS

WFLAGS=-Wall -Wextra
CFLAGS=--std=c23 -g -Os -mmcu=$(MCU) -ffunction-sections -fdata-sections
CPPFLAGS=-DF_CPU=$(F_CPU) -MMD -I$(INC_DIR)
IFLAGS=
LDFLAGS=-Wl,--gc-sections
AVRDUDE_FLAGS=-c$(PROGRAMMER) -P$(PORT) -b$(BAUD_RATE)

# PROJ OPTIONS
# PORT              = /dev/ttyACM0
# BAUD_RATE         = 57600
# PROGRAMMER        = avr109
# PROJECT_NAME      = Splonk
# PROJECT_FILELIST  = Filelist.$(PROJECT_NAME).mk
# TARGET            = $(PROJECT_NAME)
# SERIAL_PORT      ?= $(PORT)
# SERIAL_BAUD_RATE ?= $(BAUD_RATE)

# DEFS
TIMEOUT          ?= 30
SRC_DIR          ?= src
OBJ_DIR          ?= build/$(PROJECT_NAME)
SERIAL_PORT      ?= $(PORT)
SERIAL_BAUD_RATE ?= $(BAUD_RATE)

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

wait-device:
	@echo "Waiting for $(PORT) to appear (timeout: $(TIMEOUT)s)..."
	@i=0; \
	while [ $$i -lt $(TIMEOUT) ]; do \
		if [ -e "$(PORT)" ]; then \
			echo "$(PORT) found"; \
			exit 0; \
		fi; \
		sleep 1; \
		i=$$((i + 1)); \
	done; \
	echo "ERROR: $(PORT) not found after $(TIMEOUT)s" >&2; \
	exit 1

flash: hex
	$(MAKE) --no-print-directory wait-device
	[ $(MCU) = "atmega32u4" ] && stty -F $(PORT) speed 1200 || true
	avrdude -v -V -D -p $(MCU) -U flash:w:$(HEX_FILE):i $(AVRDUDE_FLAGS)


$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c 
	@mkdir -p $(shell dirname $@)
	$(CC) $(CPPFLAGS) $(WFLAGS) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(ELF_FILE): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ) -o $@

$(HEX_FILE): $(ELF_FILE)
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

filelist:
	@rm -f "$(PROJECT_FILELIST)"
	@printf '%-78s\\\n' "SRC_FILES =" > "$(PROJECT_FILELIST)"
	@find "$(SRC_DIR)" -type f -name '*.c' \
		| sed -e 's|^$(SRC_DIR)/||' -e 's|\.c$$||' \
		| sort \
		| while read f; do printf '%-78s\\\n' "$$f"; done >> "$(PROJECT_FILELIST)"
	@echo "" >> "$(PROJECT_FILELIST)"

serial:
	picocom -b $(SERIAL_BAUD_RATE) $(SERIAL_PORT)

-include $(DEP);

