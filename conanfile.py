# See https://docs.conan.io/2/reference/conanfile.html

import conan
import conan.tools.meson
import conan.tools.files

class NutrimaticConan(conan.ConanFile):
    name = "nutrimatic"
    version = "0.1"
    package_type = "application"

    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": False}

    requires = ["openfst/1.8.2", "libxml2/2.12.4", "tre/cci.20230717"]
    tool_requires = ["meson/1.7.2", "ninja/1.12.1"]
    generators = ["MesonToolchain", "PkgConfigDeps", "VirtualBuildEnv"]

    exports_sources = "source/*"
    no_copy_source = True

    def validate(self):
        conan.tools.build.check_min_cppstd(self, 17)

    def layout(self):
        self.folders.source = "source"
        self.folders.build = "build"
        self.folders.generators = "build/dep-info"

    def build(self):
        meson = conan.tools.meson.Meson(self)
        meson.configure(reconfigure=True)
        meson.build()

    def package(self):
        meson = conan.tools.meson.Meson(self)
        meson.install()
        source_root, output_dir = self.recipe_folder, self.package_folder
        conan.tools.files.copy(self, "cgi_scripts/*", source_root, output_dir)
        conan.tools.files.copy(self, "web_static/*", source_root, output_dir)
