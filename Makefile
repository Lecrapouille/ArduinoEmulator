###############################################################################
# Location of the project directory and Makefiles
#
P := .
M := $(P)/.makefile

###############################################################################
# Project definition
#
include $(P)/Makefile.common
TARGET_NAME := Arduino-Emulator
TARGET_DESCRIPTION := An emulator for Arduino for testing your Arduino code.
include $(M)/project/Makefile

###############################################################################
# Inform Makefile where to find third-party header files
#
INCLUDES += $(P)/include $(P)/src
INCLUDES += $(THIRD_PARTIES_DIR)
INCLUDES += $(THIRD_PARTIES_DIR)/json/include
INCLUDES += $(THIRD_PARTIES_DIR)/cxxopts/include

###############################################################################
# Inform Makefile where to find *.cpp files
#
VPATH += $(P)/src

###############################################################################
# Project defines
#
DEFINES +=

###############################################################################
# Make the list of files to compile
#
SRC_FILES := $(wildcard $(P)/src/*.cpp)

###############################################################################
# Set third-party library flags
#
USER_CXXFLAGS += -Wno-old-style-cast -Wno-noexcept -Wno-useless-cast
USER_CXXFLAGS += -Wno-switch-enum -Wno-float-equal

###############################################################################
# Sharable information between all Makefiles
#
include $(M)/rules/Makefile