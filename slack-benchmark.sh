#! /bin/bash
BENCHMARK=./build/runBenchmarks
if [ ! -f "$BENCHMARK" ]; then
  exit 1
fi
if [ -z "$TRAVIS_BUILD_NUMBER" ]; then
  exit 1
fi
BENCHMARK_FILE="benchmark-data.json"
# Get Commit
GIT_COMMIT=`git branch -v | grep \* | sed 's/\(\S*\s*\)\(([^)]*)\)*\S*\s*\([0-9a-z]*\).*/\3/'`
GIT_REVISION=`git branch -v --abbrev=12 | grep \* | sed 's/\(\S*\s*\)\(([^)]*)\)*\S*\s*\([0-9a-z]*\).*/\3/'`
# Get name of repo
GIT_REPO=`git config --get remote.origin.url | sed 's/[^\.]*\.com[\:\/]*\([^\.]*\)\(\.git\)*\(@HEAD\)*/\1/'`
# Get name of the current branch
GIT_BRANCH=`git show -s --pretty=%d HEAD | sed 's/.*,\s*\(origin\/*\)\([^)^,]*\)*[)]*.*/\2/'`
# Get name of the committing git user
GIT_USER=`git show --quiet --pretty=format:%an`
GIT_PREV_REVISION=`git log --skip=1 --max-count=1 | grep commit | sed 's/\s*commit\s*\([0-9a-z]*\).*/\1/'`
GIT_PREV_REVISION=${GIT_PREV_REVISION:0:12}
echo $GIT_PREV_REVISION
GIT_LINK="https://github.com/$GIT_REPO/compare/$GIT_PREV_REVISION...$GIT_REVISION"
HEADER="Build <$TRAVIS_BUILD_WEB_URL|#$TRAVIS_BUILD_NUMBER> (<$GIT_LINK|$GIT_COMMIT>) $GIT_REPO@$GIT_BRANCH by $GIT_USER" > $BENCHMARK_FILE
BENCHMARK_DATA=`$BENCHMARK`
echo "{\"text\":\"${HEADER}\n\`\`\`${BENCHMARK_DATA}\`\`\`\"}" >> $BENCHMARK_FILE
BENCHMARK_FILE_PATH=`realpath $BENCHMARK_FILE`
curl -X POST -H 'Content-type: application/json' --data-binary "@$BENCHMARK_FILE_PATH" https://hooks.slack.com/services/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