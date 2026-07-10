check-var-defined      = $(if $(strip $($1)),,$(error "$1" is not defined))
relpath                = $(shell 	realpath -m --relative-to="$(or $(2),$(CURDIR))" "$(1)")

ifeq ($(USE_ABSOLUTE_PATH),1)
relpath                = $(abspath $(1))
override SOURCE_PREFIX =
endif

BASE_MAKEFILE    := $(call relpath,$(abspath $(lastword $(MAKEFILE_LIST))))
BASE_DIRECTORY   := $(call relpath,$(dir $(BASE_MAKEFILE)))

MAKEFLAGS += --no-builtin-rules

# Makefile

# TOOLS
CC            = avr-gcc
OBJCOPY       = avr-objcopy
AR            = avr-gcc-ar

# FLAGS
WERROR       ?= 
WFLAGS        = $(WERROR) -Wenum-conversion -Wimplicit-fallthrough -Wpedantic \
                -Wextra -Wall -Wshadow -Wpadded -Wconversion -Wstrict-prototypes \
                -Wmissing-declarations -Wstrict-prototypes -Wundef \
                -Wmissing-prototypes -Wold-style-definition -Winline \
                -Wsign-conversion -Wcast-align -Wcast-qual -Wwrite-strings \
                -Wuninitialized -Wdouble-promotion -Wfloat-equal \
                -Wnull-dereference -Wformat=2
CFLAGS        = --std=c23 -g -Os -mmcu=$(MCU) -ffunction-sections -fdata-sections -flto
CPPFLAGS      = -include$(BASE_DIRECTORY)/lib/include/lib/config.h \
                -include$(BASE_DIRECTORY)/lib/include/lib/lib.h \
                -DF_CPU=$(F_CPU) -MMD -DMCU_$(MCU)=1 -DBOARD_$(BOARD)=1
IFLAGS        = -I$(INC_DIR)
LDFLAGS       = -Wl,--gc-sections -Wl,-flto
AVRDUDE_FLAGS = -c$(PROGRAMMER) -P$(PORT) -b$(BAUD_RATE)

# PROJ OPTIONS

# BASE_MAKEFILE    := $(abspath $(lastword $(MAKEFILE_LIST)))
# PORT              = /dev/ttyACM0
# BAUD_RATE         = 57600
# PROGRAMMER        = avr109
# MCU               = atemega32u4
# BOARD             = promicro

$(call check-var-defined,PROJECT_MAKEFILE)
$(call check-var-defined,PORT)
$(call check-var-defined,BAUD_RATE)
$(call check-var-defined,PROGRAMMER)
# $(call check-var-defined,MCU)
# $(call check-var-defined,BOARD)

# DEFAULT PROJECT OPTIONS

PROJECT_ROOT       ?= $(call relpath,$(dir $(PROJECT_MAKEFILE)))
PROJECT_NAME       ?= $(basename $(dir $(PROJECT_ROOT)))
PROJECT_FILELIST   ?= $(call relpath,$(PROJECT_ROOT)/Filelist.mk)
TARGET             ?= $(PROJECT_NAME)
SERIAL_PORT        ?= $(PORT)
SERIAL_BAUD_RATE   ?= $(BAUD_RATE)

# DEFS
TIMEOUT          ?= 30
SERIAL_PORT      ?= $(PORT)
SERIAL_BAUD_RATE ?= $(BAUD_RATE)

OBJ_DIR_BASE     ?= $(BASE_DIRECTORY)/build/
SRC_DIR          ?= $(call relpath,$(PROJECT_ROOT)/src)
INC_DIR          ?= $(call relpath,$(PROJECT_ROOT)/include)

USE_LIB          ?= 1
OUTPUT_ARCHIVE   ?= 0
SOURCE_PREFIX    ?=

override OBJ_DIR_BASE := $(call relpath,$(OBJ_DIR_BASE))


-include $(PROJECT_FILELIST)

OBJ_DIR = $(OBJ_DIR_BASE)/$(PROJECT_NAME)

SRC=$(addsuffix .c,$(addprefix $(SRC_DIR)/,$(SRC_FILES)))
OBJ=$(addsuffix .o,$(addprefix $(OBJ_DIR)/,$(SRC_FILES)))
DEP=$(addsuffix .d,$(addprefix $(OBJ_DIR)/,$(SRC_FILES)))

ELF_FILE     = $(OBJ_DIR)/$(TARGET).elf
HEX_FILE     = $(OBJ_DIR)/$(TARGET).hex
ARCHIVE_FILE = $(OBJ_DIR)/lib$(TARGET).a

.DEFAULT_GOAL = all

ifeq ($(USE_LIB),1)
IFLAGS += -I$(call relpath,$(BASE_DIRECTORY)/lib/include)
endif
ifeq ($(USE_LIB),1)
LDFLAGS += -L$(call relpath,$(OBJ_DIR)/lib/) -lsplonk
endif


ifeq ($(OUTPUT_ARCHIVE), 0)
all: flash
else
all: archive
endif

.SUFFIXES:
.PHONY: re
re: fclean all

.PHONY: fclean
fclean: clean
	@printf " RM $(OBJ_DIR_BASE)\n"
	@rm -rf $(OBJ_DIR_BASE)
.PHONY: clean
clean:
	@printf " RM $(OBJ_DIR)\n"
	@rm -rf $(OBJ_DIR)

hex: $(HEX_FILE)
archive: $(ARCHIVE_FILE)

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

.PHONY: flash
flash: hex
	@$(MAKE) --no-print-directory wait-device
ifeq ($(MCU),atmega32u4)
	@printf " STTY $(PORT) 1200\n"
	@stty -F $(PORT) speed 1200 >/dev/null
endif
	@printf " FLASH[$(MCU)] $(HEX_FILE)\n"
	@avrdude -V -D -p $(MCU) -U flash:w:$(HEX_FILE):i $(AVRDUDE_FLAGS)


$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c 
	@mkdir -p $(shell dirname $@)
	@printf " CC $(SOURCE_PREFIX)$<\n"
	@$(CC) $(CPPFLAGS) $(WFLAGS) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(ELF_FILE): $(OBJ) $(if $(filter 1,$(USE_LIB)), $(OBJ_DIR)/lib/libsplonk.a)
	@printf " CCLD $@ $(if $(filter 1,$(USE_LIB)),[With Lib])\n"
	@$(CC) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $@


$(HEX_FILE): $(ELF_FILE)
	@printf " OBJCOPY $@\n"
	@$(OBJCOPY) -O ihex $< $@

.PHONY: filelist
filelist:
	@rm -f "$(PROJECT_FILELIST)"
	@printf '%-78s\\\n' "SRC_FILES =" > "$(PROJECT_FILELIST)"
	@find "$(SRC_DIR)" -type f -name '*.c' \
		| sed -e 's|^$(SRC_DIR)/||' -e 's|\.c$$||' \
		| sort \
		| while read f; do printf '%-78s\\\n' "$$f"; done >> "$(PROJECT_FILELIST)"
	@echo "" >> "$(PROJECT_FILELIST)"
	@printf "Filelist: $(PROJECT_NAME)\n"
ifeq ($(USE_LIB),1)
	@$(MAKE) --no-print-directory -C $(BASE_DIRECTORY)/lib filelist
endif

.PHONY: serial
serial:
	@picocom -b $(SERIAL_BAUD_RATE) $(SERIAL_PORT)

EXCLUDE := \
	.DEFAULT_GOAL \
	CURDIR \
	EXCLUDE \
	MAKEFILE_LIST \
	MAKEFLAGS \
	SHELL \
	check-var-defined \
	relpath \


.PHONY: .vars
.vars:
	@($(foreach v,$(.VARIABLES),\
	  $(if $(filter-out $(EXCLUDE),$(v)),\
	  $(if $(filter file command\ line,$(origin $(v))),\
	    echo '$(v) = $($(v))';\
	  ))\
	)) | sort


export MCU
export BOARD
export F_CPU

$(OBJ_DIR)/lib/libsplonk.a:
	@$(MAKE) --no-print-directory -C $(BASE_DIRECTORY)/lib \
		"OBJ_DIR_BASE=$(call relpath,$(abspath $(OBJ_DIR)),$(BASE_DIRECTORY)/lib)" \
		"SOURCE_PREFIX=$(BASE_DIRECTORY)/lib/"

$(ARCHIVE_FILE:.a=.d): $(OBJ)
	@echo "$(abspath $(ARCHIVE_FILE)): \\" > $(ARCHIVE_FILE:.a=.d)
	@cat $(DEP) | sed -e 's/^[^:]*: */  /;s/  / /g;s/ /\n/g;/^$$/d'| \
	 xargs realpath | sort -u | xargs printf '%-78s\\\n' >> $@
	@printf '%-78s\\\n' $(abspath $(PROJECT_MAKEFILE)) >> $@
	@printf '%-78s\\\n' $(abspath $(PROJECT_FILELIST)) >> $@

$(ARCHIVE_FILE): $(ARCHIVE_FILE:.a=.d) $(OBJ)
	@printf " AR $@\n"
	@$(AR) rcs $@ $^
	
ifeq ($(USE_LIB),1)
-include $(OBJ_DIR)/lib/libsplonk.d
endif
-include $(DEP);

