
# These may be overridden by make invocators
DESTDIR  =
GOSH = gosh
GAUCHE_CONFIG = gauche-config
INSTALL := /usr/bin/install


INSTALL_TYPE = site
HEADER_INSTALL_DIR  = $(DESTDIR)`$(GAUCHE_CONFIG) --$(INSTALL_TYPE)incdir`/
SCM_INSTALL_DIR     = $(DESTDIR)`$(GAUCHE_CONFIG) --$(INSTALL_TYPE)libdir`/
ARCH_INSTALL_DIR    = $(DESTDIR)`$(GAUCHE_CONFIG) --$(INSTALL_TYPE)archdir`/
SCM_HI_INSTALL_DIR =  $(DESTDIR)$$(gauche-config --sitelibdir)
BIN_INSTALL_DIR = /usr/bin


SCMFILES := $(wildcard scm/*.scm)
SCM_HI_FILES := $(wildcard bdb/*scm dbm/*scm)
BINFILES := $(wildcard bin/*.scm)

.PHONY:	test

test :
	@rm -f test.log
	@rmdir ./testdir || : ok
	$(GOSH) -I scm -I . test/test.scm > test.log

install :
	$(INSTALL) -d $(SCM_INSTALL_DIR)
	$(INSTALL) -d $(SCM_HI_INSTALL_DIR)
	@for f in $(SCMFILES) _end; do \
	  if test $$f != _end; then \
	    $(INSTALL) --verbose -m 444 $$f $(SCM_INSTALL_DIR); \
	  fi; \
	done
	@for f in $(SCM_HI_FILES) _end; do \
	  if test $$f != _end; then \
	    $(INSTALL) --verbose -D -m 444 $$f $(SCM_HI_INSTALL_DIR)/$$f; \
	  fi; \
	done
	for p in $(BINFILES); do \
	  $(INSTALL) -v -m 555 $$p $(DESTDIR)$(BIN_INSTALL_DIR) ; \
	done


clean :
	rm -rf core $(TARGET) $(OBJS) *~ test.log so_locations


# 'link' creates symlinks from source tree to extension modules, so that
# it can be tested within the source tree.  'unlink' removes them.
# these are only for developer's.

link : $(TARGET) $(SCMFILES)
	$(GOSH) ../xlink -d gauche -l $(TARGET) $(SCMFILES)

unlink :
	-$(GOSH) ../xlink -d gauche -u $(TARGET) $(SCMFILES)

