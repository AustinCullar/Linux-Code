include config.mk

SUBDIRS = $(filter-out include/., $(wildcard */.))

all: $(SUBDIRS)

clean: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) $(MAKECMDGOALS) -C ./$@

.PHONY: all clean $(SUBDIRS)
