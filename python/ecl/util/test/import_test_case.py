import importlib
import inspect
import os
import unittest


class ImportTestCase(unittest.TestCase):
    def import_module(self, module):
        return importlib.import_module(module)

    def import_package(self, package):
        if "__" in package:
            return True
        module = self.import_module(package)

        path = os.path.dirname(inspect.getfile(module))

        for entry in sorted(os.listdir(path)):
            entry_path = os.path.join(path, entry)
            if os.path.isdir(entry_path):
                module = os.path.basename(entry)
                sub_module = f"{package}.{module}"
                self.import_package(sub_module)
            else:
                module, ext = os.path.splitext(entry)
                if module == "__init__":
                    continue

                if ext == "py":
                    self.import_module(f"{package}.{module}")

        return True
