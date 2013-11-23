#------------------------------------------------------------------------------#
# Linux Makefile for OPINCAA API.                                              #
#------------------------------------------------------------------------------#

ARCHITECTURES := connex16-hm-generic

.PHONY: build

build: $(ARCHITECTURES)

$(ARCHITECTURES):
	make -C arch/$@

clean:
	@rm -rf build
	@rm -rf libs
