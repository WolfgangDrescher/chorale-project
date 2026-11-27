# AGENTS.md

Guidance for coding agents working in this repository. The README describes the
project for humans; this file adds what an agent needs to change it safely.


## Project at a glance

* `chorale-search/`: C++20 tools on humlib (`chorale-generate`,
  `chorale-search`, `chorale-segment`, `chorale-check`, `chorale-info`,
  `chorale-group`). Library code in `src/`, one entry point per tool in `cli/`,
  tests in `tests/`.
* `chorale-webapp/`: Nuxt 4 with Nuxt UI, Nuxt Content, Pinia, i18n and Verovio.
  The endpoints in `server/api/` run the binaries of `chorale-search/build/`
  through `server/utils/cli-endpoint.js`.
* `chorale-tools/`: Nitro service that runs the same tools in Docker. The webapp
  forwards to it when `CHORALE_TOOLS_URL` is set
  (`server/middleware/tools-proxy.js`).
* `annotations/`: hand-made modulations and the lyrics. Corrections go here,
  never into generated files.
* `bach-370-chorales/`, `kittel-24-chorales/`, `schiorring-choral-bog/`: git
  submodules with the source scores. Do not edit them from here.


## Build and test

```sh
git submodule update --init
make            # build, corpus, kern, webapp, analyses, history, stats, tests
make build      # rebuild the C++ tools after a change in chorale-search/
make test       # run the chorale-search test suite
make test-diminutions        # check the diminution queries against the annotations
make kern chor001 chor009    # regenerate only some chorales
```

The webapp needs `make` to have run first:

```sh
cd chorale-webapp
npm run dev
```

* Run `make test` after every change in `chorale-search/`. CI runs the same
  suite through the `test` target of the `Dockerfile`.
* The tests use the small framework in `tests/test_framework.hpp`. A new test
  file has to be registered in `chorale-search/CMakeLists.txt`.
* A fix in `chorale-search/` comes with a test that fails without it.
* The pinned C++ libraries live in `chorale-search/deps.json`.
* `chorale-search/scripts/fetch-deps.sh` downloads tarballs from
  codeload.github.com. Some sandboxes block that host while `git fetch` from
  github.com works. Then fetch the ref of each `url` in `deps.json` with git
  into `chorale-search/external/<name>/` and write the marker `.fetched` the
  script compares (`<url> <keep joined by commas>`), or it downloads again.
* `make history` needs the full history and stops `make` in a shallow clone
  before the stats and the tests. Run `git fetch --unshallow origin master`
  first.
* A clean `make build` takes about five minutes on four cores.
* There is no linter and no test suite for the webapp. Check a change in the
  webapp in the running app.


## Generated files

Never commit or edit by hand: `kern/`, `corpus/`, `positions/` and in
`chorale-webapp/` the directories `content/bach-chorales/`,
`content/collection-chorales/`, `content/analysis-results/` and the file
`content-history.json`.

The test fixtures in `chorale-search/tests/fixtures/` are generated too but
committed. Regenerate them with `make fixtures` only when the fixture chorales,
the modulations or the generator change, and commit the result.


## Changes that touch several places

* A new or changed query key: `src/Query.hpp`, `src/JsonIO.cpp`,
  `src/QueryValidation.cpp`, the tests, the editor schema in
  `chorale-webapp/app/utils/chorale-search-query-schema.js` and the docs in
  `chorale-webapp/content/docs/`.
* A key of a group query: `src/GroupQuery.hpp`, `src/GroupingJson.cpp`, the
  tests, `chorale-webapp/app/utils/chorale-group-query-schema.js` and
  `chorale-webapp/content/docs/11.grouping.md`.
* A new analysis: a JSON file in `chorale-webapp/analyses/` (see its README),
  named by a lesson in its front matter. `make analyses` runs it with
  `--verify`.
* A question in `content/faq/` states numbers in `stat`. `make stats` checks
  them against the corpus,
  `node chorale-webapp/scripts/check-learn-stats.mjs --write` corrects them.
* An API error uses the error classes of `server/utils/cli-endpoint.js`.


## Content and language

* The docs (`content/docs/`) and the changelog are in English. Lessons, FAQ
  and the interface (`i18n/locales/de*.yaml`) are in German, the only locale.
* Text of the interface goes into `i18n/locales/de.yaml`, of the learn section
  into `de.learn.yaml`, not into the components.
* Wrap the prose of every Markdown file (content, READMEs, this file) at 80
  characters. Tables, code blocks and front matter stay unwrapped.
* Name a chorale with `:chorale{id="chor005"}`, scale degrees with
  `:deg{v="4 5 1" bass}` and figured bass with `:fb{v="6 4 2"}`. The components
  are in `app/components/content/`.
* Show measures of a chorale with `:score-example{chorale-id="chor001"
  measures="2"}`, a score of its own with **kern in the `kern` property of the
  YAML block of `::score-example`.
* Set terms of music theory in another language than the text in italics in
  prose: *quinta syncopata*, *semitonium modi*, *passus duriusculus*. Keys,
  tags and values stay in code: `semitonium-modi`.
* A changelog entry (`content/changelog/<date>.md`) is drafted with
  `node chorale-webapp/scripts/changelog-draft.mjs` and describes what a visitor
  notices, not the commits.


## Code style

* Indent with 4 spaces (`.editorconfig`). JavaScript is ES modules with single
  quotes and semicolons. Vue components use `<script setup>` and plain
  JavaScript.
* Explicit names over comments. A comment is short and says why, not what; two
  lines rather than a paragraph. A function that needs a long comment usually
  needs a different design.
* No comments in Vue `<template>` unless something is truly surprising.
* Do not use the em dash (U+2014) in code, comments, content or commit messages.


## Commits

* One subject line in the imperative that says what changes for the user or the
  code, e.g. "Fix the lyrics of a split note being checked at every onset".
  No prefixes like `feat:`, no trailing period, at most 72 characters.
* Usually no body. Add one only when the reason is not obvious from the subject.
* No fixup commits. Until a commit is merged, correct a mistake in the commit
  it belongs to.
