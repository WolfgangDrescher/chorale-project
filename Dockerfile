# syntax=docker/dockerfile:1

# The two services of compose.yaml are the targets of this file:
#
#   tools   runs the command line tools of chorale-search (the search, the
#           segmentation and the check) for the webapp, together with the
#           corpus they read
#   webapp  the Nuxt application, which is a plain Node application: it has
#           none of the C++ tools
#
# Debian is the same in every stage that builds or runs the C++ tools, since a
# binary needs a glibc that is not older than the one it was linked against.
# Change it here, in one place.
ARG DEBIAN=bookworm
ARG NODE=22


# The C++ tools of chorale-search. Downloads the pinned libraries, see
# chorale-search/deps.json.
FROM debian:${DEBIAN}-slim AS toolchain
RUN apt-get update \
    && apt-get install -y --no-install-recommends build-essential cmake jq curl ca-certificates \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /build
COPY chorale-search chorale-search
RUN make -C chorale-search


# ./kern and ./corpus, generated the way `make kern corpus` does.
FROM toolchain AS data
COPY Makefile ./
COPY annotations annotations
COPY bach-370-chorales bach-370-chorales
RUN test -n "$(ls bach-370-chorales/kern/*.krn 2>/dev/null)" \
    || { echo "Error: bach-370-chorales is empty, run 'git submodule update --init'" >&2; exit 1; }
RUN make kern corpus


# The tools service: the endpoints of the webapp that run the tools, as a Nitro
# server of their own.
FROM node:${NODE}-${DEBIAN}-slim AS tools-build
WORKDIR /build/chorale-tools
COPY chorale-tools/package.json chorale-tools/package-lock.json ./
RUN npm ci
COPY chorale-tools/nitro.config.ts ./
COPY chorale-webapp/server ../chorale-webapp/server
RUN npm run build


# The tools service. Laid out like the repository, since the endpoints reach the
# binaries and the corpus by relative paths. Its healthcheck takes any answer,
# an unknown route included.
FROM node:${NODE}-${DEBIAN}-slim AS tools
WORKDIR /app/chorale-tools
COPY --from=toolchain /build/chorale-search/build/chorale-search \
    /build/chorale-search/build/chorale-segment \
    /build/chorale-search/build/chorale-check \
    /app/chorale-search/build/
COPY --from=data /build/corpus /app/corpus
COPY --from=tools-build /build/chorale-tools/.output ./.output
USER node
ENV NODE_ENV=production PORT=4000
EXPOSE 4000
HEALTHCHECK --interval=30s --timeout=5s --start-period=10s \
    CMD node -e "fetch('http://127.0.0.1:4000/').then(() => process.exit(0), () => process.exit(1))"
CMD ["node", ".output/server/index.mjs"]


# The webapp, built with chorale-info (the key, the meter and the length of
# every chorale, for content/chorales). kern comes first as in the Makefile: the
# install runs `nuxt prepare`, and the config points at it.
FROM node:${NODE}-${DEBIAN}-slim AS webapp-build
WORKDIR /build/chorale-webapp
COPY --from=data /build/kern /build/kern
COPY chorale-webapp ./
RUN npm ci
COPY --from=toolchain /build/chorale-search/build/chorale-info /build/chorale-search/build/chorale-info
RUN node scripts/extract-chorales.mjs \
    && npm run build


# The webapp runs on Node alone. Laid out like the repository: the chorale list
# and the export read ../kern. Nuxt Content keeps its database in .data.
FROM node:${NODE}-${DEBIAN}-slim AS webapp
WORKDIR /app/chorale-webapp
COPY --from=data /build/kern /app/kern
COPY --from=webapp-build --chown=node:node /build/chorale-webapp/.output ./.output
RUN mkdir .data && chown node:node .data
USER node
ENV NODE_ENV=production PORT=3000
EXPOSE 3000
HEALTHCHECK --interval=30s --timeout=5s --start-period=20s \
    CMD node -e "fetch('http://127.0.0.1:3000/api/chorales').then((r) => process.exit(r.ok ? 0 : 1), () => process.exit(1))"
CMD ["node", ".output/server/index.mjs"]
