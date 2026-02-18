from conan import ConanFile


class ResdataConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("backward-cpp/1.6")
        self.requires("catch2/2.13.9")
        self.requires("fmt/8.0.1")
        self.requires("pybind11/2.13.6")

    def configure(self):
        self.options["catch2"].with_main = True
        if self.settings.os == "Macos":
            self.options["backward-cpp"].stack_details = "backtrace_symbol"
