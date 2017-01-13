#!groovy

node('master') {
  try {
    stage('Checkout') {
      clean_checkout()
    }
    stage('Fetch Dependencies') {
      build_conan_package_cach()
    }
      //set conan user home env var
      //make build directory
      // sh 'mkdir build'
      // withEnv(['CONAN_USER_HOME=$WORKSPACE/deps']) {
        //run conan install
        // sh 'cd build && env && PATH=/sbin:$PATH conan install --profile=debug --build=outdated -g env -g txt .. -s compiler=clang -s compiler.version=3.8'
      // }
    // }
    stage('Build') {
      //run conan build
      withEnv(['CONAN_USER_HOME=$WORKSPACE/deps']) {
        sh 'cd build && conan build ..'
      }
      echo 'Build Complete!'
    }
    stage('Test') {
      parallel 'unit tests': {
        sh './mettle ut.*'     
        echo 'Unit Tests Complete!'
      }, 'acceptance tests': {
        sh 'behave features/*.feature'
        echo 'Acceptance Tests Complete!'
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
