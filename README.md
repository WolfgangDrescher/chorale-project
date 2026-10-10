# Chorale Project

The **Chorale Project** brings together a search engine, a score-based web
application, and annotations for the 370 four-part chorales of Johann Sebastian
Bach. The scores are read from Craig Sapp's Humdrum edition, enriched with
modulation annotations and analysis spines (scale degrees, melodic intervals,
metric weights, figured bass), and can be searched with structured pattern
queries. The web application renders the chorales with Verovio and exposes the
search, the segmentation of a score into queryable patterns, and a check for
part-writing errors such as parallel fifths and octaves.


## Technologies

* [Humdrum](https://www.humdrum.org/) (`**kern`) and [humlib](https://github.com/craigsapp/humlib) for reading and analysing the scores.
* C++17 and CMake for the command line tools in `chorale-search`.
* [Nuxt 4](https://nuxt.com/) and [Vue 3](https://vuejs.org/) as the application framework.
* [Nuxt UI](https://ui.nuxt.com/) for UI components and Tailwind utility classes.
* [Nuxt Content](https://content.nuxt.com/) for the per-chorale YAML metadata.
* [Verovio](https://www.verovio.org/) and [vue-verovio-canvas](https://github.com/WolfgangDrescher/vue-verovio-canvas) for score rendering.


## Repository overview

| Path                  | Description                                                                                                                                              |
| --------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bach-370-chorales/`  | Git submodule with the source `**kern` files. Source: [github.com/craigsapp/bach-370-chorales](https://github.com/craigsapp/bach-370-chorales).         |
| `annotations/`        | Hand-made annotations, e.g. `bach-modulations.json`.                                                                                                     |
| `chorale-search/`     | C++ tools (`chorale-generate`, `chorale-search`, `chorale-segment`, `chorale-check`) and their test suite.                                              |
| `chorale-webapp/`     | Nuxt application.                                                                                                                                        |
| `kern/`               | Generated. Scores with modulations, served to Verovio for rendering. Not committed.                                                                     |
| `corpus/`             | Generated. Same scores plus analysis spines, read by the search. Not committed.                                                                         |


## Requirements

* A C++17 compiler, CMake (3.16 or newer), and `make`
* `jq`, `curl` and `tar` (used to download the pinned C++ dependencies)
* Node.js and npm (a current LTS release)


## Project setup

```sh
git clone --recurse-submodules <repository-url>
cd chorale-project
make
cd chorale-webapp
npm run dev
```

If the repository was cloned without `--recurse-submodules`, fetch the scores
first:

```sh
git submodule update --init
```

The development server runs on `http://localhost:3000`.

`make` creates everything that is needed: the compiled tools, `kern/`, `corpus/`
and the chorale metadata for the web application. What exactly is created and in which order is
described in [Make commands](#make-commands) below.


## Make commands

`make` without arguments is the same as `make all` and runs everything in the
right order. The individual steps depend on each other, so you rarely need to
call them yourself:

| Command        | Description                                                                                                                      |
| -------------- | -------------------------------------------------------------------------------------------------------------------------------- |
| `make build`   | Download the pinned C++ libraries and build the `chorale-search` binaries into `chorale-search/build/`.                          |
| `make kern`    | Generate `kern/` (scores with modulations) from `annotations/bach-modulations.json`. Needs `build`.                              |
| `make corpus`  | Generate `corpus/` (same scores plus analysis spines). Needed by the search and the segment endpoints. Needs `build`.            |
| `make webapp`  | Install the npm dependencies and generate `chorale-webapp/content/chorales/` (one YAML file per chorale). Needs `kern`.          |
| `make test`    | Run the `chorale-search` test suite.                                                                                             |
| `make fixtures`| Regenerate the committed test fixtures in `chorale-search/tests/fixtures/`. Not part of `make`, see below.                      |

The order that `make all` follows is `build`, `corpus`, `kern`, `webapp`, `test`.
The fixtures are deliberately not part of `make`: they are committed, and the
tests read them as they are. They only need to be regenerated when the list of
fixture chorales, the modulation annotations or the generator change, and the
result should then be committed.

To generate only some chorales, pass their ids:

```sh
make kern chor001 chor009
make corpus chor001 chor009
```

Cleaning up:

| Command            | Description                                                          |
| ------------------ | -------------------------------------------------------------------- |
| `make clean`       | Remove `kern/`, `corpus/` and `chorale-webapp/content/chorales/`.    |
| `make clean-build` | Remove the `chorale-search` build directory.                         |
| `make clean-deps`  | Remove the downloaded C++ libraries.                                 |
| `make distclean`   | Everything above, back to a freshly cloned tree.                     |


## Web application

All commands in this section run in `chorale-webapp/`. The generated files from
`make` have to exist first, otherwise the chorale list is empty and the search
endpoints cannot find the binaries and the corpus.

```sh
cd chorale-webapp
npm run dev       # development server on http://localhost:3000
npm run build     # production build
npm run preview   # preview the production build
npm run generate  # static generation
```

The server endpoints in `chorale-webapp/server/api/` call the binaries in
`chorale-search/build/` and read `corpus/`, so the app has to be started on a
machine where `make` has been run. The scores for rendering are served from
`kern/`.


## Updating after changes

When the annotations or the scores in the submodule change, regenerate the
derived data:

```sh
git submodule update
make clean
make
```

For changes to the C++ sources, `make build` is enough to rebuild the binaries.
