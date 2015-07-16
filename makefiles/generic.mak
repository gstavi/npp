# File: generic.mak
# Generic makefile for c/c++ executables and libraries
# Copyright 2013 by Gur Stavi <gur.stavi@gmail.com>

THIS_MAKE := $(subst \,/,$(word $(words $(MAKEFILE_LIST)), $(MAKEFILE_LIST)))
MAKER = $(VV)$(MAKE) --no-print-directory -r -f $(THIS_MAKE)
THIS_DIR := $(dir $(THIS_MAKE))

$(if $(TARGET),,$(error TARGET not defined))
DBGBLD = $(filter 1,$(DEBUG))
DBGVARS=0

TOOLCHAIN ?= gcc
CFLAGS_VARS ?= BASE_CFLAGS CFLAGS
LDFLAGS_VARS ?= BASE_LDFLAGS LIBS
RESFLAGS_VARS ?=
CFLAGS ?=
BASE_LDFLAGS ?=
LIBS ?=
LINK_DEPS ?=
OUTDIR ?= $(if $(DBGBLD),dbg,opt)
OBJDIR = $(OUTDIR)/obj
DEPDIR = $(OUTDIR)/dep
RESDIR = $(OUTDIR)/obj/res
CPPEXT ?= cpp
VV = $(if $(filter 1,$(VERBOSE)),,@)

ifeq ($(DO_COMPILE),1)
$(if $(SRC_FILES),,$(info SRC_FILES undefined using local c/cpp files))
SRC_FILES ?= $(wildcard *.c *.cpp)
AUTO_INCLUDE_DIRS := $(if $(INCLUDE_DIRS_BASE), $(strip $(sort \
	$(shell /usr/bin/find $(INCLUDE_DIRS_BASE) -name '*.h' -printf "%h\n"))))
INCLUDE_DIRS += $(AUTO_INCLUDE_DIRS) $(MORE_INCLUDE_DIRS)
endif

TARGET_TYPE=$(word 2,$(subst :, ,$(filter *$(suffix $(TARGET)):%,\
	*.exe:EXE *.EXE:EXE *:EXE *.so:DLL *.dll:DLL *.DLL:DLL *.lib:LIB *.LIB:LIB *.a:LIB)))

ifeq ($(TARGET_TYPE),)
$(error failed to detect target type from '$(TARGET)')
endif

# VAR[_(ms|gcc)][_(dbg|opt)][_(filename|suffix)]
# 1-VARNAME 2-TOOLCHAIN 3-DBG|OPT 4-SUFFIX 5-FILENAME
candidate_list_order = \
	$1$2$3$5 $1$2$5 $1$3$5 $1$5 \
	$1$2$3$4 $1$2$4 $1$2$3 $1$2 \
	$1$3$4 $1$3 $1$4 $1
fixname = $(subst -,_,$(subst .,_,$(1)))
#1-VARNAME 2-FILENAME 3-SELECTED-VARNAME
candidate_list = $(call candidate_list_order,$(1),_$(TOOLCHAIN),$( \
	)_$(if $(DBGBLD),dbg,opt),$(call fixname,$(suffix $(notdir $(2)))),$( \
	)_$(call fixname,$(notdir $(2))))
valid_candidates = $(foreach v,$(call candidate_list,$(1),$(2)),\
	$(if $(filter undefined,$(origin $(v))),,$(v))) unknown_var
unknown_var = $(error Can't find $(1) for $(2))
get_var_helper = $(if $(filter 1,$(DBGVARS)),$(info $(1)[$(2)]=$(3)))$($(3))
get_var = $(call get_var_helper,$(1),$(2),$(word 1,$(call valid_candidates,$(1),$(2))))
get_multi_var = $(foreach v,$(call get_var,$(1),$(2)),$(call get_var,$(v),$(2)))

ifeq ($(TOOLCHAIN),gcc)
OBJEXT=o
RESEXT=res
GCC_COMPILER ?= $(CROSS_TOOL_PREFIX)gcc
GCC_ARCHIVER ?= $(CROSS_TOOL_PREFIX)ar
GCC_COMPILER_$(CPPEXT) ?= $(CROSS_TOOL_PREFIX)g++
GCC_LINKER ?= $(CROSS_TOOL_PREFIX)$(if $(filter .$(CPPEXT),$(suffix $(SRC_FILES))),g++,gcc)
BASE_CFLAGS_dbg ?= -O0 -D_DEBUG
BASE_CFLAGS_opt ?= -O2 -DNDEBUG
DBG_INFO_CFLAGS ?=
ALL_CFLAGS = $$(call get_multi_var,CFLAGS_VARS,$(1)) $(addprefix -I,$(INCLUDE_DIRS))
ALL_RESFLAGS = $$(call get_multi_var,RESFLAGS_VARS,$(1)) $(addprefix -I,$(INCLUDE_DIRS))
COMPILER = $$(call get_var,GCC_COMPILER,$(1))
SONAME ?= $(notdir $(TARGET))
#COMPILE_MACRO $(1)=<src file> $(2)=<obj file>
define COMPILE_MACRO
	@echo Compile $$(notdir $(1))
	$(VV)$(COMPILER) -c $(ALL_CFLAGS) $(1) -o $(2)
endef
define COMPILE_RES_MACRO
	@echo Resource $$(notdir $(1))
	$(VV)windres $(ALL_RESFLAGS) -i $(1) -O coff -o $(2)
endef
#LINK_MACRO $(1)=<dst.exe> assume src.o in $^
LINK_MACRO_EXE = $(VV)$(GCC_LINKER) -o $(1) $(OBJ_FILES) $(call get_multi_var,LDFLAGS_VARS,$(1))
LINK_MACRO_DLL = $(VV)$(GCC_LINKER) -o $(1) $(OBJ_FILES) -shared\
 -Wl,-soname,$(call get_var,SONAME,$(1)) $(call get_multi_var,LDFLAGS_VARS,$(1))
LINK_MACRO_LIB = $(VV)$(GCC_ARCHIVER) -rc $(1) $(OBJ_FILES)
#DEP_MACRO $(1)=<src.c> $(2)=<dst.d>
define DEP_MACRO
	@echo Depend $$(notdir $(1))
	$(VV)$(COMPILER) -MM -MF $(2) -MT $(2) $(ALL_CFLAGS) $(1)
endef
endif

$(if $(filter gcc ms,$(TOOLCHAIN)),,$(error Unknown TOOLCHAIN '$(TOOLCHAIN)'))

ifeq ($(TOOLCHAIN),ms)
OBJEXT=obj
PCHEXT=pch
RESEXT=res
MS_COMPILER ?= cl.exe
MS_LINKER ?= cl.exe
MS_STATIC_LINKER ?= link.exe /lib
BASE_CFLAGS_dbg ?= /Od /D_DEBUG
BASE_CFLAGS_opt ?= /O2 /DNDEBUG
DBG_INFO_CFLAGS ?= /Zi /Fd$(basename $@).pdb
PCH_CREATE_CFLAGS ?= /Yc /Fp:$(PCH_FILE)
PCH_USE_CFLAGS ?= /Yu$(PRE_COMP_FILE_NAME) /Fp$(PCH_FILE)
BASE_LDFLAGS = /debug
ALL_CFLAGS = $$(call get_multi_var,CFLAGS_VARS,$(1)) $(addprefix /I,$(INCLUDE_DIRS))
ALL_RESFLAGS = $$(call get_multi_var,RESFLAGS_VARS,$(1)) $(addprefix /i,$(INCLUDE_DIRS))
COMPILER = $$(call get_var,MS_COMPILER,$(1))
PRE_COMPILE_HDR_MACRO = $(VV)$(COMPILER) /nologo /c $(ALL_CFLAGS) $(1)
COMPILE_MACRO = $(VV)$(COMPILER) /nologo /c $(ALL_CFLAGS) $(1) /Fo$(2)
COMPILE_RES_MACRO = $(VV)rc.exe $(ALL_RESFLAGS) /fo $(2) $(1)
LINK_MACRO_EXE = $(VV)$(MS_LINKER) /nologo $(OBJ_FILES) /Fe$(1) \
	/link $(call get_multi_var,LDFLAGS_VARS,$(1))
LINK_MACRO_DLL = $(VV)$(MS_LINKER) /nologo /LD $(OBJ_FILES) /Fe$(1) \
	/link $(call get_multi_var,LDFLAGS_VARS,$(1))
LINK_MACRO_LIB = $(VV)$(MS_STATIC_LINKER) $(OBJ_FILES) /OUT:$(1)
define DEP_MACRO
	$(VV)$(COMPILER) /nologo /E $(ALL_CFLAGS) $(1) | \
	sed -n "s|^#line [0-9]* ||p" | \
	/usr/bin/sort -u | \
	xargs cygpath -u | \
	sed -e 's| |\\ |g' -e "s|.*|$(2): &|">$(2)
endef
endif

DEP_FILES = $(addprefix $(DEPDIR)/,$(addsuffix .d,$(basename \
	$(notdir $(SRC_FILES)))))
PCH_FILE = $(strip\
	$(addprefix $(OBJDIR)/,\
	$(addsuffix .$(PCHEXT),$(basename $(notdir $(PRE_COMP_HDR))))))
OBJ_FILES = $(strip\
	$(addprefix $(OBJDIR)/,\
	$(addsuffix .$(OBJEXT),$(basename $(notdir $(SRC_FILES)))))\
	$(addprefix $(RESDIR)/,\
	$(addsuffix .$(RESEXT),$(basename $(notdir $(RES_FILES))))))

TARGET_LINK_DEP := $(call get_var,LINK_DEPS,$(TARGET))

# If source files are not the same as last link (e.g. a file was removed) then
# force a link even if all object files are up to date. Also if link flags changed.
SRC_FILES_REF = $(DEPDIR)/src.files
SQUEEZED_SRC_FILES := !$(subst $(UNdEF) $(UNdEF),!,$(strip \
    $(SRC_FILES) $(call get_multi_var,LDFLAGS_VARS,$(TARGET))))
OLD_SRC_FILES := $(if $(wildcard $(SRC_FILES_REF)),$(shell cat $(SRC_FILES_REF)),X)
FORCE_LINK = $(if $(subst $(SQUEEZED_SRC_FILES),,$(OLD_SRC_FILES)),FORCE)

all:
	$(MAKER) DO_COMPILE=1 $(OUTDIR)/$(TARGET)

$(OUTDIR)/$(TARGET): $(OBJ_FILES) $(TARGET_LINK_DEP) $(FORCE_LINK) | SILENT
	$(call LINK_MACRO_$(TARGET_TYPE),$@)
	@echo -n '$(SQUEEZED_SRC_FILES)'>$(SRC_FILES_REF)

dummy_link_dep $(TARGET_LINK_DEP):

# Convert source file in $(1) to obj/dep/res file
src2pch = $(OBJDIR)/$(basename $(notdir $(1))).$(PCHEXT)
src2obj = $(OBJDIR)/$(basename $(notdir $(1))).$(OBJEXT)
src2dep = $(DEPDIR)/$(notdir $(1)).d
src2res = $(RESDIR)/$(basename $(notdir $(1))).$(RESEXT)

# Generate rule for pre compiled header (.pch) file
gen_pch_file = $(src2pch): $(1) $(src2dep) | $(OBJDIR); \
	$(call PRE_COMPILE_HDR_MACRO,$$<,$$@)
$(foreach file,$(PRE_COMP_HDR),$(eval $(call gen_pch_file,$(file))))

# Generate rules for object (.obj) files
gen_compile_file = $(src2obj): $(1) $(src2dep) | $(OBJDIR); \
	$(call COMPILE_MACRO,$$<,$$@)
$(foreach file,$(SRC_FILES),$(eval $(call gen_compile_file,$(file))))

# Generate rules for compiling resource files
gen_compile_res_file = $(src2res) : $(1) | $(RESDIR); \
	$(call COMPILE_RES_MACRO,$$<,$$@)
$(foreach file,$(RES_FILES),$(eval $(call gen_compile_res_file,$(file))))

# Generate rules for dependency (.d) files
PCH_DEP=$(if $(filter $(PRE_COMP_HDR),$(1)),,$(PCH_FILE))
define gen_dep_file
$(src2dep): $(1) $(PCH_DEP) | $(DEPDIR)
	@rm -f $$@
	$(call DEP_MACRO,$$<,$$@)
endef
$(foreach file,$(SRC_FILES) $(PRE_COMP_HDR),$(eval $(call gen_dep_file,$(file))))

# Create output directories
$(OUTDIR) $(OBJDIR) $(DEPDIR) $(RESDIR): ; mkdir -p $@
# Create single project dependency files from multiple source dependency files.
$(OUTDIR)/make.dep: $(DEP_FILES) | $(OUTDIR)
	@rm -f $@
	@cat $(DEP_FILES)>$@
	$(if $(EXTRA_DEP), @echo "$(DEP_FILES) $(OBJ_FILES): $(EXTRA_DEP)">>$@)

clean: ; rm -rf $(OUTDIR)
rebuild: clean ; $(MAKER) all
deps: ; $(MAKER) $(OUTDIR)/make.dep DO_COMPILE=1

# Include project dependency file only when actually compiling
ifeq ($(DO_COMPILE),1)
-include $(OUTDIR)/make.dep
endif

# To enable progress in case headers that were in dependencies were removed and
# no longer needed
%.h %.hpp: ; @echo need $@

FORCE:
SILENT: ; @:

.PHONY: all clean rebuild deps dummy_link_dep FORCE SILENT
