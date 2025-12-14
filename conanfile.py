from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout
from conan.tools.files import copy


class LimpConan(ConanFile):
    name = "limp"
    version = "0.1.0"
    license = "MIT"
    author = "thousandmiles"
    url = "https://github.com/thousandmiles/LIMP"
    description = "Lightweight Industrial Messaging Protocol (LIMP) - A compact binary protocol for embedded systems"
    topics = ("protocol", "messaging", "embedded", "industrial")
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_zmq": [True, False]
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_zmq": True
    }
    exports_sources = "CMakeLists.txt", "include/*", "src/*", "examples/*", "tests/*"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def requirements(self):
        if self.options.with_zmq:
            self.requires("zeromq/4.3.5")
            self.requires("cppzmq/4.10.0")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["LIMP_BUILD_ZMQ"] = self.options.with_zmq
        tc.variables["LIMP_BUILD_EXAMPLES"] = False
        tc.variables["LIMP_BUILD_TESTS"] = False
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, "*.hpp", src=self.source_folder, dst=self.package_folder, keep_path=True)
        copy(self, "*.a", src=self.build_folder, dst=self.package_folder, keep_path=False)
        copy(self, "*.so", src=self.build_folder, dst=self.package_folder, keep_path=False)
        copy(self, "*.dylib", src=self.build_folder, dst=self.package_folder, keep_path=False)
        copy(self, "*.dll", src=self.build_folder, dst=self.package_folder, keep_path=False)
        copy(self, "*.lib", src=self.build_folder, dst=self.package_folder, keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["limp"]
        self.cpp_info.includedirs = ["include"]
