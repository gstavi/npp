THIS_DIR := $(dir $(lastword $(subst \,/,$(MAKEFILE_LIST))))
include $(THIS_DIR)common.mak

TARGET=libboost_regex.lib
OUTDIR = $(WORKDIR)/scilexer
MORE_INCLUDE_DIRS =
BOOST_ROOT ?= $(TOPDIR)/boost_1_54_0
OUTDIR = $(WORKDIR)/boost_regex
OUTDIR_WIN = "$(shell cygpath -w $(abspath $(OUTDIR)))"

$(eval $(call COMMON_FLAG_RULES))

all: $(PLATDORM_DIR)/$(TARGET)

$(PLATDORM_DIR)/$(TARGET): $(OUTDIR)/lib/$(TARGET) $(FLAG)
	cp $< $@

#Since we don't edit boost we don't care about dependencies. If target exist we
# don't try to recreate it
$(OUTDIR)/lib/$(TARGET):
	@$(MK) -f $(MAIN_MAKE) compile_boost

compile_boost: $(OUTDIR)/bin/b2.exe
	$(MK) -f $(abspath $(MAIN_MAKE)) -C $(BOOST_ROOT) boost_build_regex

$(OUTDIR)/bin/b2.exe:
	$(MK) -f $(abspath $(MAIN_MAKE)) -C $(BOOST_ROOT)/tools/build/v2 boost_bootstrap

boost_bootstrap:
	cmd /c bootstrap.bat
	./b2.exe --prefix=$(OUTDIR_WIN) toolset=msvc

boost_build_regex:
	$(OUTDIR)/bin/b2.exe --with-regex --build-dir=$(OUTDIR_WIN) \
		--layout=system --stagedir=$(OUTDIR_WIN) toolset=msvc \
		variant=$(if $(DBGBLD),debug,release) \
		link=static threading=multi runtime-link=static

clean:
	rm -rf $(OUTDIR)

.PHONY: compile_boost boost_bootstrap boost_build_regex

deps:
	echo ASASASAS$(error popo)