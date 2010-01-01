PYTHON = python
top_srcdir = `pwd`
SUBDIRS = \
	$(top_srcdir)/config \
	$(top_srcdir)/examples \
	$(top_srcdir)/examples/freetype \
	$(top_srcdir)/examples/sdl \
	$(top_srcdir)/examples/sdlext \
	$(top_srcdir)/doc \
	$(top_srcdir)/doc/capi \
	$(top_srcdir)/doc/src \
	$(top_srcdir)/doc/tutorial \
	$(top_srcdir)/test \
	$(top_srcdir)/test/c_api \
	$(top_srcdir)/test/util \
	$(top_srcdir)/lib \
	$(top_srcdir)/lib/freetype \
	$(top_srcdir)/lib/sdl \
	$(top_srcdir)/lib/sdlext \
	$(top_srcdir)/lib/sdlgfx \
	$(top_srcdir)/lib/sdlimage \
	$(top_srcdir)/lib/sdlmixer \
	$(top_srcdir)/lib/sdlttf \
	$(top_srcdir)/lib/sprite \
	$(top_srcdir)/lib/threads \
	$(top_srcdir)/src \
	$(top_srcdir)/src/base \
	$(top_srcdir)/src/freetype \
	$(top_srcdir)/src/mask \
	$(top_srcdir)/src/sdl \
	$(top_srcdir)/src/sdlext \
	$(top_srcdir)/src/sdlgfx \
	$(top_srcdir)/src/sdlimage \
	$(top_srcdir)/src/sdlmixer \
	$(top_srcdir)/src/sdlttf

all: clean build

dist: clean docs
	@echo "Creating dist..."
	@$(PYTHON) setup.py sdist --format gztar
	@$(PYTHON) setup.py sdist --format zip

bdist: clean docs
	@echo "Creating bdist..."
	@$(PYTHON) setup.py bdist

build:
	@echo "Running build..."
	@$(PYTHON) setup.py build #-c mingw32
	@echo "Build finished, invoke 'make install' to install."

install:
	@echo "Installing..."
	@$(PYTHON) setup.py install 

clean:
	@echo "Cleaning up in $(top_srcdir)/ ..."
	@rm -f *.cache *.core *~ MANIFEST *.pyc *.orig
	@rm -rf src/doc
	@rm -rf build dist

	@for dir in $(SUBDIRS); do \
		if test -f $$dir/Makefile; then \
			make -C $$dir clean; \
		else \
			cd $$dir; \
			echo "Cleaning up in $$dir..."; \
			rm -f *~ *.cache *.core *.pyc *.orig; \
		fi \
	done

docs:
	@echo "Creating docs package"
	@cd doc && make html
	@mv doc/sphinx/build/html doc/html
	@rm -rf doc/sphinx/build
	@cd doc && make docclean

release: dist
	@$(PYTHON) config/bundle_docs.py

runtest:
	@$(PYTHON) test/run_tests.py

# Do not run these in production environments! They are for testing
# purposes only!

buildall: clean
	@python2.4 setup.py build
	@python2.5 setup.py build
	@python2.6 setup.py build
	@python3.1 setup.py build

installall:
	@python2.4 setup.py install
	@python2.5 setup.py install
	@python2.6 setup.py install
	@python3.1 setup.py install

testall:
	@python2.4 test/run_tests.py
	@python2.5 test/run_tests.py
	@python2.6 test/run_tests.py
	@python3.1 test/run_tests.py

testall2:
	@python2.4 -c "import pygame2.test; pygame2.test.run ()"
	@python2.5 -c "import pygame2.test; pygame2.test.run ()"
	@python2.6 -c "import pygame2.test; pygame2.test.run ()"
	@python3.1 -c "import pygame2.test; pygame2.test.run ()"

purge_installs:
	rm -rf /usr/local/include/python2.4/pygame2*
	rm -rf /usr/local/include/python2.5/pygame2*
	rm -rf /usr/local/include/python2.6/pygame2*
	rm -rf /usr/local/include/python3.1/pygame2*
	rm -rf /usr/local/lib/python2.4/site-packages/pygame2*
	rm -rf /usr/local/lib/python2.5/site-packages/pygame2*
	rm -rf /usr/local/lib/python2.6/site-packages/pygame2*
	rm -rf /usr/local/lib/python3.1/site-packages/pygame2*

