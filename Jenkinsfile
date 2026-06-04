pipeline {
    agent any

    options {
        buildDiscarder(logRotator(numToKeepStr: '10', artifactNumToKeepStr: '5'))
        disableConcurrentBuilds()
        skipDefaultCheckout(true)
        timestamps()
    }

    environment {
        BUILD_DIR = 'build'
        TEST_EXECUTABLE = './build/rvc_controller_tests'
        GTEST_XML = 'build/gtest-results.xml'
        COVERAGE_XML = 'build/coverage.xml'
        COVERAGE_HTML = 'build/coverage.html'

        CLANG_TIDY_CHECKS = 'clang-analyzer-*,bugprone-*,performance-*'
        CLANG_TIDY_REPORT = 'build/clang-tidy.txt'
        CLANG_TIDY_FIXES = 'build/clang-tidy-fixes.yaml'
        CPPCHECK_TEXT = 'build/cppcheck.txt'
        CPPCHECK_XML = 'build/cppcheck.xml'

        SONAR_QUALITY_GATE_TIMEOUT_MINUTES = '30'
        SONAR_TOKEN = credentials('sonar-token')
        SONAR_ORG = 'ooad-team3'
        SONAR_PROJECT_KEY = 'OOAD-Team3_Robot-Vacuum-Cleaner-AI'

        DISCORD_WEBHOOK = credentials('discord-webhook')
    }

    stages {
        stage('Checkout') {
            steps {
                checkout scm
            }
            post {
                success {
                    discordSend(
                        webhookURL: env.DISCORD_WEBHOOK,
                        title: 'Checkout Success',
                        description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\nStage: Checkout\n상태: 성공",
                        link: env.BUILD_URL,
                        result: 'SUCCESS'
                    )
                }
                failure {
                    discordSend(
                        webhookURL: env.DISCORD_WEBHOOK,
                        title: 'Checkout Failed',
                        description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\nStage: Checkout\n상태: 실패",
                        link: env.BUILD_URL,
                        result: 'FAILURE'
                    )
                }
            }
        }

        stage('Configure') {
            steps {
                sh '''
                    cmake -S . -B ${BUILD_DIR} \
                      -DCMAKE_BUILD_TYPE=Debug \
                      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
                      -DCMAKE_CXX_FLAGS="--coverage" \
                      -DCMAKE_C_FLAGS="--coverage"
                '''
            }
            post {
                success {
                    discordSend(
                        webhookURL: env.DISCORD_WEBHOOK,
                        title: 'Configure Success',
                        description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\nStage: Configure\n상태: 성공",
                        link: env.BUILD_URL,
                        result: 'SUCCESS'
                    )
                }
                failure {
                    discordSend(
                        webhookURL: env.DISCORD_WEBHOOK,
                        title: 'Configure Failed',
                        description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\nStage: Configure\n상태: 실패",
                        link: env.BUILD_URL,
                        result: 'FAILURE'
                    )
                }
            }
        }

        stage('Build') {
            steps {
                sh 'cmake --build ${BUILD_DIR}'
            }
            post {
                success {
                    discordSend(
                        webhookURL: env.DISCORD_WEBHOOK,
                        title: 'Build Success',
                        description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\nStage: Build\n상태: 성공",
                        link: env.BUILD_URL,
                        result: 'SUCCESS'
                    )
                }
                failure {
                    discordSend(
                        webhookURL: env.DISCORD_WEBHOOK,
                        title: 'Build Failed',
                        description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\nStage: Build\n상태: 실패",
                        link: env.BUILD_URL,
                        result: 'FAILURE'
                    )
                }
            }
        }

        stage('Test') {
            steps {
                sh '''
                    rm -f ${GTEST_XML}
                    ${TEST_EXECUTABLE} --gtest_output=xml:${GTEST_XML}
                '''
            }
            post {
                always {
                    junit allowEmptyResults: false, testResults: 'build/gtest-results.xml'
                }
                success {
                    discordSend(
                        webhookURL: env.DISCORD_WEBHOOK,
                        title: 'Test Success',
                        description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\nStage: Test\n상태: 성공",
                        link: env.BUILD_URL,
                        result: 'SUCCESS'
                    )
                }
                failure {
                    discordSend(
                        webhookURL: env.DISCORD_WEBHOOK,
                        title: 'Test Failed',
                        description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\nStage: Test\n상태: 실패",
                        link: env.BUILD_URL,
                        result: 'FAILURE'
                    )
                }
            }
        }

        stage('Clang-Tidy') {
            steps {
                sh '''
                    command -v clang-tidy >/dev/null 2>&1 || {
                      echo "clang-tidy is not installed on this Jenkins agent."
                      exit 1
                    }

                    rm -f ${CLANG_TIDY_REPORT} ${CLANG_TIDY_FIXES}
                    FILES=$(find src app tests -type f \\( -name '*.cpp' -o -name '*.cc' -o -name '*.cxx' \\))

                    if [ -z "$FILES" ]; then
                      echo "No C++ source files found for clang-tidy analysis." > ${CLANG_TIDY_REPORT}
                      : > ${CLANG_TIDY_FIXES}
                      exit 0
                    fi

                    clang-tidy \
                    -p ${BUILD_DIR} \
                    -checks="${CLANG_TIDY_CHECKS}" \
                    -header-filter='(app|include|src|tests)/.*' \
                    -system-headers=false \
                    -export-fixes=${CLANG_TIDY_FIXES} \
                    $FILES > ${CLANG_TIDY_REPORT} 2>&1 || true

                    cat ${CLANG_TIDY_REPORT}
                '''
            }
            post {
                always {
                    archiveArtifacts artifacts: 'build/clang-tidy.txt, build/clang-tidy-fixes.yaml', allowEmptyArchive: true
                }
                success {
                    script {
                        discordSend(
                            webhookURL: env.DISCORD_WEBHOOK,
                            title: 'Clang-Tidy Success',
                            description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\nStage: Clang-Tidy\n상태: 성공",
                            link: env.BUILD_URL,
                            result: 'SUCCESS'
                        )

                        if (fileExists(env.CLANG_TIDY_REPORT)) {
                            discordSend(
                                webhookURL: env.DISCORD_WEBHOOK,
                                title: 'Clang-Tidy Report',
                                description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\nStage: Clang-Tidy\n파일: clang-tidy.txt",
                                link: env.BUILD_URL,
                                result: 'SUCCESS',
                                customFile: env.CLANG_TIDY_REPORT
                            )
                        }

                        if (fileExists(env.CLANG_TIDY_FIXES)) {
                            discordSend(
                                webhookURL: env.DISCORD_WEBHOOK,
                                title: 'Clang-Tidy Fixes',
                                description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\nStage: Clang-Tidy\n파일: clang-tidy-fixes.yaml",
                                link: env.BUILD_URL,
                                result: 'SUCCESS',
                                customFile: env.CLANG_TIDY_FIXES
                            )
                        }
                    }
                }
                failure {
                    discordSend(
                        webhookURL: env.DISCORD_WEBHOOK,
                        title: 'Clang-Tidy Failed',
                        description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\nStage: Clang-Tidy\n상태: 실패",
                        link: env.BUILD_URL,
                        result: 'FAILURE'
                    )
                }
            }
        }

        stage('Cppcheck') {
            steps {
                sh '''
                    command -v cppcheck >/dev/null 2>&1 || {
                      echo "cppcheck is not installed on this Jenkins agent."
                      exit 1
                    }

                    rm -f ${CPPCHECK_TEXT} ${CPPCHECK_XML}

                    cppcheck \
                      --project=${BUILD_DIR}/compile_commands.json \
                      --enable=warning,style,performance,portability \
                      --std=c++17 \
                      --language=c++ \
                      --inline-suppr \
                      --suppress=missingIncludeSystem \
                      --suppress=*:build/_deps/* \
                      -i${BUILD_DIR}/_deps \
                      2> ${CPPCHECK_TEXT} || true

                    cat ${CPPCHECK_TEXT}

                    cppcheck \
                      --project=${BUILD_DIR}/compile_commands.json \
                      --enable=warning,style,performance,portability \
                      --std=c++17 \
                      --language=c++ \
                      --inline-suppr \
                      --suppress=missingIncludeSystem \
                      --suppress=*:build/_deps/* \
                      -i${BUILD_DIR}/_deps \
                      --xml \
                      --xml-version=2 \
                      2> ${CPPCHECK_XML} || true
                '''
            }
            post {
                always {
                    archiveArtifacts artifacts: 'build/cppcheck.txt, build/cppcheck.xml', allowEmptyArchive: true
                }
                success {
                    script {
                        discordSend(
                            webhookURL: env.DISCORD_WEBHOOK,
                            title: 'Cppcheck Success',
                            description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\nStage: Cppcheck\n상태: 성공",
                            link: env.BUILD_URL,
                            result: 'SUCCESS'
                        )

                        if (fileExists(env.CPPCHECK_TEXT)) {
                            discordSend(
                                webhookURL: env.DISCORD_WEBHOOK,
                                title: 'Cppcheck Text Report',
                                description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\nStage: Cppcheck\n파일: cppcheck.txt",
                                link: env.BUILD_URL,
                                result: 'SUCCESS',
                                customFile: env.CPPCHECK_TEXT
                            )
                        }

                        if (fileExists(env.CPPCHECK_XML)) {
                            discordSend(
                                webhookURL: env.DISCORD_WEBHOOK,
                                title: 'Cppcheck XML Report',
                                description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\nStage: Cppcheck\n파일: cppcheck.xml",
                                link: env.BUILD_URL,
                                result: 'SUCCESS',
                                customFile: env.CPPCHECK_XML
                            )
                        }
                    }
                }
                failure {
                    discordSend(
                        webhookURL: env.DISCORD_WEBHOOK,
                        title: 'Cppcheck Failed',
                        description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\nStage: Cppcheck\n상태: 실패",
                        link: env.BUILD_URL,
                        result: 'FAILURE'
                    )
                }
            }
        }

        stage('Publish Static Analysis') {
            steps {
                recordIssues(
                    enabledForFailure: true,
                    failOnError: true,
                    tools: [
                        clangTidy(
                            id: 'clang-tidy',
                            name: 'Clang-Tidy',
                            pattern: 'build/clang-tidy.txt'
                        )
                    ],
                    sourceCodeEncoding: 'UTF-8',
                    trendChartType: 'AGGREGATION_TOOLS'
                )

                recordIssues(
                    enabledForFailure: true,
                    failOnError: true,
                    tools: [
                        cppCheck(
                            id: 'cppcheck',
                            name: 'Cppcheck',
                            pattern: 'build/cppcheck.xml'
                        )
                    ],
                    sourceCodeEncoding: 'UTF-8',
                    trendChartType: 'AGGREGATION_TOOLS'
                )
            }
            post {
                success {
                    discordSend(
                        webhookURL: env.DISCORD_WEBHOOK,
                        title: 'Static Analysis Publish Success',
                        description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\nStage: Publish Static Analysis\n상태: 성공",
                        link: env.BUILD_URL,
                        result: 'SUCCESS'
                    )
                }
                failure {
                    discordSend(
                        webhookURL: env.DISCORD_WEBHOOK,
                        title: 'Static Analysis Publish Failed',
                        description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\nStage: Publish Static Analysis\n상태: 실패",
                        link: env.BUILD_URL,
                        result: 'FAILURE'
                    )
                }
            }
        }

        stage('Coverage') {
            steps {
                sh '''
                    gcovr -r . \
                      --sonarqube ${COVERAGE_XML} \
                      --exclude "${BUILD_DIR}/.*" \
                      --exclude '.*/_deps/.*' \
                      --exclude 'app/rvc_app/main[.]cpp' \
                      --exclude 'src/net/.*' \
                      --exclude 'src/protocol/.*' \
                      --exclude 'src/sim/.*'

                    gcovr -r . \
                      --html-details ${COVERAGE_HTML} \
                      --exclude "${BUILD_DIR}/.*" \
                      --exclude '.*/_deps/.*' \
                      --exclude 'app/rvc_app/main[.]cpp' \
                      --exclude 'src/net/.*' \
                      --exclude 'src/protocol/.*' \
                      --exclude 'src/sim/.*'
                '''
            }
            post {
                always {
                    archiveArtifacts artifacts: 'build/gtest-results.xml, build/coverage.xml, build/coverage.html', allowEmptyArchive: true, fingerprint: true
                }
                success {
                    discordSend(
                        webhookURL: env.DISCORD_WEBHOOK,
                        title: 'Coverage Success',
                        description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\nStage: Coverage\n상태: 성공",
                        link: env.BUILD_URL,
                        result: 'SUCCESS'
                    )
                }
                failure {
                    discordSend(
                        webhookURL: env.DISCORD_WEBHOOK,
                        title: 'Coverage Failed',
                        description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\nStage: Coverage\n상태: 실패",
                        link: env.BUILD_URL,
                        result: 'FAILURE'
                    )
                }
            }
        }

        stage('SonarQube Analysis') {
            steps {
                withSonarQubeEnv('SonarQubeCloud') {
                    sh '''
                        test -f ${BUILD_DIR}/compile_commands.json
                        test -f ${COVERAGE_XML}

                        sonar-scanner \
                        -Dsonar.organization=${SONAR_ORG} \
                        -Dsonar.projectKey=${SONAR_PROJECT_KEY} \
                        -Dsonar.sources=src,include \
                        -Dsonar.tests=tests \
                        -Dsonar.cfamily.compile-commands=${BUILD_DIR}/compile_commands.json \
                        -Dsonar.coverageReportPaths=${COVERAGE_XML} \
                        -Dsonar.coverage.exclusions=src/net/**,src/protocol/**,src/sim/**,app/rvc_app/main.cpp \
                        -Dsonar.token=${SONAR_TOKEN}
                    '''
                }
            }
            post {
                success {
                    discordSend(
                        webhookURL: env.DISCORD_WEBHOOK,
                        title: 'SonarQube Analysis Success',
                        description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\nStage: SonarQube Analysis\n상태: 성공",
                        link: env.BUILD_URL,
                        result: 'SUCCESS'
                    )
                }
                failure {
                    discordSend(
                        webhookURL: env.DISCORD_WEBHOOK,
                        title: 'SonarQube Analysis Failed',
                        description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\nStage: SonarQube Analysis\n상태: 실패",
                        link: env.BUILD_URL,
                        result: 'FAILURE'
                    )
                }
            }
        }
    }

    post {
        success {
            echo 'Build, test, static analysis, and coverage completed successfully.'
            discordSend(
                webhookURL: env.DISCORD_WEBHOOK,
                title: 'Pipeline Success',
                description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\n전체 파이프라인 성공",
                link: env.BUILD_URL,
                result: 'SUCCESS'
            )
        }
        failure {
            echo 'Pipeline failed. Check Console Output.'
            discordSend(
                webhookURL: env.DISCORD_WEBHOOK,
                title: 'Pipeline Failed',
                description: "${env.JOB_NAME} #${env.BUILD_NUMBER}\n전체 파이프라인 실패",
                link: env.BUILD_URL,
                result: 'FAILURE'
            )
        }
        always {
            cleanWs()
        }
    }
}
