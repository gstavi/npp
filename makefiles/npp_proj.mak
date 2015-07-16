THIS_DIR := $(dir $(lastword $(subst \,/,$(MAKEFILE_LIST))))
include $(THIS_DIR)common.mak

DEP_MAKES = $(addprefix $(MAIN_DIR),\
	scilexer.mak npp.mak tagleet.mak lexamples.mak)

all: install

install clean build:
	$(VV)$(call DO_DEP_MAKES,$@)

distclean:
	$(VV)$(call DO_DEP_MAKES,$@)
	$(vv) rm -rf $(INSTALL_PATH)
