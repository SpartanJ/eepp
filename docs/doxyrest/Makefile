SPHINXOPTS    = -j auto -c .
SPHINXBUILD   = sphinx-build
SOURCEDIR     = ../rst
BUILDDIR      = ../html

clean:
	rm -rf ../doxygen-html ../html ../xml ../rst && mkdir ../xml

doxygen:
	cd ../../ && doxygen Doxyfile
	sh fix_md_files.sh

doxyrest: doxygen
	doxyrest -c doxyrest-config.lua
	perl -i -p0e 's/`(.*?)OS>`__//se' ../rst/page_index.rst
	perl -i -p0e 's/image:: (?!http)/image:: https:\/\/web.ensoft.dev\/eepp\/screenshots\//g' ../rst/page_index.rst
	perl -i -p0e 's/`https:\/\/github.com\/SpartanJ\/eepp.git <https:\/\/github.com\/SpartanJ\/eepp.git>`__`/https:\/\/github.com\/SpartanJ\/eepp.git/g' ../rst/page_index.rst
	perl -i -p0e 's/``git/`git/g' ../rst/page_index.rst

sphinx: doxyrest
	@$(SPHINXBUILD) -M html "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

.PHONY: clean doxygen doxyrest sphinx

all: clean doxygen doxyrest sphinx
