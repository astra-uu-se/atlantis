#! /bin/bash
GIT_REVISION=$(git rev-parse --short "$GITHUB_SHA")
# Get name of the current branch
GIT_BRANCH="${GITHUB_REF#refs/heads/}"
GIT_LINK="$GITHUB_SERVER_URL/$GITHUB_REPOSITORY/commit/$GITHUB_SHA"
HEADER="Run <$GITHUB_SERVER_URL/$GITHUB_REPOSITORY/actions/runs/$GITHUB_RUN_ID|#$GITHUB_RUN_ID> (<$GIT_LINK|$GIT_REVISION>) $GITHUB_REPOSITORY@$GIT_BRANCH by $GITHUB_ACTOR"
echo "$HEADER"
./build/runBenchmarks | python3 slack-formatter.py --header="${HEADER}" --webhook="${SLACK_WEBHOOK_URL}"