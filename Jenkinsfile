pipeline {
  agent {
    node {
      label "master"
      customWorkspace "${JOB_NAME.replaceAll('%2F', '_')}"
    }
  }

  options {
    buildDiscarder logRotator(artifactDaysToKeepStr: '', artifactNumToKeepStr: '', daysToKeepStr: '200', numToKeepStr: '200')
  }

  stages {
    stage('package-el7') {
      // Build environment is defined by the Dockerfile
      agent {
        dockerfile {
          filename 'docker/el7.Dockerfile'
          additionalBuildArgs  '--build-arg SPECFILE=bbcollab-libopal.spec'
          customWorkspace "${JOB_NAME.replaceAll('%2F', '_')}"
        }
      }
      steps {
        // Copy RPM dependencies to the workspace for fingerprinting (see Dockerfile)
        sh 'cp -r /tmp/build-deps .'
        sh './rpmbuild.sh'
      }
      post {
        success {
          fingerprint 'build-deps/*.rpm'
          archiveArtifacts artifacts: 'rpmbuild/**/*', fingerprint: true
          stash name: 'el7_rpms', includes: 'rpmbuild/RPMS/**/*'
        }
      }
    }
    stage('publish') {
      when {
        beforeAgent true
        anyOf {
          branch 'develop'
          branch 'release/*'
        }
      }
      agent { label 'Nexus' }
      options { skipDefaultCheckout() }
      environment {
        BASE_PATH = '/var/www/html/yum'
      }
      steps {
        script {
          def dists = ['el7']
          for (int i = 0; i < dists.size(); ++i) {
            env.REPO = "${dists[i]}/"

            if (env.BRANCH_NAME == 'develop') {
              env.REPO += 'mcu-develop'
            } else {
              env.REPO += 'mcu-release'
            }

            sh "rm -rf rpmbuild"
            unstash "${dists[i]}_rpms"
            sh "aws s3 sync --acl public-read rpmbuild/RPMS/x86_64 s3://citc-artifacts/yum/$REPO/base/"
            sh "mv rpmbuild/RPMS/x86_64/* $BASE_PATH/$REPO/base/"
            // Update Nexus repository metadata
            sh "createrepo --update $BASE_PATH/$REPO"
            sh "aws s3 sync --acl public-read --delete $BASE_PATH/$REPO/repodata s3://citc-artifacts/yum/$REPO/repodata/"
          }
        }
      }
      post {
        success {
          script {
            if (env.BRANCH_NAME == 'develop') {
              build job: "/mcu/develop", quietPeriod: 60, wait: false
            }
          }
        }
      }
    }

    stage('tag-release') {
      when {
        branch 'release/*'
      }
      steps {
        // Set the key to do the git push to "THis is probably Geo's key!"
        sshagent(['9a03ea9a-2af6-4f40-a178-6231e71d8dab']) {
          sh """
            major=`sed -n 's/%global *version_major *//p' bbcollab-libopal.spec`
            minor=`sed -n 's/%global *version_minor *//p' bbcollab-libopal.spec`
            patch=`sed -n 's/%global *version_patch *//p' bbcollab-libopal.spec`
            oem=`  sed -n 's/%global *version_oem *//p'   bbcollab-libopal.spec`
            git tag \$major.\$minor.\$patch.\$oem-2.${BUILD_NUMBER}
            git tag ${env.BRANCH_NAME.replaceAll("release/", "")}-2.${BUILD_NUMBER}
            git push --tags
          """
        }
      }
    }
  }
  post {
    success {
      slackSend color: "good", message: "SUCCESS: <${currentBuild.absoluteUrl}|${currentBuild.fullDisplayName}>"
    }
    failure {
      slackSend color: "danger", message: "FAILURE: <${currentBuild.absoluteUrl}|${currentBuild.fullDisplayName}>"
    }
    unstable {
      slackSend color: "warning", message: "UNSTABLE: <${currentBuild.absoluteUrl}|${currentBuild.fullDisplayName}>"
    }
  }
}
