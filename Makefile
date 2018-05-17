#------------------------------------------------------------------------------#
# Linux Makefile for OPINCAA API.                                              #
#------------------------------------------------------------------------------#

ARCHITECTURES := connex16-hm-generic

.PHONY: build

build: $(ARCHITECTURES)

$(ARCHITECTURES):
	make -C arch/$@

install: build
	make -C arch/$(ARCHITECTURES) install

test: build
	@cd tests && ./test_harness.sh && cd ..

clean:
	@rm -rf build
	@rm -rf libs
