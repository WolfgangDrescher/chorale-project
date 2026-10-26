# Chorale Project

The **Chorale Project** brings together a search engine, a score-based web
application, and annotations for the 370 four-part chorales of Johann Sebastian
Bach. The scores are read from Craig Sapp's Humdrum edition, enriched with
modulation annotations and computed spines (scale degrees, melodic intervals,
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

| Path                 | Description                                                                                                                                     |
| -------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------- |
| `bach-370-chorales/` | Git submodule with the source `**kern` files. Source: [github.com/craigsapp/bach-370-chorales](https://github.com/craigsapp/bach-370-chorales). |
| `annotations/`       | Hand-made annotations, e.g. `bach-modulations.json`.                                                                                            |
| `chorale-search/`    | C++ tools (`chorale-generate`, `chorale-search`, `chorale-segment`, `chorale-check`, `chorale-info`) and their test suite.                      |
| `chorale-webapp/`    | Nuxt application.                                                                                                                               |
| `chorale-tools/`     | Build setup of the tools service for Docker, see [Docker](#docker).                                                                             |
| `kern/`              | Generated. Scores with modulations, served to Verovio for rendering. Not committed.                                                             |
| `corpus/`            | Generated. Same scores plus computed spines, read by the search. Not committed.                                                                 |


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
| `make corpus`  | Generate `corpus/` (same scores plus computed spines). Needed by the search and the segment endpoints. Needs `build`.            |
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


## Docker

`Dockerfile` and `compose.yaml` run the project as two services:

| Service  | Description                                                                                                                                            |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `webapp` | The Nuxt application as a plain Node application. It contains none of the C++ tools. Published on `127.0.0.1:3000` for the reverse proxy of the host.  |
| `tools`  | Runs the search, the segmentation and the check, together with the corpus. Only the webapp can reach it, over a network without access to the outside. |

```sh
docker compose up -d --build   # build the images here
```

Nothing is built on the server. Every push to master runs
`.github/workflows/build.yaml`: if the tests pass, both images are pushed to
ghcr.io as `sha-3f9a1c2` (the commit) and `latest`. Pull requests only run the
tests.

The build needs the `bach-370-chorales` submodule checked out and network
access, for the pinned C++ libraries.

The `tools` service runs the endpoints `chorale-search`, `chorale-segment` and
`chorale-check` of the webapp as a Nitro server of their own
(`chorale-tools/nitro.config.ts`). With `CHORALE_TOOLS_URL` set, the webapp
passes those requests on to it. Without it they run the tools directly, so
`npm run dev` needs no Docker.

`docker/nginx.example.conf` is a starting point for the reverse proxy on the server.

### Server

Clone the repository on the server, without the submodule since the scores are
part of the images, and run the update script. Nothing is built there.

```sh
git clone https://github.com/WolfgangDrescher/chorale-project.git
cd chorale-project
docker/update.sh
```

The script shows what is running and what is new, and asks before it updates to
the newest commit that has images: commits whose tests failed, or whose build is
still running, are skipped. It checks out that commit, so `compose.yaml` and the
images fit together, and starts the containers with it.

```sh
docker/update.sh            # ask, then update to the newest commit with images
docker/update.sh 3f9a1c2    # a rollback, or any other commit with images
docker/update.sh --build    # build the images on the server instead (fetches the submodule)
```

It warns when `docker/nginx.example.conf` differs between the running and the
new commit, since the nginx config of the server is not part of the repository
and has to be adjusted by hand. It says so when the target is older than what is
running (a rollback), when the clone has commits that are not on
`origin/master`, and when the images are not built for the architecture of the
machine (the CI builds `linux/amd64`). Tracked files must not be edited on the
server: settings that belong to it go in `compose.override.yaml` and `.env`,
which git ignores.


## Updating after changes

When the annotations or the scores in the submodule change, regenerate the
derived data:

```sh
git submodule update
make clean
make
```

For changes to the C++ sources, `make build` is enough to rebuild the binaries.
