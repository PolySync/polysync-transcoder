#!groovy

node('worker') {
  try {
    stage('Checkout') {
      clean_checkout()
    }
    stage('Fetch Dependencies') {
      build_conan_package_cache()
    }
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
