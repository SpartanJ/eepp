SPHINXOPTS    = -c .
SPHINXBUILD   = sphinx-build
SOURCEDIR     = ../rst
BUILDDIR      = ../html

clean:
	rm -rf ../doxygen-html ../html ../xml ../rst && mkdir ../xml

doxygen:
	cd ../../ && doxygen Doxyfile

doxyrest: doxygen
	doxyrest -c doxyrest-config.lua

sphinx: doxyrest
	@$(SPHINXBUILD) -M html "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

.PHONY: clean doxygen doxyrest sphinx

all: clean doxygen doxyrest sphinx
