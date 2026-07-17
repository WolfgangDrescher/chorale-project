#!/usr/bin/env bash

# Downloads external dependencies pinned in deps.json and extracts them into
# external/<name>/ (stripping the archive's top-level folder so files land at
# e.g. external/humlib/min/humlib.h), then deletes everything except the paths
# listed in that dependency's "keep" array -- so only the files actually used by
# CMakeLists.txt end up on disk.
#
# Usage: scripts/fetch-deps.sh [name ...]   (default: all deps in deps.json)

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEPS_JSON="$ROOT/deps.json"
EXTERNAL="$ROOT/external"

if ! command -v jq >/dev/null 2>&1; then
    echo "Error: this script needs 'jq' (apt install jq / brew install jq)" >&2
    exit 1
fi

# Prunes $1 (a directory) down to just the paths in $2..$n (paths relative to
# $1). A kept path may be a directory (keeps its whole subtree) or a file.
# Ancestor directories of kept paths are preserved even though they aren't
# themselves listed.
#
# Collects the full file list via command substitution (not a live pipe) before
# deleting anything: `find` finishes its filesystem walk first, so deleting an
# early, unwanted directory can't race against `find` still traversing into that
# same subtree (which otherwise prints spurious "No such file or directory" from
# `find` for paths deleted out from under it mid-walk).
prune_to() {
    local root="$1"; shift
    local keep=("$@")
    [ ${#keep[@]} -eq 0 ] && return 0

    local all_paths
    all_paths="$(find "$root" -mindepth 1)"

    while IFS= read -r path; do
        [ -z "$path" ] && continue
        local rel="${path#"$root"/}"
        local keepit=0
        for k in "${keep[@]}"; do
            if [ "$rel" = "$k" ]; then keepit=1; break; fi # exact match
            case "$rel" in "$k"/*) keepit=1; break;; esac  # rel is inside a kept dir
            case "$k" in "$rel"/*) keepit=1; break;; esac  # rel is an ancestor of a kept path
        done

        if [ "$keepit" -eq 0 ]; then
            rm -rf "$path"
        fi
    done <<< "$all_paths"
}

names=("$@")
if [ ${#names[@]} -eq 0 ]; then
    names=()
    while IFS= read -r line; do names+=("$line"); done < <(jq -r 'keys[]' "$DEPS_JSON")
fi

for name in "${names[@]}"; do
    url=$(jq -r --arg n "$name" '.[$n].url' "$DEPS_JSON")
    if [ "$url" = "null" ]; then
        echo "Error: no dependency named '$name' in $DEPS_JSON" >&2
        exit 1
    fi
    keep=()
    while IFS= read -r line; do keep+=("$line"); done < <(jq -r --arg n "$name" '.[$n].keep[]?' "$DEPS_JSON")

    dest="$EXTERNAL/$name"
    marker="$dest/.fetched"
    expected_marker="$url $(IFS=,; echo "${keep[*]:-}")"
    if [ -f "$marker" ] && [ "$(cat "$marker")" = "$expected_marker" ]; then
        echo "== $name: already present at pinned version, skipping =="
        continue
    fi

    echo "== $name: downloading =="
    archive="$(mktemp)"
    trap 'rm -f "$archive"' EXIT
    if ! curl -fsSL -o "$archive" "$url"; then
        echo "Error: download failed for '$name' ($url)" >&2
        echo "  Check network access, or that the URL/tag/commit still exists." >&2
        exit 1
    fi

    rm -rf "$dest"
    mkdir -p "$dest"
    case "$url" in
        *.tar.xz)
            if ! tar -xJf "$archive" -C "$dest" --strip-components=1; then
                echo "Error: extracting '$name' failed (.tar.xz needs xz/liblzma support in 'tar';" >&2
                echo "  if your 'tar' lacks that, switch this dependency's URL in scripts/deps.json" >&2
                echo "  to a .tar.gz source instead, e.g. a codeload.github.com tag/commit archive)." >&2
                exit 1
            fi
            ;;
        *.tar.gz|*tar.gz*)
            if ! tar -xzf "$archive" -C "$dest" --strip-components=1; then
                echo "Error: extracting '$name' failed (corrupt download, or 'tar' issue)." >&2
                exit 1
            fi
            ;;
        *) echo "Error: don't know how to extract '$url'" >&2; exit 1 ;;
    esac
    rm -f "$archive"
    trap - EXIT

    if [ ${#keep[@]} -gt 0 ]; then
        prune_to "$dest" "${keep[@]}"
        echo "== $name: pruned to ${keep[*]} =="
    fi

    echo "$expected_marker" > "$marker"
    echo "== $name: extracted to $dest =="
done

echo "All dependencies fetched."
