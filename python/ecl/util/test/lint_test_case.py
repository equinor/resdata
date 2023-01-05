#!/usr/bin/env python
import sys
import fnmatch
import os
import unittest

try:
    from pylint import epylint as lint
except ImportError:
    sys.stderr.write(
        "Could not import pylint module - lint based testing will be skipped\n"
    )
    lint = None


class LintTestCase(unittest.TestCase):
    """This class is a test case for linting."""

    LINT_ARGS = ["-d", "R,C,W"] + ["--extension-pkg-whitelist=numpy"]

    @staticmethod
    def _get_lintable_files(paths, whitelist=()):
        """Recursively traverses all folders in paths for *.py files"""
        matches = []
        for folder in paths:
            for root, _, filenames in os.walk(folder):
                for filename in fnmatch.filter(filenames, "*.py"):
                    if filename not in whitelist:
                        matches.append(os.path.join(root, filename))
        return matches

    def assertLinted(self, paths, whitelist=()):  # noqa
        """Takes a path to a folder or a list of paths to folders and recursively finds
        all *.py files within that folder except the ones with filenames in whitelist.

        Will assert lint.lint(fname) == 0 for every *.py file found.
        """
        if lint is None:
            self.skipTest("pylint not installed")

        if isinstance(paths, str):
            paths = [paths]
        files = self._get_lintable_files(paths, whitelist=whitelist)
        for f in files:
            self.assertEqual(
                0, lint.lint(f, self.LINT_ARGS), "Linting required for %s" % f
            )
