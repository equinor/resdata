function ci_install_cmake {
    pip install cmake ninja

    local root=${KOMODO_ROOT}/${CI_KOMODO_RELEASE}/root

    export CMAKE_GENERATOR=Ninja
    export CMAKE_PREFIX_PATH=$root

    # Colour!
    export CFLAGS="${CFLAGS:-} -fdiagnostics-color=always"
    export CXXFLAGS="${CXXFLAGS:-} -fdiagnostics-color=always"
    export LDFLAGS="${LDFLAGS:-} -fdiagnostics-color=always"

    export LD_LIBRARY_PATH=$root/lib:$root/lib64${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}
}
function ci_install_conan {
    pip install "conan<2"

    # Conan v1 bundles its own certs due to legacy reasons, so we point it
    # to the system's certs instead.
    export CONAN_CACERT_PATH=/etc/pki/tls/cert.pem
}
build_and_run_ctest () {
    pushd $CI_TEST_ROOT
    cmake $CI_SOURCE_ROOT                                  \
        -DBUILD_TESTS=ON                                   \
        -DEQUINOR_TESTDATA_ROOT=$EQUINOR_TESTDATA_ROOT/ecl
    cmake --build .
    ctest --output-on-failure
    popd
    # Reset
    rm -rf $CI_TEST_ROOT
    mkdir -p $CI_TEST_ROOT
}
copy_test_files () {
    pushd $CI_TEST_ROOT
    mkdir -p {.git,python}
    ln -s {$CI_SOURCE_ROOT,$PWD}/bin
    ln -s {$CI_SOURCE_ROOT,$PWD}/lib
    ln -s {$CI_SOURCE_ROOT,$PWD}/test-data
    cp -R {$CI_SOURCE_ROOT,$PWD}/python/tests
    popd
}
run_tests () {
    ci_install_conan
    if [[ ! -z "${CI_PR_RUN:-}" ]]
    then
        pip install .
    fi
    ci_install_cmake
    build_and_run_ctest
    copy_test_files
    install_test_dependencies
    cd $CI_TEST_ROOT
    pytest -vv python/tests
}
