#!groovy
node('worker') {
  stage('Checkout') {
    checkout([
      $class: 'GitSCM',
      branches: scm.branches,
      extensions: scm.extensions + [[$class: 'CleanBeforeCheckout']],
      userRemoteConfigs: scm.userRemoteConfigs
    ])
    try {
      sh 'rm -rf clang-debug'
    } finally {}
  }
  stage('CLang Debug Build') {
    echo 'Compiling'
    sh 'mkdir clang-debug'
    sh 'CXX=clang++ cmake -Bclang-debug -H. -DJENKINS=1 -DCMAKE_BUILD_TYPE=Debug'
    sh 'cd clang-debug && make'
    echo 'Build Complete!'
  }
  stage('Clang Test') {
    parallel 'unit tests': {
      
      echo 'Unit Tests Complete!'
    }, 'acceptance tests': {
      sh 'behave *.feature'
      echo 'Acceptance Tests Complete!'
    }
  }
  stage('GCC Debug Build') {
    echo 'Compiling'
    sh 'mkdir gcc-debug'
    sh 'CXX=g++ cmake -Bclang-debug -H. -DJENKINS=1 -DCMAKE_BUILD_TYPE=Debug'
    sh 'cd gcc-debug && make'
    echo 'Build Complete!'
  }
  stage('GCC Test') {
    parallel 'unit tests': {
      
      echo 'Unit Tests Complete!'
    }, 'acceptance tests': {
      sh 'behave *.feature'
      echo 'Acceptance Tests Complete!'
    }
  }
  stage('Release') {
    echo 'Release Package Created!'
  }
}
