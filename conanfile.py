from conans import ConanFile, CMake
import sys, os, subprocess

# Conan evaluates the conanfile's "version" attribute twice: once when
# doing "conan export" and again after the conanfile has been copied over
# to the Conan package store. When it happens the second time, the conanfile
# is no longer in a git repo, so the call to "git describe" to get the version
# through the git tag doesn't work.
#
# This function detects whether or not the conanfile is being evaluated from
# a repo (the first time) or the package store (the second time). If from a repo,
# it gets the version via "git describe" and writes the version to a VERSION file.
# If from the package store, it checks the VERSION file.
#
def get_version():

    try:
        conan_data_dir = os.path.join( os.environ["CONAN_USER_HOME"], ".conan" )
    except:
        print( "CONAN_USER_HOME environment variable not set" )
        sys.exit(1)

    conanfile_dir = os.path.dirname( os.path.realpath(__file__) )

    # version being evaluated from cache -- check VERSION file
    if conan_data_dir in conanfile_dir:
        try:
            version_file = os.path.join( conanfile_dir, "VERSION" )
            f = open( version_file, "r" )
            version = f.read()
            f.close()

            print( "Version read from VERSION file" )
        except:
            print( "ERROR - cannot read version from VERSION file" )
            sys.exit(1)

    # version being evaluated from repo -- use 'git describe'
    else:
        try:
            version = subprocess.check_output(["git", "-C", conanfile_dir, "describe"]).strip("\n")

            version_file = os.path.join( conanfile_dir, "VERSION" )
            f = open( version_file, "w+" )
            f.write( version )
            f.close()

            print( "Version read from 'git describe'" )
        except:
            print( "ERROR - cannot read version from 'git describe'" )
            sys.exit(1)

    return version

class Recipe(ConanFile):
    name = "polysync-transcoder"
    version = get_version()
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


