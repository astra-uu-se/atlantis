TRAVIS_BRANCH="develop"
if [ "$TRAVIS_BRANCH" = "master" -o "$TRAVIS_BRANCH" = "develop" ]; then echo "after_script"; fi