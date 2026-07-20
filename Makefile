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
#
# make fixtures
#     wipe chorale-search/tests/fixtures/ and regenerate it (with
#     modulations applied, same as `make kern`) for the chorales listed in
#     FIXTURE_CHORALES below -- these ARE committed, unlike ./kern

.PHONY: kern clean fixtures

CHORALES += $(filter-out kern clean fixtures,$(MAKECMDGOALS))

FIXTURE_CHORALES := chor001 chor009 chor029

kern:
	$(MAKE) -C chorale-search
	./chorale-search/build/add-modulations \
		annotations/bach-modulations.json \
		bach-370-chorales/kern \
		kern/bach-370-chorales \
		$(CHORALES)

clean:
	rm -rf kern

fixtures:
	$(MAKE) -C chorale-search
	rm -rf chorale-search/tests/fixtures
	mkdir -p chorale-search/tests/fixtures
	./chorale-search/build/add-modulations \
		annotations/bach-modulations.json \
		bach-370-chorales/kern \
		chorale-search/tests/fixtures \
		$(FIXTURE_CHORALES)

# lets chorale IDs passed after `kern` (e.g. `make kern chor001`) be picked up
# as $(CHORALES) above instead of make trying to build them as targets.
%:
	@:
