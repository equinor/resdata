from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain


class ResdataConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    options = {"build_tests": [True, False]}
    default_options = {"build_tests": False}

    def requirements(self):
        self.requires("backward-cpp/1.6")
        self.requires("fmt/8.0.1")
        self.requires("pybind11/3.0.1")

        if self.options.build_tests:
            self.requires("catch2/3.14.0")

    def configure(self):
        if self.options.build_tests:
            self.options["catch2"].with_main = True
        if self.settings.compiler == "clang":
            self.options["backward-cpp"].stack_details = "backtrace_symbol"

    def generate(self):
        tc = CMakeToolchain(self)
        tc.cache_variables["BUILD_TESTS"] = "ON" if self.options.build_tests else "OFF"
        tc.generate()
        CMakeDeps(self).generate()
