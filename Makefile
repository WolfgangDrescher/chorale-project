# make kern
#     generate ./kern (bach-370-chorales scores with modulations) from
#     annotations/bach-modulations.json -- never committed, always derived.
#     This is the corpus the webapp serves to Verovio for rendering, so it
#     carries no analysis spines.
#
# make kern chor001 chor009
#     generate ./kern for only the given chorales
#
# make kern CHORALES="chor001 chor009"
#     same, via variable instead of extra args
#
# make corpus
#     generate ./corpus -- same scores as ./kern, plus the analysis spines
#     (**deg, **mint, **metweight, **fb, **hint-xy) baked in. This is what
#     chorale-search --no-analysis reads, which is what makes a full-corpus
#     search take under a second instead of ~36s. Also never committed.
#     Takes chorale ids the same way `make kern` does.
#
# make clean
#     remove the generated ./kern and ./corpus directories
#
# make fixtures
#     wipe chorale-search/tests/fixtures/ and regenerate it (with
#     modulations applied, same as `make kern`) for the chorales listed in
#     FIXTURE_CHORALES below -- these ARE committed, unlike ./kern
#
# make test
#     forwards to `make -C chorale-search test`

.PHONY: kern corpus clean fixtures test

CHORALES += $(filter-out kern corpus clean fixtures test,$(MAKECMDGOALS))

FIXTURE_CHORALES := chor001 chor006 chor009 chor029

GENERATE := ./chorale-search/build/chorale-generate
MODULATIONS := annotations/bach-modulations.json
SOURCE_KERN := bach-370-chorales/kern

kern:
	$(MAKE) -C chorale-search
	$(GENERATE) $(SOURCE_KERN) kern/bach-370-chorales \
		--modulations $(MODULATIONS) \
		$(CHORALES)

corpus:
	$(MAKE) -C chorale-search
	$(GENERATE) $(SOURCE_KERN) corpus/bach-370-chorales \
		--modulations $(MODULATIONS) \
		--analysis \
		$(CHORALES)

clean:
	rm -rf kern corpus

fixtures:
	$(MAKE) -C chorale-search
	rm -rf chorale-search/tests/fixtures
	mkdir -p chorale-search/tests/fixtures
	$(GENERATE) $(SOURCE_KERN) chorale-search/tests/fixtures \
		--modulations $(MODULATIONS) \
		$(FIXTURE_CHORALES)

test:
	$(MAKE) -C chorale-search test

# lets chorale IDs passed after `kern` (e.g. `make kern chor001`) be picked up
# as $(CHORALES) above instead of make trying to build them as targets.
%:
	@:
