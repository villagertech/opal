#!/bin/bash
# Build script using standard tools (should be done within a clean VM or
# container to ensure reproducibility, and avoid polluting development environment)

set -ex

SPECFILE=bbcollab-libopal.spec
TARBALL=zsdk-opal.src.tgz
BUILD_ARGS=(--define='_smp_mflags -j4')

if [[ $BUILD_NUMBER ]]; then
    BUILD_ARGS+=(--define="jenkins_release .${BUILD_NUMBER}")
fi

if [[ "$BRANCH_NAME" == "develop" ]]; then
    BUILD_ARGS+=(--define="branch_id 1" --define="opal_stage -beta")
elif [[ "$BRANCH_NAME" == release/* ]]; then
    BUILD_ARGS+=(--define="branch_id 2" --define="opal_stage .")
fi

# Create/clean the rpmbuild directory tree
rpmdev-setuptree
rpmdev-wipetree

# Update the git commit in revision.h (tarball excludes git repo)
export PKG_CONFIG_PATH=/opt/bbcollab/lib64/pkgconfig
./configure     # have to run configure here to ensure the Makefile includes work
make clean
make $(pwd)/revision.h

# Create the source tarball
tar -czf $(rpm --eval "%{_sourcedir}")/$TARBALL --exclude-vcs --exclude=rpmbuild .
cp bbcollab-filter-requires.sh $(rpm --eval "%{_sourcedir}")

# Build the RPM(s)
rpmbuild -ba "${BUILD_ARGS[@]}" $SPECFILE

# Copy the output RPM(s) to the local output directory
mkdir -p rpmbuild
cp -pr $(rpm --eval "%{_rpmdir}") rpmbuild/
