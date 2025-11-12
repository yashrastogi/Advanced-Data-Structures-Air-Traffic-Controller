# ---- Configurable variables (override on the command line) -------------------
CXX       ?= g++
TARGET    ?= gatorAirTrafficScheduler
SRCS      ?= Gator_Air_Traffic_Slot_Scheduler.cpp
BUILD     ?= debug          # options: debug | release
SAN       ?= address        # set empty to disable (e.g., SAN=)
ARCH      ?= arm64          # only used on macOS; set empty to skip

# ---- Derived flags -----------------------------------------------------------
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
# Sanitizer applies in both modes if SAN is set
CXXFLAGS += $(SAN_F)
LDFLAGS  := $(SAN_F)

# ---- Files / objects ---------------------------------------------------------
OBJS := $(SRCS:.cpp=.o)

# ---- Default target ----------------------------------------------------------
.PHONY: all
all: $(TARGET)

# Keep your original 'main' target as an alias to the build
.PHONY: main
main: $(TARGET)

# Link
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Compile
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ---- Convenience targets -----------------------------------------------------
.PHONY: run
run: $(TARGET)
	./$(TARGET) input.txt

# Rebuild from scratch
.PHONY: rebuild
rebuild: clean all

# Clean artifacts
.PHONY: clean
clean:
	$(RM) $(OBJS) $(TARGET)

# Nodemon helper (requires nodemon installed)
.PHONY: nodemon
nodemon:
	nodemon --ext "cpp,hpp,txt" --exec 'make run || exit 1 ; make clean'

# Show config
.PHONY: info
info:
	@echo "CXX       = $(CXX)"
	@echo "BUILD     = $(BUILD)"
	@echo "SAN       = $(SAN)"
	@echo "ARCH      = $(ARCH)"
	@echo "CXXFLAGS  = $(CXXFLAGS)"
	@echo "LDFLAGS   = $(LDFLAGS)"
	@echo "SRCS      = $(SRCS)"
	@echo "OBJS      = $(OBJS)"
	@echo "TARGET    = $(TARGET)"
