from conans import ConanFile, CMake

class Recipe(ConanFile):
    name = "polysync-transcoder"
    version = "0.1"
    author = "PolySync"
    license = "MIT"
    url = "https://github.com/PolySync/polysync-transcoder"

    settings = "os", "compiler", "build_type", "arch", "target_platform"
    exports = ( "polysync-transcode", "share", "plugin" )
    generators = "cmake"

    options = ""
    default_options = ""

    requires = ( "boost/1.63.0@polysync/boost", "mettle/1.2.12@polysync/mettle" )

    def build(self):
        cmake = CMake(self.settings)

        self.run( "CXX=clang++ cmake \"%s\"" % self.conanfile_directory )
        self.run( "cmake --build . %s" % cmake.build_config )

    def package(self):
        self.copy( "polysync-transcode",
                   dst="bin",
                   src="bin" )

