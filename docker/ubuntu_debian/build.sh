#!/usr/bin/bash

set -e
set -x

BASEDIR=$(cd $(dirname "$0") && pwd -P)
GITDIR=$(git rev-parse --show-toplevel)

. "${BASEDIR}/../common.sh"

ARCH="X86_64"
DOCKER_IMG="ubuntu"
TAG="latest"

if [[ -n "$2" ]]; then
    TAG="${2##*:}"
    DOCKER_IMG="${2%%:*}"
fi

if [[ "$1" = "X86" ]] || [[ "$1" = "x86" ]]; then
    ARCH="X86"
    DOCKER_IMG="i386/$DOCKER_IMG"
elif [[ "$1" = "ARM" ]] || [[ "$1" = "arm" ]] || [[ "$1" = "arm32" ]]; then
    ARCH="ARM"
    DOCKER_IMG="arm32v7/$DOCKER_IMG"
elif [[ "$1" = "AARCH64" ]] || [[ "$1" = "aarch64" ]] || [[ "$1" = "arm64" ]]; then
    ARCH="AARCH64"
    DOCKER_IMG="arm64v8/$DOCKER_IMG"
fi

CMAKE_ARGUMENT="$3"

DISTRIB="${DOCKER_IMG##*/}"

DOCKER_TAG="qbdi:${ARCH}_${DOCKER_IMG##*/}_${TAG}"

DOCKERFILE="${BASEDIR}/Dockerfile"

prepare_archive

docker build "${BASEDIR}" -t "${DOCKER_TAG}" -f "${DOCKERFILE}" \
             --build-arg DOCKER_IMG="${DOCKER_IMG}:${TAG}" \
             --build-arg QBDI_ARCH="$ARCH" \
             --build-arg CMAKE_ARGUMENT="${CMAKE_ARGUMENT}"

delete_archive

echo "Success build ${DOCKER_TAG}"
