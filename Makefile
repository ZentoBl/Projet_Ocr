SUBDIRS = Reseau_Neuronal image_modifier interface Solver

.PHONY: all clean run $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

run:
	$(MAKE) -C image_modifier run

clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done