run_tests() {
    ln -f -s $EQUINOR_TESTDATA_ROOT/resdata test-data/Equinor
    pip install --group test
    pytest -vv
}
