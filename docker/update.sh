#!/bin/sh
# Updates the containers on the server. Run it in the clone of the repository.
#
#   docker/update.sh            update to the newest commit with images
#   docker/update.sh <commit>   update (or roll back) to that commit
#   docker/update.sh --build    build the images here instead of pulling them
#                               (fetches the bach-370-chorales submodule)
#
# The images of a commit exist once .github/workflows/build.yaml has tested and
# built it. The compose.yaml of that commit is checked out with it, so the two
# always fit together. Server-specific settings belong in compose.override.yaml
# and .env, which git ignores.
set -eu

NGINX_TEMPLATE=docker/nginx.example.conf

# The commit that .env says is running, or nothing.
deployed_commit() {
    [ -f .env ] || return 0
    version=$(sed -n 's/^CHORALE_VERSION=//p' .env)
    case $version in
        sha-*) git rev-parse --verify --quiet "${version#sha-}^{commit}" || true ;;
    esac
}

short() {
    git rev-parse --short=7 "$1"
}

describe() {
    git log -1 --format='%h %s' "$1"
}

# Whether the images of both services exist for a commit.
images_exist() {
    for image in $(CHORALE_VERSION="sha-$(short "$1")" docker compose config --images); do
        docker manifest inspect "$image" > /dev/null 2>&1 || return 1
    done
}

# The architecture the images have to be built for: the one of the Docker
# server, or the one DOCKER_DEFAULT_PLATFORM asks for.
platform_arch() {
    if [ -n "${DOCKER_DEFAULT_PLATFORM:-}" ]; then
        echo "${DOCKER_DEFAULT_PLATFORM#*/}" | cut -d/ -f1
    else
        docker version --format '{{.Server.Arch}}'
    fi
}

# The architectures an image is built for, separated by commas. Empty when the
# registry does not say.
image_archs() {
    docker manifest inspect "$1" 2> /dev/null \
        | grep -oE '"architecture": *"[a-z0-9]+"' \
        | sed 's/.*"\([a-z0-9]*\)"$/\1/' \
        | grep -v '^unknown$' | sort -u | paste -sd, -
}

# Fails with a message when the images of a commit do not fit this machine.
check_platform() {
    arch=$(platform_arch)
    for image in $(CHORALE_VERSION="sha-$(short "$1")" docker compose config --images); do
        archs=$(image_archs "$image")
        case ",$archs," in
            ,,|*,"$arch",*) ;;
            *)
                echo "Error: the images of $(short "$1") are built for $archs, not for $arch." >&2
                echo "  Build them here with --build, or run them with" >&2
                echo "  DOCKER_DEFAULT_PLATFORM=linux/${archs%%,*} docker/update.sh" >&2
                return 1
                ;;
        esac
    done
}

# Whether the nginx template differs between two commits.
nginx_changed() {
    [ -n "$1" ] && ! git diff --quiet "$1" "$2" -- "$NGINX_TEMPLATE"
}

main() {
    build=0
    wanted=
    for arg in "$@"; do
        case $arg in
            --build) build=1 ;;
            -*) echo "Unknown option: $arg" >&2; exit 2 ;;
            *) wanted=$arg ;;
        esac
    done

    cd "$(dirname "$0")/.."

    if [ -n "$(git status --porcelain --untracked-files=no --ignore-submodules=all)" ]; then
        echo "Error: tracked files have local changes, which would be lost." >&2
        echo "  Keep server settings in compose.override.yaml and .env." >&2
        exit 1
    fi
    git fetch --quiet origin master

    current=$(deployed_commit)
    here=$(git rev-parse HEAD)
    if [ -n "$current" ]; then
        echo "Running: $(describe "$current")"
    else
        echo "Running: nothing deployed yet"
    fi
    [ "$here" = "$current" ] || echo "Checked out: $(describe "$here")"

    if [ -n "$wanted" ]; then
        target=$(git rev-parse --verify --quiet "$wanted^{commit}") || {
            echo "Error: unknown commit $wanted" >&2
            exit 1
        }
        if [ "$build" -eq 0 ] && ! images_exist "$target"; then
            echo "Error: no images for $(short "$target") (tests failed, or the build is still running)." >&2
            exit 1
        fi
    elif [ "$build" -eq 1 ]; then
        target=$(git rev-parse origin/master)
    else
        target=
        for candidate in $(git rev-list --max-count=30 origin/master); do
            if images_exist "$candidate"; then
                target=$candidate
                break
            fi
        done
        if [ -z "$target" ]; then
            echo "Error: none of the last 30 commits of master has images." >&2
            exit 1
        fi
        skipped=$(git rev-list --count "$target..origin/master")
        [ "$skipped" -eq 0 ] || echo "Skipped: $skipped newer commit(s) without images (tests failed, or the build is still running)"
    fi

    if [ "$target" = "$current" ] && [ "$target" = "$here" ]; then
        echo "Already up to date."
        exit 0
    fi

    if [ "$build" -eq 0 ]; then
        check_platform "$target" || exit 1
    fi

    # What the target is, compared with what is running (or else checked out).
    base=${current:-$here}
    kind=Update
    if [ "$target" = "$base" ]; then
        echo "Target:  $(describe "$target")"
    elif git merge-base --is-ancestor "$base" "$target"; then
        echo "Update:  $(describe "$target")"
        echo "Changes:"
        git log --format='  %h %s' "$base..$target"
    elif git merge-base --is-ancestor "$target" "$base"; then
        kind="Roll back"
        echo "Rollback: $(describe "$target")"
        echo "WARNING: $(short "$target") is older than $(short "$base"). Left behind:"
        git log --format='  %h %s' "$target..$base"
    else
        kind=Switch
        echo "Switch:  $(describe "$target")"
        echo "WARNING: $(short "$target") and $(short "$base") have diverged."
    fi

    unpushed=$(git rev-list --count origin/master..HEAD)
    if [ "$unpushed" -gt 0 ] && [ "$target" != "$here" ]; then
        echo "WARNING: $unpushed commit(s) of this clone are not on origin/master (not pushed)."
    fi

    changed_nginx=0
    if nginx_changed "$current" "$target"; then
        changed_nginx=1
        echo ""
        echo "WARNING: $NGINX_TEMPLATE changed. Check the nginx config of this server:"
        git diff "$current" "$target" -- "$NGINX_TEMPLATE"
    fi

    printf '%s to %s? [y/N] ' "$kind" "$(short "$target")"
    read -r answer
    case $answer in
        y|Y|j|J) ;;
        *) echo "Cancelled."; exit 0 ;;
    esac

    git checkout --quiet --detach "$target"
    # The version goes into .env only when the update has worked.
    export CHORALE_VERSION="sha-$(short "$target")"
    if [ "$build" -eq 1 ]; then
        git submodule update --init --quiet
        docker compose up -d --build --wait
    else
        docker compose pull
        docker compose up -d --wait
    fi
    echo "CHORALE_VERSION=$CHORALE_VERSION" > .env
    docker image prune --force > /dev/null

    echo "Now running: $(describe "$target")"
    if [ "$changed_nginx" -eq 1 ]; then
        echo ""
        echo "WARNING: $NGINX_TEMPLATE changed. Apply it to the nginx config of this server."
    elif [ -z "$current" ]; then
        echo ""
        echo "First deployment: set up nginx with $NGINX_TEMPLATE."
    fi
}

# On one line, so the shell has read it all before git replaces this file.
main "$@"; exit $?
