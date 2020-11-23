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
