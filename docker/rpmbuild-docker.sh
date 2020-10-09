#!/bin/sh

set -e

PACKAGENAME=bbcollab-libopal
TARGET_OS=${1:-el7}
CMD=${2:-./rpmbuild.sh}

docker build -f $TARGET_OS.Dockerfile --build-arg SPECFILE=$PACKAGENAME.spec -t $PACKAGENAME-$TARGET_OS-build .
docker run -it --rm --mount type=bind,src=$PWD,dst=/host -w /host $PACKAGENAME-$TARGET_OS-build $CMD
