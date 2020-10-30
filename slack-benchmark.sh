BENCHMARK=./build/runBenchmarks
if [ ! -f "$BENCHMARK" ]; then
  exit 1
fi
BENCHMARK_DATA=`$BENCHMARK`
BENCHMARK_FILE="benchmark-data.json"
HEADER="Build #$TRAVIS_BUILD_NUMBER (`git branch -v | grep \* | sed 's/\S*\s*\S*\s\(\w*\).*/\1/'`) `git config --get remote.origin.url | sed 's/[^\.]*\.com[\:\/]*\([^\.]*\)\(\.git\)*\(@HEAD\)*/\1/'`@`git rev-parse --abbrev-ref HEAD` by `git show --quiet --pretty=format:%an`" > $BENCHMARK_FILE
echo "{\"text\":\"${HEADER}\n\`\`\`${BENCHMARK_DATA}\`\`\`\"}" >> $BENCHMARK_FILE
BENCHMARK_FILE_PATH=`realpath $BENCHMARK_FILE`
#curl -X POST -H 'Content-type: application/json' --data-binary "@$BENCHMARK_FILE_PATH" https://hooks.slack.com/services/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