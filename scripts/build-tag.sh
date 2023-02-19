#!/usr/bin/env bash

DIR=$(realpath $(dirname $0))
TAG="${TAG:-v1.4}"

# Load ini lib
source "${DIR}/ini.sh"

function usage {
  echo ""
  echo "Usage: $(basename $0) [options] <tag>"
  echo ""
  echo "Options:"
  echo "  -h --help  Show this usage"
  echo ""
  echo "Tag:"
  echo ""
  echo "  The default tag configured is '${TAG}', but specifying one will allow"
  echo "  you to load a different configuration to build the tag for."
  echo ""
}

# Parse arguments
while [[ "$#" -gt 0 ]]; do
  case "$1" in
    -h|--help)
      usage
      exit 0
      ;;
    *)
      TAG="$1"
      ;;
  esac
  shift
done

# Ensure tag is given
if [ -z "${TAG}" ]; then
  usage
  exit 1
fi

# Ensure the config for the given tag exists
CFGFILE="${DIR}/../config/package-${TAG}.ini"
if [ ! -f "${CFGFILE}" ]; then
  echo "Configuration for tag '${TAG}' does not exist"
  exit 1
fi

# Make an artifact directory, as a stand-alone git repository
rm -rf "${DIR}/../build"
git init "${DIR}/../build"
git -C "${DIR}/../build" remote add origin "$(git -C "${DIR}/../" remote get-url origin)"
git -C "${DIR}/../build" fetch --all
git -C "${DIR}/../build" checkout "release-${TAG}" || git -C "${DIR}/../build" checkout -b "release-${TAG}"
rm -rf "${DIR}/../build/*"

# Load tarball into directory
TARBALL=$(ini_foreach ini_output_value "${CFGFILE}" "mirror.tarball")
if [ ! -z "${TARBALL}" ]; then
  TARBALLTMP=$(mktemp)
  curl -sSL "${TARBALL}" > "${TARBALLTMP}"
  tar -x -C "${DIR}/../build" -f "${TARBALLTMP}" --strip-components=1
  rm -f "${TARBALLTMP}"
fi

# Handle specific sources
while read srcfg; do
  DST=${srcfg%%=*}
  DST=${DST%%*( )}
  SRC=${srcfg#*=}
  SRC=${SRC##*( )}
  curl -sSL "${SRC}" > "${DIR}/../build/${DST}"
done < <(ini_foreach ini_output_value "${CFGFILE}" "mirror.source")

# Install package.ini into the build directory without mirror config
ini_foreach ini_output_full "${CFGFILE}" \
  | grep -vE '^mirror\.' \
  | ini_write "${DIR}/../build/package.ini"

# Create a commit+tag of this version and push to origin
git -C "${DIR}/../build" add .
git -C "${DIR}/../build" commit -m "Release (update) $(TZ=UTC date '+%Y-%m-%dT%H:%MZ')"
git -C "${DIR}/../build" push -f origin HEAD

# Push only specified tags
while read tag; do
  git -C "${DIR}/../build" tag -f "${tag}"
  git -C "${DIR}/../build" push -f origin "refs/tags/${tag}"
done < <(ini_foreach ini_output_value "${CFGFILE}" "mirror.tag")

# Done
