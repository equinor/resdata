#!/usr/bin/env python
import sys
import os
from unittest import TextTestRunner


def runTestCase(tests, verbosity=0):
    test_result = TextTestRunner(verbosity=verbosity).run(tests)

    if len(test_result.errors) or len(test_result.failures):
        test_result.printErrors()
        sys.exit(1)


def update_path():
    for path in os.environ["CTEST_PYTHONPATH"].split(":"):
        sys.path.insert(0 , path)
        
    
if __name__ == '__main__':
    update_path( )
    from ecl.test import ErtTestRunner

    for test_class in sys.argv[1:]:
        tests = ErtTestRunner.getTestsFromTestClass(test_class)
        
        # Set verbosity to 2 to see which test method in a class that fails.
        runTestCase(tests, verbosity=0)
    
