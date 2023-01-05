from ecl.util.test import LintTestCase


class LintErt(LintTestCase):
    """Tests that no file in ert needs linting"""

    def test_lint_ecl(self):
        # white = ['ecl_kw.py', 'ecl_type.py', 'ecl_sum.py', 'ecl_grid.py', 'ecl_npv.py']  # TODO fix issues and remove
        # self.assertLinted('ecl/ecl', whitelist=white)
        pass  # temporarily disable linting due to monkey patching

    def test_lint_geo(self):
        self.assertLinted("ecl/geo")

    def test_lint_util(self):
        self.assertLinted(
            "ecl/util",
            whitelist=[
                "vector_template.py",
                "test_area.py",
                "extended_testcase.py",
                "ert_test_context.py",
                "ert_test_runnet.py",
                "import_test_case.py",
                "lint_test_case.py",
                "path_context.py" "source_enumerator.py",
                "temp_area.py",
                "test_run.py",
            ],
        )

    def test_lint_well(self):
        self.assertLinted("ecl/well")
