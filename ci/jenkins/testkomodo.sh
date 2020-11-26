build_and_run_ctest () {
    pushd $CI_TEST_ROOT
    cmake $CI_SOURCE_ROOT  -DBUILD_TESTS=ON
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
    ln -s -T $EQUINOR_TEST_DATA_ROOT  test-data/Equinor
    cp -R {$CI_SOURCE_ROOT,$PWD}/python/tests
    popd
}
run_tests () {
    if [[ ! -z "${CI_PR_RUN:-}" ]]
    then
        pip install .
    fi
    ci_install_cmake
    build_and_run_ctest
    copy_test_files
    install_test_dependencies
    cd $CI_TEST_ROOT
    pytest -vv
}
