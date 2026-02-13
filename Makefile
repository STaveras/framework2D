# Build settings
CXX ?= c++
STD := -std=c++17
WARN := -Wall -Wextra
SUPPRESS_UNUSED ?= 1
ifeq ($(SUPPRESS_UNUSED),1)
  WARN += -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function
endif
DIAG := -fdiagnostics-color=always
INCLUDES := -I./ext -isystem ./ext/inc -isystem ./ext/inc/metal-cpp
LIBS := -lglfw -lvulkan -ltinyxml2 -lsimdjson

# Debug (make DEBUG=1)
ifeq ($(DEBUG),1)
  DEBUG_FLAGS := -D_DEBUG -g
else
  DEBUG_FLAGS :=
endif

UNAME_S := $(shell uname -s)

# Platform-specific flags
ifeq ($(UNAME_S),Darwin)
  CXX := clang++
  PLATFORM_COMPILE_FLAGS := -stdlib=libc++ -DGL_SILENCE_DEPRECATION -x objective-c++
  PLATFORM_LINK_FLAGS := -stdlib=libc++ -framework Metal -framework QuartzCore -framework OpenGL -framework Cocoa
  # Ensure runtime can find Vulkan dylib on common macOS prefixes
  PLATFORM_RPATH := -Wl,-rpath,/usr/local/lib -Wl,-rpath,/opt/homebrew/lib
  PLATFORM_SRCS := src/RendererMTL.mm src/TextureMTL.mm
else
  PLATFORM_COMPILE_FLAGS :=
  PLATFORM_LINK_FLAGS :=
  PLATFORM_RPATH :=
  PLATFORM_SRCS :=
endif

COMMON_SRCS := $(shell find src -type f -name '*.cpp' | sort)

SRCS := $(COMMON_SRCS) $(PLATFORM_SRCS)

TARGET_BASE := $(notdir $(CURDIR))
ifeq ($(DEBUG),1)
  TARGET := bin/$(TARGET_BASE)_d
  OBJDIR := build/obj_d
else
  TARGET := bin/$(TARGET_BASE)
  OBJDIR := build/obj
endif
OBJS := $(SRCS:%.cpp=$(OBJDIR)/%.o)
OBJS := $(OBJS:%.mm=$(OBJDIR)/%.o)

CPPFLAGS := $(INCLUDES)
CXXFLAGS := $(STD) $(WARN) $(DIAG) $(DEBUG_FLAGS) $(PLATFORM_COMPILE_FLAGS)
LDFLAGS := $(LIBS) $(PLATFORM_LINK_FLAGS) $(PLATFORM_RPATH)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $^ -o $@ $(LDFLAGS)

# C++ sources
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Objective-C++ sources (macOS)
$(OBJDIR)/%.o: %.mm
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(OBJS:.o=.d)

clean:
	rm -rf build/obj build/obj_d bin/$(TARGET_BASE) bin/$(TARGET_BASE)_d
