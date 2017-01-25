#!groovy
default_conan_build('debug', './targets/x86_64/Ubuntu-16.04', ' -s compiler=clang -s compiler.version=3.8') {
  stage('Test') {
    withEnv(["POLYSYNC_TRANSCODE_LIB=$WORKSPACE/build/plugin:$WORKSPACE", "PATH=$WORKSPACE/build:$PATH"]) {
      parallel 'unit tests': {
        sh 'cd build && make run-unit-tests'
        echo 'Unit Tests Complete!'
      }, 'acceptance tests': {
        sh 'behave features'
        echo 'Acceptance Tests Complete!'
      }
    }
  }
  stage('Release') {
    echo 'No rules for Release Package.  Release Package Created!'
  }
}
