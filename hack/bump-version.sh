#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
#
# bump-version.sh - bump the tina version in all version files.
#
# Usage:
#   hack/bump-version.sh 0.1.15          # set an explicit version
#   hack/bump-version.sh --part patch    # bump patch (default), minor or major
#
# After bumping, review the changes (`git diff`) and commit them.
# The tag (with a leading 'v', e.g. v0.1.15) is created automatically
# when the drafted GitHub release is published.

set -eu

cd "$(dirname "$0")/.."

FILES="configure.ac rust/Cargo.toml python/src/tina_mgr/defs.py"

# --- determine old version (from configure.ac, the C canonical source) ---

old_version=$(sed -n 's/^AC_INIT(tina, \([0-9][0-9.]*\)).*$/\1/p' configure.ac)
case "$old_version" in
  [0-9]*.[0-9]*.[0-9]*) ;;
  *) echo "error: cannot parse version from configure.ac (got: '$old_version')" >&2; exit 1 ;;
esac

# --- determine new version ---

case "${1:-}" in
  ""|--part)
    part="${2:-patch}"
    [ "${1:-}" = "--part" ] || part="${1:-patch}"
    case "$part" in
      major|minor|patch) ;;
      *) echo "error: --part must be major, minor or patch" >&2; exit 1 ;;
    esac
    major=$(echo "$old_version" | cut -d. -f1)
    minor=$(echo "$old_version" | cut -d. -f2)
    patch=$(echo "$old_version" | cut -d. -f3)
    case "$part" in
      major) major=$((major + 1)); minor=0; patch=0 ;;
      minor) minor=$((minor + 1)); patch=0 ;;
      patch) patch=$((patch + 1)) ;;
    esac
    new_version="$major.$minor.$patch"
    ;;
  -*)
    echo "error: unknown option: $1" >&2; exit 1 ;;
  *)
    new_version="$1"
    case "$new_version" in
      v*) echo "error: pass the version without the 'v' prefix (e.g. 0.1.15)" >&2; exit 1 ;;
      [0-9]*.[0-9]*.[0-9]*) ;;
      *) echo "error: version must look like X.Y.Z (got: '$new_version')" >&2; exit 1 ;;
    esac
    ;;
esac

if [ "$new_version" = "$old_version" ]; then
  echo "error: new version equals current version ($old_version)" >&2; exit 1
fi

# --- rewrite the three version files ---

sed -i.bak -E "s/^AC_INIT\(tina, [0-9.]+\)$/AC_INIT(tina, $new_version)/" configure.ac
sed -i.bak -E "/^\[package\]/,/^\[/ s/^version = \"[0-9.]+\"$/version = \"$new_version\"/" rust/Cargo.toml
sed -i.bak -E "s/^VERSION: Final = \"[0-9.]+\"$/VERSION: Final = \"$new_version\"/" python/src/tina_mgr/defs.py

rm -f configure.ac.bak rust/Cargo.toml.bak python/src/tina_mgr/defs.py.bak

# --- verify ---

fail=0
grep -q "AC_INIT(tina, $new_version)" configure.ac || { echo "FAIL: configure.ac"; fail=1; }
grep -q "version = \"$new_version\"" rust/Cargo.toml || { echo "FAIL: rust/Cargo.toml"; fail=1; }
grep -q "VERSION: Final = \"$new_version\"" python/src/tina_mgr/defs.py || { echo "FAIL: defs.py"; fail=1; }
[ "$fail" -eq 0 ] || exit 1

echo "Bumped version: $old_version -> $new_version in:"
echo "  - configure.ac"
echo "  - rust/Cargo.toml"
echo "  - python/src/tina_mgr/defs.py"
echo
echo "Next steps:"
echo "  1. review:  git diff"
echo "  2. commit:  git commit -am 'Bump version to $new_version'"
echo "  3. push, then publish the drafted release as v$new_version on GitHub"
