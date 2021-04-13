pipeline {
    agent {
	kubernetes {
	    yaml """
apiVersion: v1
kind: Pod
metadata:
  labels:
    some-label: dind-agent
spec:
  containers:
  - name: dind
    image: docker:dind
    args: ["--registry-mirror=https://registry.bansci.com"]
    tty: true
    securityContext:
      privileged: true
    volumeMounts:
      - name: docker-graph-storage
        mountPath: /var/lib/docker
    #A fix for external network access issues, see https://liejuntao001.medium.com/fix-docker-in-docker-network-issue-in-kubernetes-cc18c229d9e5
    args:
    - "--mtu=1440"
  volumes:
    - name: docker-graph-storage
      emptyDir: {}
        """.stripIndent()
	}
    }
    
    stages {
	stage('Fetch docker toolchain image') {
	    steps{
		container('dind') {
		    script {
			dockerBuildImage = docker.image('toastedcrumpets/autopipefit-toolchain')
		    }
		}
	    }
	}
	stage('CMake configure') {
	    steps{
		container('dind') {
		    script {
			dockerBuildImage.inside {
			    dir('build') {
				sh "cmake .."
			    }
			}
		    }
		}
	    }
	}
	stage('Make the C++ library') {
	    steps{
		container('dind') {
		    script {
			dockerBuildImage.inside {
			    dir('build') {
				sh "make -j2"
			    }
			}
		    }
		}
	    }
	}
	stage('Test the C++ library') {
	    steps{
		container('dind') {
		    script {
			dockerBuildImage.inside {
			    dir('build') {
				sh 'ctest -j2 --no-compress-output -T Test'
			    }
			}
		    }
		}
		post {
		    always {
			archiveArtifacts (
			    artifacts: 'build/Testing/**/*.xml',
			    fingerprint: true,
			)
			// Process the CTest xml output with the xUnit plugin
			xunit (
			    testTimeMargin: '3000',
			    thresholdMode: 1,
			    thresholds: [
				skipped(failureThreshold: '0'),
				failed(failureThreshold: '0')
			    ],
			    tools: [CTest(
				pattern: 'build/Testing/**/*.xml',
				deleteOutputFiles: true,
				failIfNotNew: false,
				skipNoTestFiles: true,
				stopProcessingIfError: true
			    )]
			)
		    }
		}
	    }
	}
    }
}
