# make kern
#     generate ./kern (bach-370-chorales scores with modulations) from
#     annotations/bach-modulations.json -- never committed, always derived.
#
# make kern chor001 chor009
#     generate ./kern for only the given chorales
#
# make kern CHORALES="chor001 chor009"
#     same, via variable instead of extra args
#
# make clean
#     remove the generated ./kern directory

.PHONY: kern clean

CHORALES += $(filter-out kern clean,$(MAKECMDGOALS))

kern:
	$(MAKE) -C chorale-search
	./chorale-search/build/add-modulations \
		annotations/bach-modulations.json \
		bach-370-chorales/kern \
		kern/bach-370-chorales \
		$(CHORALES)

clean:
	rm -rf kern

# lets chorale IDs passed after `kern` (e.g. `make kern chor001`) be picked up
# as $(CHORALES) above instead of make trying to build them as targets.
%:
	@:
