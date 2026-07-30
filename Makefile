# make
# make all
#     the whole project: ./kern, ./corpus, and the test suite. The
#     chorale-search build the three of them need runs exactly once, because
#     they share it as the `build` prerequisite below rather than each
#     invoking $(MAKE) -C chorale-search for themselves
#
# make build
#     just the chorale-search binaries (forwards to `make -C chorale-search`).
#     Rarely needed on its own -- kern, corpus, fixtures and test all pull it in
#
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
#     remove the generated ./kern and ./corpus directories -- only what this
#     Makefile itself produces. The chorale-search build survives, so the
#     next `make kern` does not recompile it; use clean-build for that
#
# make clean-build
#     forwards to `make -C chorale-search clean` (removes its build/)
#
# make clean-deps
#     forwards to `make -C chorale-search clean-deps` (removes its
#     external/, so the next build downloads the pinned libraries again)
#
# make distclean
#     clean plus everything chorale-search generates, i.e. back to a freshly
#     cloned tree. Leaves chorale-webapp/ alone -- it has no make targets here
#
# make fixtures
#     wipe chorale-search/tests/fixtures/ and regenerate it (with
#     modulations applied, same as `make kern`) for the chorales listed in
#     FIXTURE_CHORALES below -- these ARE committed, unlike ./kern
#
# make test
#     run the chorale-search test suite (after `build`, via its run-tests
#     target -- so it does not configure and build a second time)

# Single source of truth for "this is a target, not a chorale id". The catch-all
# rule at the bottom means anything missing from this list would silently be
# passed to chorale-generate as a chorale id instead.
TARGETS := all build kern corpus clean clean-build clean-deps distclean fixtures test

.PHONY: $(TARGETS)
.DEFAULT_GOAL := all

CHORALES += $(filter-out $(TARGETS),$(MAKECMDGOALS))

FIXTURE_CHORALES := chor001 chor005 chor006 chor008 chor009 chor029 chor039 chor103

GENERATE := ./chorale-search/build/chorale-generate
MODULATIONS := annotations/bach-modulations.json
SOURCE_KERN := bach-370-chorales/kern

all: build kern corpus test

build:
	$(MAKE) -C chorale-search

kern: build
	$(GENERATE) $(SOURCE_KERN) kern/bach-370-chorales \
		--modulations $(MODULATIONS) \
		$(CHORALES)

corpus: build
	$(GENERATE) $(SOURCE_KERN) corpus/bach-370-chorales \
		--modulations $(MODULATIONS) \
		--analysis \
		$(CHORALES)

clean:
	rm -rf kern corpus

clean-build:
	$(MAKE) -C chorale-search clean

clean-deps:
	$(MAKE) -C chorale-search clean-deps

distclean: clean
	$(MAKE) -C chorale-search distclean

fixtures: build
	rm -rf chorale-search/tests/fixtures
	mkdir -p chorale-search/tests/fixtures
	$(GENERATE) $(SOURCE_KERN) chorale-search/tests/fixtures \
		--modulations $(MODULATIONS) \
		$(FIXTURE_CHORALES)

test: build
	$(MAKE) -C chorale-search run-tests

# lets chorale IDs passed after `kern` (e.g. `make kern chor001`) be picked up
# as $(CHORALES) above instead of make trying to build them as targets.
%:
	@:
