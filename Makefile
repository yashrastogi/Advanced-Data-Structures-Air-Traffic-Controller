# Configurable variables
CXX       ?= g++
TARGET    ?= gatorAirTrafficScheduler
SRCS      ?= Gator_Air_Traffic_Slot_Scheduler.cpp
BUILD     ?= debug
SAN       ?= address
ARCH      ?= arm64

# Derived flags
WARN      := -Wall -Wextra -Wpedantic -Werror
DEBUG_F   := -g
OPT_F_DBG := -O1
OPT_F_REL := -O3
SAN_F     := $(if $(SAN),-fsanitize=$(SAN),)

# Only pass -arch on macOS/Clang; it's not a GNU g++ flag on Linux
UNAME_S   := $(shell uname -s)
ARCH_F    :=
ifeq ($(UNAME_S),Darwin)
  ifneq ($(ARCH),)
    ARCH_F := -arch $(ARCH)
  endif
endif

# Choose flags per build type
ifeq ($(BUILD),release)
  CXXFLAGS := $(WARN) $(OPT_F_REL) $(ARCH_F)
else
  CXXFLAGS := $(WARN) $(DEBUG_F) $(OPT_F_DBG) $(ARCH_F)
endif
CXXFLAGS += $(SAN_F)

.PHONY: all
all: $(TARGET)

.PHONY: main
main: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $^ -o $@

.PHONY: run
run: $(TARGET)
	./$(TARGET) input.txt

.PHONY: rebuild
rebuild: clean all

.PHONY: clean
clean:
	$(RM) $(TARGET)

# Nodemon helper (requires nodemon installed)
.PHONY: nodemon
nodemon:
	nodemon --ext "cpp,hpp" --exec 'make run || exit 1 ; make clean'

.PHONY: info
info:
	@echo "CXX       = $(CXX)"
	@echo "BUILD     = $(BUILD)"
	@echo "SAN       = $(SAN)"
	@echo "ARCH      = $(ARCH)"
	@echo "CXXFLAGS  = $(CXXFLAGS)"
	@echo "SRCS      = $(SRCS)"
	@echo "TARGET    = $(TARGET)"
