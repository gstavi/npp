MKFILE_LIST := $(subst \,/,$(MAKEFILE_LIST))
COMMON_MAK := $(lastword $(MKFILE_LIST))
MAIN_MAKE := $(lastword $(filter-out $(COMMON_MAK),$(MKFILE_LIST)))
$(if $(MAIN_MAKE),,$(error Could not detect main makefile))

COMMON_DIR := $(dir $(COMMON_MAK))
MAIN_DIR := $(dir $(MAIN_MAKE))
MK = $(MAKE) --no-print-directory -r
MKGEN = $(MK) -f $(COMMON_DIR)generic.mak
VV = $(if $(filter 1,$(VERBOSE)),,@)

TOOLCHAIN=ms
DEBUG=1
VERBOSE=0
TOPDIR = $(COMMON_DIR)..
PLATDORM_DIR = $(TOPDIR)/$(TOOLCHAIN)
DBGBLD = $(filter 1,$(DEBUG))
MSBLD = $(filter ms,$(TOOLCHAIN))
WORKDIR = $(PLATDORM_DIR)/$(if $(DBGBLD),dbg,opt)
FLAG = $(OUTDIR)/flag.txt
DBG_FLAG=$(subst $(PLATDORM_DIR)/opt,$(PLATDORM_DIR)/dbg,$(FLAG))
OPT_FLAG=$(subst $(PLATDORM_DIR)/dbg,$(PLATDORM_DIR)/opt,$(FLAG))

INSTALL = cp

# Clear them so a sub project will not inherit the values
CFLAGS_VARS = BASE_CFLAGS DBG_INFO_CFLAGS
LDFLAGS_VARS = BASE_LDFLAGS
DEP_MAKES =
CPPEXT =
MORE_INCLUDE_DIRS =
PRE_COMPILE_DEP =
PRE_INSTALL_DEP =
PRE_COMP_HDR =

RT_CFLAGS_ms_opt = /MT
RT_CFLAGS_ms_dbg = /MTd
RT_CFLAGS_gcc =
C_RT_LIBS_ms_opt =
C_RT_LIBS_ms_dbg =
C_RT_LIBS_gcc =
CPP_RT_LIBS_ms_opt =
CPP_RT_LIBS_ms_dbg =
CPP_RT_LIBS_gcc =

INSTALL_DIR =
INSTALL_PATH = $(TOPDIR)/bin/$(INSTALL_DIR)

TARGET_FILES = $(TARGET)
ERR_DEP = $(if $(TARGET),$(if $(SRC_FILES),,src_files_err),target_err)

define MAKE_LINE
	$(MK) -f $(1) $(2)

endef # blank line above is intentional

DO_DEP_MAKES = $(foreach depmake,$(DEP_MAKES),$(call MAKE_LINE,$(depmake),$(1)))

common_all: all

define COMMON_FLAG_RULES
$(DBG_FLAG): ; $(VV)rm -f $(OPT_FLAG) && touch $$@
$(OPT_FLAG): ; $(VV)rm -f $(DBG_FLAG) && touch $$@
endef

define COMMON_RULES
build all: $(addprefix $(PLATDORM_DIR)/,$(TARGET_FILES))
$(addprefix $(PLATDORM_DIR)/,$(TARGET_FILES)): $(PLATDORM_DIR)/%: \
	$(OUTDIR)/% $(FLAG)
	cp $$< $$@
install: $(addprefix $(INSTALL_PATH)/,$(TARGET_FILES)) $(PRE_INSTALL_DEP)
$(addprefix $(INSTALL_PATH)/,$(TARGET_FILES)): $(INSTALL_PATH)/%: \
	$(PLATDORM_DIR)/% | $(INSTALL_PATH)
	$(INSTALL) $$< $$@
$(WORKDIR) $(OUTDIR) $(INSTALL_PATH): ; mkdir -p $$@
$(addprefix $(OUTDIR)/,$(TARGET_FILES)): FORCE $$(ERR_DEP) $(PRE_COMPILE_DEP)
	$$(if $$(filter %$$(TARGET),$$@),\
		$$(call DO_DEP_MAKES)\
		$$(MKGEN))
clean:
	$(VV)$$(MKGEN) $$@
	$(VV)rm -f $(PLATDORM_DIR)/$(TARGET)
distclean: clean
	$$(call DO_DEP_MAKES,$$@)
deps: $$(ERR_DEP)
	$$(call DO_DEP_MAKES,$$@)
	$$(MKGEN) $$@
$(call COMMON_FLAG_RULES)
endef


target_err: ;$(error $(notdir $(MAIN_MAKE)). TARGET undefined)
src_files_err: ;$(error $(notdir $(MAIN_MAKE)). SRC_FILES undefined/empty)
rebuild: clean; $(VV)$(MK) -f $(MAIN_MAKE) all

FORCE:

.PHONY: common_all all build install rebuild clean distclean deps \
	src_files_err target_err FORCE

.EXPORT_ALL_VARIABLES:
