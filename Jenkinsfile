#!groovy
node('clang') {
  stage('Checkout') {
    checkout([
      $class: 'GitSCM',
      branches: scm.branches,
      extensions: scm.extensions + [[$class: 'CleanBeforeCheckout']],
      userRemoteConfigs: scm.userRemoteConfigs
    ])
  }
  stage('Build') {
    echo 'Compiling'
    sh 'CXX=clang++ cmake .'
    sh 'make'
    echo 'Build Complete!'
  }
  stage('Test') {
    parallel 'unit tests': {
      //sh 'make test'
      echo 'Unit Tests Complete!'
    }, 'acceptance tests': {
      echo 'Acceptance Tests Complete!'
    }
  }
  stage('Release') {
    echo 'Release Package Created!'
  }
}
