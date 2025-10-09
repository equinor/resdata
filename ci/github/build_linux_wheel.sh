#!/bin/bash
set -euo pipefail

case "$1" in
    3.11) pyver=cp311-cp311 ;;
    3.12) pyver=cp312-cp312 ;;
    3.13) pyver=cp313-cp313 ;;
    3.14) pyver=cp314-cp314 ;;
    *)
        echo "Unknown Python version $1"
        exit 1
        ;;
esac

# Install dependencies
yum install -y zlib-devel

git config --global --add safe.directory /github/workspace

# Build wheel
cd /github/workspace
/opt/python/$pyver/bin/pip wheel . --no-deps -w wheelhouse
auditwheel repair wheelhouse/* -w dist
