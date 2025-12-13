SUBDIRS = Reseau_Neuronal image_modifier interface Solver final_result

.PHONY: all clean run $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

run: all
	cd interface && ./interface

clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
