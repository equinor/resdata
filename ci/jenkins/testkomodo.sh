
_build_libecl () {
    cmake -S . -B build -DBUILD_TESTS=ON -DENABLE_PYTHON=ON -DBUILD_APPLICATIONS=ON -DINSTALL_ERT_LEGACY=ON -DERT_USE_OPENMP=ON
    cmake --build build
}

copy_test_files () {
    cp -r build $CI_TEST_ROOT/build
}

start_tests () {
    echo "Running ctest"
    pushd build
    if [[ $(python -c "import sys;print(sys.version_info.major)") == "3" ]];
    then
        ctest \
            -E "(tests.util_tests.test_version.VersionTest)|(tests.ecl_tests.test_ecl_ecl.EclEclTest)|(tests.legacy_tests.*)" \
            --output-on-failure
    else
        ctest -E tests.util_tests.test_version.VersionTest --output-on-failure
    fi
    popd

}

run_tests () {
    ci_install_cmake
    _build_libecl
    if [ -z "$CI_PR_RUN" ]
    then
        #removing built libs in order to ensure we are using libs from komodo
        rm -rf build/lib/python${CI_PYTHON_VERSION}/site-packages/ert
        rm -rf build/lib/python${CI_PYTHON_VERSION}/site-packages/ecl
        rm -r build/lib64
    fi

    copy_test_files

    install_test_dependencies

    pushd $CI_TEST_ROOT
    start_tests
}
