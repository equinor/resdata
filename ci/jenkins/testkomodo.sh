copy_test_files () {
    pushd $CI_TEST_ROOT
    mkdir -p {.git,python}
    ln -s {$CI_SOURCE_ROOT,$PWD}/bin
    ln -s {$CI_SOURCE_ROOT,$PWD}/lib
    ln -s {$CI_SOURCE_ROOT,$PWD}/test-data
    cp -R {$CI_SOURCE_ROOT,$PWD}/python/tests
    popd
}