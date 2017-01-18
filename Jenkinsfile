#!groovy

node('worker') {
  try {
    stage('Checkout') {
      clean_checkout()
    }
    stage('Fetch Dependencies') {
      build_conan_package_cache('debug', './targets/x86_64/Ubuntu-16.04', ' -s compiler=clang -s compiler.version=3.8')
    }
    stage('Build') {
      //run conan build
      withEnv(['CONAN_USER_HOME=$WORKSPACE/deps']) {
        sh 'cd build && conan build ..'
      }
      echo 'Build Complete!'
    }
    stage('Test') {
      withEnv(['POLYSYNC_TRANSCODE_LIB=$WORKSPACE/build/plugin:$WORKSPACE', 'PATH=$WORKSPACE/build:$PATH']) {
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
    stage('Tag') {
      tag_with_build_number()
    }
    stage('Clean After') {
      deleteDir()
    }
  }
  catch(Exception e) {
    throw e;
  }
  finally {
    deleteDir()
  }
}
