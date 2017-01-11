#!groovy
node('worker') {
  try {
    stage('Clean Before') {
      deleteDir()
    }
    stage('Checkout') {
      checkout([
        $class: 'GitSCM',
        branches: scm.branches,
        extensions: scm.extensions + [[$class: 'CleanBeforeCheckout']],
        userRemoteConfigs: scm.userRemoteConfigs
      ])
    }
    stage('Fetch Dependencies') {
      try {
        sh 'rm -rf deps'
      } finally {}
      // clone conan package cache
      checkout([
        $class: 'GitSCM',
        branches: [[name: '*/devel']],
        extensions: [[$class: 'RelativeTargetDirectory', relativeTargetDir: 'deps']] + [[$class: 'CleanBeforeCheckout']],
        userRemoteConfigs: [[credentialsId: '94447011-c506-4ace-bbd8-a11bae132c69', url: 'git@bitbucket.org:PolySync/conan-package-cache']]
        ])
      //run script to fetch dependencies (manifest)
      sh 'cd deps && ./create-cache.sh ./build-manifests/x86_64/Ubuntu-16.04/cache-packages/devel.txt'
      //set conan user home env var
      //make build directory
      sh 'mkdir build'
      withEnv(['CONAN_USER_HOME=$WORKSPACE/deps']) {
        //run conan install
        sh 'cd build && env && PATH=/sbin:$PATH conan install --profile=debug --build=outdated -g env -g txt .. -s compiler=clang -s compiler.version=3.8'
      }
    }
    stage('Build') {
      //run conan build
      withEnv(['CONAN_USER_HOME=$WORKSPACE/deps']) {
        sh 'cd build && conan build ..'
      }
      echo 'Build Complete!'
    }
    stage('Test') {
      withEnv(['POLYSYNC_TRANSCODE_LIB=$WORKSPACE/build/plugin:$WORKSPACE']) {
        parallel 'unit tests': {
          sh 'mettle ut.*'     
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
