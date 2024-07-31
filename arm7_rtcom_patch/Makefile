#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

export TEST_ROM_NAME	:=	RtcomTestProject
export TARGET	:=	$(shell basename $(CURDIR))
export TOPDIR	:=	$(CURDIR)

# specify a directory which contains the nitro filesystem
# this is relative to the Makefile
# NITRODATA	:= files

# These set the information text in the nds file
#GAME_TITLE     := My Wonderful Homebrew
#GAME_SUBTITLE1 := built with devkitARM
#GAME_SUBTITLE2 := http://devitpro.org

ifneq ($(strip $(NITRODATA)),)
	export NITRO_FILES	:=	$(CURDIR)/$(NITRODATA)
endif

include $(DEVKITARM)/ds_rules

.PHONY: checkarm11_ucode checkarm7 checkarm9 clean

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all: checkarm11_ucode checkarm7 checkarm9 $(TARGET).nds

#---------------------------------------------------------------------------------
checkarm11_ucode:
	$(MAKE) -C arm11_ucode

#---------------------------------------------------------------------------------
checkarm7: checkarm11_ucode
	$(MAKE) -C arm7
	
#---------------------------------------------------------------------------------
checkarm9:
	$(MAKE) -C arm9

#---------------------------------------------------------------------------------
$(TARGET).nds	: $(NITRO_FILES) arm7/$(TARGET).elf arm9/$(TARGET).elf
	ndstool	-c $(TEST_ROM_NAME).nds -7 arm7/$(TARGET).elf -9 arm9/$(TARGET).elf \
	-b $(GAME_ICON) "$(GAME_TITLE);$(GAME_SUBTITLE1);$(GAME_SUBTITLE2)" \
	$(_ADDFILES)

#---------------------------------------------------------------------------------
arm11_ucode/$(TARGET).uc11:
	$(MAKE) -C arm11_ucode

#---------------------------------------------------------------------------------
arm7/$(TARGET).elf: arm11_ucode/$(TARGET).uc11
	$(MAKE) -C arm7
	
#---------------------------------------------------------------------------------
arm9/$(TARGET).elf:
	$(MAKE) -C arm9

#---------------------------------------------------------------------------------
clean:
	$(MAKE) -C arm11_ucode clean
	$(MAKE) -C arm9 clean
	$(MAKE) -C arm7 clean
	rm -f $(TEST_ROM_NAME).nds $(TARGET).arm7 $(TARGET).arm9
