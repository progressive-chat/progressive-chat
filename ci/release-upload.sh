#!/usr/bin/env bash
# SPDX-License-Identifier: AGPL-3.0-only
# Attach build files to a release, rolling to the next release when the
# current one reaches GitHub's per-release asset cap (1000 assets).
#
# Why the count is checked BEFORE upload: `gh release upload --clobber`
# deletes an existing same-name asset first and then uploads; at the 1000
# cap the upload fails with HTTP 422 after the delete, so the old asset is
# gone and the new one never landed. Checking first and rolling avoids that.
#
#   release-upload.sh <base-tag> <prefix> <file...>
#
#   base-tag  the current release tag, e.g. builds-part-1
#   prefix    name for the next release if this one is full, e.g. builds-part
#             (the counter is discovered: builds-part-1 -> builds-part-2)
#   file...   the files to attach (already built)
#
# Env: GH_TOKEN (or gh's own auth), GITHUB_REPOSITORY.
set -euo pipefail

TAG="${1:?base release tag}"
PREFIX="${2:?next-release prefix (e.g. builds-part)}"
shift 2
FILES=("$@")
[ ${#FILES[@]} -gt 0 ] || { echo "no files to upload" >&2; exit 2; }

REPO="${GITHUB_REPOSITORY:?GITHUB_REPOSITORY not set}"
LIMIT=1000

count_assets() {
  gh release view "$TAG" --repo "$REPO" --json assets --jq '.assets | length' 2>/dev/null || echo 0
}

# The next tag in the series: <prefix>-1 -> <prefix>-2, etc. (exists or not).
next_tag() {
  local n base
  base="${TAG%-*}"                 # everything before the last -N
  n="${TAG##*-}"                   # the trailing number
  case "$n" in (*[!0-9]*) echo "${PREFIX}-2";;   # no counter -> start at -2
               (*) echo "${base}-$((n + 1))";; esac
}

ensure_release() {
  # Create the target release (from the same tag's notes) if it is missing.
  if ! gh release view "$1" --repo "$REPO" >/dev/null 2>&1; then
    gh release create "$1" --repo "$REPO" \
      --title "$1" --notes "Build assets continued from ${TAG}." --latest=false
  fi
}

upload_into() {
  local tag="$1"; shift
  local f base
  for f in "$@"; do
    base="$(basename "$f")"
    # A same-name asset is replaced only when the target has room: at the cap
    # we roll instead, so never clobber past the limit.
    local n; n="$(gh release view "$tag" --repo "$REPO" --json assets --jq '.assets | length' 2>/dev/null || echo 0)"
    if [ "$n" -ge "$LIMIT" ]; then
      echo "release $tag is full ($n assets); rolling" >&2
      tag="$(next_tag)"
      ensure_release "$tag"
      n=0
    fi
    echo "upload $base -> $tag ($n/$LIMIT)"
    gh release upload "$tag" "$f" --repo "$REPO" --clobber
  done
}

# Start on the given tag, creating it if needed.
ensure_release "$TAG"
upload_into "$TAG" "${FILES[@]}"
