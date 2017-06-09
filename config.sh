function pre_build {
    if [ ! -n "$IS_OSX" ]; then
        yum install -y lapack-devel blas-devel zlib-devel
    fi
    build_zlib
}

function run_tests {
    sudo apt-get install -y libblas3 liblapack3

    python -c "import ecl; print(ecl.__version__)"
}
