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

    def build(self):
        cmake = CMake(self.settings)

        self.run( "CXX=clang++ cmake \"%s\"" % self.conanfile_directory )
        self.run( "cmake --build . %s" % cmake.build_config )

    def requirements(self):
        self.requires( "boost/1.63.0@polysync/boost" )
        self.options["boost"].shared = True
        self.requires( "mettle/1.2.12@polysync/mettle" )
        self.parse_reqs()

    def package(self):
        self.copy( "polysync-transcode",
                   dst="bin",
                   src="bin" )

    def parse_reqs( self ):

        reqs = str( self.requires )

        if reqs:
            reqs_list = reqs.split("\n")

            for req in reqs_list:
                name = req.split("@")[0].split("/")[0]
                version = req.split("@")[0].split("/")[1]
                user = req.split("@")[1].split("/")[0]
                channel = req.split("@")[1].split("/")[1]

                try:
                    repos_dir = os.path.join( os.environ["CONAN_USER_HOME"], "repos" )
                except:
                    self.output.error( "CONAN_USER_HOME environment variable not set" )
                    sys.exit(1)

                if not os.path.exists( repos_dir ):
                    os.makedirs( repos_dir )

                package_dir = os.path.join( repos_dir, name, version )

                if os.path.exists( package_dir ):
                    self.output.info( "package repo %s/%s found - updating" % (name, version) )
                    self.run( "git -C %s pull origin %s" % (package_dir, version) )
                else:
                    self.output.info( "package repo %s/%s not found - cloning" % (name, version) )
                    self.run( "git lfs clone --depth=1 --branch=%s git@bitbucket.org:PolySync/%s %s" % (version, name, package_dir) )

                self.run( "conan export %s/%s --path %s" % (user, channel, package_dir) )


