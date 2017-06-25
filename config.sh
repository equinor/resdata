function pre_build {
    if [ ! -n "$IS_OSX" ]; then
        yum install -y lapack-devel blas-devel zlib-devel
    fi
    build_zlib
}

function run_tests {
    if [ ! -n "$IS_OSX" ]; then
        sudo apt-get install -y libblas3 liblapack3
    fi

    python -c "import ecl; print(ecl.__version__)"
}
