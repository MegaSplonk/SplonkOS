check-var-defined      = $(if $(strip $($1)),,$(error "$1" is not defined))
relpath                = $(shell 	realpath -m --relative-to="$(or $(2),$(CURDIR))" "$(1)")

ifeq ($(USE_ABSOLUTE_PATH),1)
relpath                = $(abspath $(1))
endif

BASE_MAKEFILE    := $(call relpath,$(abspath $(lastword $(MAKEFILE_LIST))))
BASE_DIRECTORY   := $(call relpath,$(dir $(BASE_MAKEFILE)))

MAKEFLAGS += --no-builtin-rules

# Makefile

# TOOLS
CC            = avr-gcc
CXX           = avr-g++
OBJCOPY       = avr-objcopy
AR            = avr-gcc-ar
SIZE          ?= avr-size

FUCKNIXOS_LDFLAGS = 
ifeq ($(MCU), attiny1614)
    FUCKNIXOS_LDFLAGS = $(shell $(CC) -print-search-dirs \
        | grep '^libraries: =' \
        | sed 's/^libraries: =//' \
        | tr : '\n' \
        | grep 'avrxmega2' \
        | sed 's/avrxmega2/avrxmega3/' \
        | xargs printf '-B%s\n'\
    )
endif


# FLAGS
WERROR       ?= 
WFLAGS        = $(WERROR) -Wenum-conversion -Wimplicit-fallthrough -Wpedantic \
                -Wextra -Wall -Wshadow -Wpadded -Wconversion -Wstrict-prototypes \
                -Wmissing-declarations -Wstrict-prototypes -Wundef \
                -Wmissing-prototypes -Wold-style-definition -Winline \
                -Wsign-conversion -Wcast-align -Wcast-qual -Wwrite-strings \
                -Wuninitialized -Wdouble-promotion -Wfloat-equal \
                -Wnull-dereference -Wformat=2
CFLAGS        = -std=c23 -g3 -Os -mmcu=$(MCU) $(CFLAGS_ADD) -flto -ffunction-sections -fdata-sections
CPPFLAGS      = $(if $(filter 1,$(USE_LIB)),\
                   -include$(BASE_DIRECTORY)/lib/include/lib/config.h \
                   -include$(BASE_DIRECTORY)/lib/include/lib/lib.h) \
                -DF_CPU=$(F_CPU) -MMD -MP -DMCU_$(MCU)=1 -DBOARD_$(BOARD)=1 -DPINOUT_$(PINOUT)=1 $(CPPFLAGS_ADD)
IFLAGS        = -I$(INC_DIR) $(IFLAGS_ADD)
LDFLAGS       = $(FUCKNIXOS_LDFLAGS) -Wl,--gc-sections -Wl,-flto $(LDFLAGS_ADD)
AVRDUDE_FLAGS = -c$(PROGRAMMER) -P$(PORT) -b$(BAUD_RATE)


CXXFLAGS        = -std=c++23 -g3 -Os -mmcu=$(MCU) $(CXXFLAGS_ADD) -fno-threadsafe-statics -flto -ffunction-sections -fdata-sections
WXXFLAGS        = $(WERROR) -Wenum-conversion -Wimplicit-fallthrough -Wpedantic \
                -Wextra -Wall -Wshadow -Wpadded -Wconversion  \
                -Wmissing-declarations -Wundef -Winline \
                -Wsign-conversion -Wcast-align -Wcast-qual -Wwrite-strings \
                -Wuninitialized -Wdouble-promotion -Wfloat-equal \
                -Wnull-dereference -Wformat=2

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

LIB_DIR           = $(BASE_DIRECTORY)/lib
LIB_SRC_DIR       = $(call relpath,$(LIB_DIR)/src/)
LIB_FILELIST      = $(LIB_DIR)/Filelist.mk

PINOUT           ?= $(BOARD)

override OBJ_DIR_BASE := $(call relpath,$(OBJ_DIR_BASE))


-include $(PROJECT_FILELIST)
ifeq ($(USE_LIB),1)
-include $(LIB_FILELIST)
endif

OBJ_DIR = $(OBJ_DIR_BASE)/$(PROJECT_NAME)

SRC=$(addsuffix .c,$(addprefix $(SRC_DIR)/,$(SRC_FILES)))
OBJ=$(addsuffix .o,$(addprefix $(OBJ_DIR)/,$(SRC_FILES) $(SRC_FILES_CPP)))
DEP=$(addsuffix .d,$(addprefix $(OBJ_DIR)/,$(SRC_FILES) $(SRC_FILES_CPP)))


SRC_CXX=$(addsuffix .cpp,$(addprefix $(SRC_DIR)/,$(SRC_FILES_CXX)))
OBJ_CXX=$(addsuffix .o,$(addprefix $(OBJ_DIR)/,$(SRC_FILES_CXX)))
DEP_CXX=$(addsuffix .d,$(addprefix $(OBJ_DIR)/,$(SRC_FILES_CXX)))

LIB_SRC=$(addsuffix .c,$(addprefix $(LIB_SRC_DIR), $(LIB_SRC_FILES)))
LIB_OBJ=$(addsuffix .o,$(addprefix $(OBJ_DIR)/lib/,$(LIB_SRC_FILES)))
LIB_DEP=$(addsuffix .d,$(addprefix $(OBJ_DIR)/lib/,$(LIB_SRC_FILES)))

ELF_FILE     = $(OBJ_DIR)/$(TARGET).elf
HEX_FILE     = $(OBJ_DIR)/$(TARGET).hex


# Conservative application limits in bytes, including bootloader headroom.
# Override for a board only after checking its actual application region.
SAFE_FLASH_SIZE = 0
ifeq ($(MCU),atmega32u4)
SAFE_FLASH_SIZE = 28000
endif
ifeq ($(MCU),atmega328p)
SAFE_FLASH_SIZE = 28000
endif
ifeq ($(MCU),attiny1614)
SAFE_FLASH_SIZE = 15000
endif


.DEFAULT_GOAL = all

ifeq ($(USE_LIB),1)
IFLAGS += -I$(call relpath,$(BASE_DIRECTORY)/lib/include)
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

# Check the actual image sent to avrdude, including initialized data and gaps.
# The exclusive end address must fit the application region; errors abort flash.
.PHONY: check-flash-size
check-flash-size: $(HEX_FILE)
	@set -eu; \
	limit='$(SAFE_FLASH_SIZE)'; \
	case "$$limit" in ''|*[!0-9]*) echo "Invalid SAFE_FLASH_SIZE: $$limit" >&2; exit 1;; esac; \
	if ! [ "$$limit" -gt 0 ]; then \
		echo "No positive SAFE_FLASH_SIZE configured for $(MCU)" >&2; exit 1; \
	fi; \
	if ! sizes=$$(LC_ALL=C $(SIZE) --format=sysv --radix=10 "$(HEX_FILE)"); then \
		echo "Cannot measure flash image: $(HEX_FILE)" >&2; exit 1; \
	fi; \
	if ! end=$$(printf '%s\n' "$$sizes" | awk '\
		/^\.sec[0-9]+[[:space:]]/ { \
			if (NF != 3 || $$2 !~ /^[0-9]+$$/ || $$3 !~ /^[0-9]+$$/) exit 1; \
			sections++; bytes += $$2; \
			if ($$3 + $$2 > end) end = $$3 + $$2; \
		} \
		/^Total[[:space:]]/ { \
			if (NF != 2 || $$2 !~ /^[0-9]+$$/) exit 1; \
			totals++; total = $$2; \
		} \
		END { \
			if (!sections || totals != 1 || bytes != total || bytes <= 0) exit 1; \
			printf "%.0f\n", end; \
		}'); then \
		echo "Cannot parse flash image size: $(HEX_FILE)" >&2; exit 1; \
	fi; \
	if [ "$$end" -gt "$$limit" ]; then \
		echo "FLASH TOO BIG: image ends at $$end bytes; limit is $$limit for $(MCU)" >&2; exit 1; \
	fi; \
	printf ' Flash image end: %s / %s bytes (%s)\n' "$$end" "$$limit" '$(MCU)'

.PHONY: flash
flash: check-flash-size
	@$(MAKE) --no-print-directory wait-device
ifeq ($(MCU)$(PROGRAMMER), atmega32u4avr109)
	@printf " STTY $(PORT) 1200\n"
	@stty -F $(PORT) speed 1200 >/dev/null
	@sleep 1
	@$(MAKE) --no-print-directory wait-device
endif
	@printf " FLASH[$(MCU)] $(HEX_FILE)\n"
	@avrdude -p $(MCU) $(AVRDUDE_FLAGS) -U flash:w:$(HEX_FILE):i


define cc_obj
	mkdir -p $(shell dirname $@)
	printf " CC $<\n"
	$(CC) $(CPPFLAGS) $(WFLAGS) $(CFLAGS) $(IFLAGS) -c $< -o $@
endef

define cxx_obj
	mkdir -p $(shell dirname $@)
	printf " CXX $<\n"
	$(CXX) $(CPPFLAGS) $(WXXFLAGS) $(CXXFLAGS) $(IFLAGS) -c $< -o $@
endef

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@$(cc_obj)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@$(cxx_obj)

$(OBJ_DIR)/lib/%.o: $(LIB_SRC_DIR)/%.c
	@$(cc_obj)

$(ELF_FILE): $(OBJ) $(LIB_OBJ) $(OBJ_CPP)
	@printf " CCLD $@ $(if $(filter 1,$(USE_LIB)),[With Lib])\n"
ifneq ($(PROJECT_USE_CPP),y)
	@$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
else
	@$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@
endif

$(HEX_FILE): $(ELF_FILE)
	@printf " OBJCOPY $@\n"
	@$(OBJCOPY) -O ihex $< $@

.PHONY: filelist
filelist:
	@rm -f "$(PROJECT_FILELIST)"
	@printf '%-78s\\\n' "SRC_FILES =" > "$(PROJECT_FILELIST)"
	@find -L "$(SRC_DIR)" -type f -name '*.c' \
		| sed -e 's|^$(SRC_DIR)/||' -e 's|\.c$$||' \
		| sort \
		| while read f; do printf '%-78s\\\n' "$$f"; done >> "$(PROJECT_FILELIST)"
	@echo "" >> "$(PROJECT_FILELIST)"
ifeq ($(PROJECT_USE_CPP),y)
	@printf '%-78s\\\n' "SRC_FILES_CPP =" >> "$(PROJECT_FILELIST)"
	@find -L "$(SRC_DIR)" -type f -name '*.cpp' \
		| sed -e 's|^$(SRC_DIR)/||' -e 's|\.cpp$$||' \
		| sort \
		| while read f; do printf '%-78s\\\n' "$$f"; done >> "$(PROJECT_FILELIST)"
	@echo "" >> "$(PROJECT_FILELIST)"
endif
	@echo "-include $(LIB_DIR)/Filelist.mk" >> "$(PROJECT_FILELIST)"
	@$(MAKE) --no-print-directory -C $(LIB_DIR) filelist
	@printf "Filelist: $(PROJECT_NAME)\n"

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
	cc_obj


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

-include $(DEP)
-include $(LIB_DEP)

