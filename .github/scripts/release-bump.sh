#!/bin/bash
set -e

# Read the current version
VERSION=$(cat VERSION | tr -d '[:space:]')
echo "Current version in VERSION file: $VERSION"

# Fetch all tags first to ensure we know what exists
git fetch --tags

# Check if tag already exists in the repository
if git rev-parse "v$VERSION" >/dev/null 2>&1; then
    echo "Tag v$VERSION already exists. Bumping version..."
    
    # Parse version parts
    IFS='.' read -r major minor patch <<< "$VERSION"
    
    # Get last commit message
    LAST_COMMIT=$(git log -1 --pretty=%B)
    echo "Last commit message: $LAST_COMMIT"
    
    if echo "$LAST_COMMIT" | grep -q -i "^feat\|^feature"; then
        minor=$((minor + 1))
        patch=0
        echo "Detected feature commit. Bumping MINOR version."
    elif echo "$LAST_COMMIT" | grep -q -i "^fix\|^patch"; then
        patch=$((patch + 1))
        echo "Detected fix/patch commit. Bumping PATCH version."
    else
        # Default fallback bump
        minor=$((minor + 1))
        patch=0
        echo "Defaulting to MINOR version bump."
    fi
    
    NEW_VERSION="$major.$minor.$patch"
    echo "New version: $NEW_VERSION"
    
    # Write back to VERSION
    echo "$NEW_VERSION" > VERSION
    
    # Configure git
    git config --global user.name "github-actions[bot]"
    git config --global user.email "github-actions[bot]@users.noreply.github.com"
    
    # Commit and push changes
    git add VERSION
    git commit -m "chore: bump version to $NEW_VERSION [skip ci]"
    git push origin HEAD:main
    
    # Set output for next steps
    echo "version=$NEW_VERSION" >> $GITHUB_OUTPUT
    echo "was_bumped=true" >> $GITHUB_OUTPUT
else
    echo "Tag v$VERSION does not exist. Using this version directly."
    echo "version=$VERSION" >> $GITHUB_OUTPUT
    echo "was_bumped=false" >> $GITHUB_OUTPUT
fi
