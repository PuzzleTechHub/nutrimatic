# See https://docs.conan.io/2/reference/conanfile.html

import conan
import conan.tools.meson

class NutrimaticConan(conan.ConanFile):
    package_type = "application"

    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": False}

    requires = ["openfst/1.8.2", "libxml2/2.12.4", "tre/cci.20230717"]
    tool_requires = ["meson/1.3.1", "ninja/1.11.1"]
    generators = ["MesonToolchain", "PkgConfigDeps"]

    def build(self):
        meson = conan.tools.meson.Meson(self)
        meson.configure(reconfigure=True)
        meson.build()

    def layout(self):
        self.folders.build = "build"
        self.folders.generators = "build/meson"

    def validate(self):
        conan.tools.build.check_min_cppstd(self, 17)
