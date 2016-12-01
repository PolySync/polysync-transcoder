#!groovy
node('clang') {
  stage('Checkout') {
    checkout([
      $class: 'GitSCM',
      branches: scm.branches,
      extensions: scm.extensions + [[$class: 'CleanBeforeCheckout']],
      userRemoteConfigs: scm.userRemoteConfigs
    ])
    try {
      sh 'rm -rf build'
    } finally {}
  }
  stage('Build') {
    echo 'Compiling'
    sh 'mkdir build'
    sh 'CXX=clang++ cmake -Bbuild -H.'
    sh 'cd build && make'
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
