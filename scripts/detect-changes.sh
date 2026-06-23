#!/bin/sh
set -e
echo "::group::Fetching Project Info"
ALL_PROJECTS="$(make list-projects)"
FULL_REBUILD_PATHS="$(make list-global-deps)"
echo "Project list       = $ALL_PROJECTS"
echo "Full rebuild paths = $FULL_REBUILD_PATHS"
echo "::endgroup::"

needs_full_rebuild() {
  export file=

  export RET_CODE=1
  export TMP_CHANGED_FILES=
  TMP_CHANGED_FILES="$(mktemp)"

  FILE_FULL_REBUILD=
  echo "$CHANGED_FILES" >"$TMP_CHANGED_FILES"
  while IFS= read -r file; do
    for path in $FULL_REBUILD_PATHS; do
      case $file in
      "$path"*)
        FILE_FULL_REBUILD="$FILE_FULL_REBUILD $file"
        RET_CODE=0
        ;;
      esac
    done
  done <"$TMP_CHANGED_FILES"
  if [ "$RET_CODE" = 0 ]; then
    echo "::group::Full rebuild cause"
    echo "$FILE_FULL_REBUILD" | xargs printf '%s\n'
    echo "::endgroup::"
  else
    echo "::group::No Full rebuild needed"
    echo "::endgroup::"
  fi

  return $RET_CODE
}

echo "::group::Getting base ref"
if [ "$GITHUB_EVENT_NAME" = "pull_request" ]; then
  BASE="$MERGE_BASE"
else
  BASE="$GITHUB_BASE_REF"
fi
if [ -z "$BASE" ]; then
  echo "::warning::No base ref found, rebuilding everything"
  echo "::group::Projects to rebuild"
  echo "$PROJECTS" | xargs printf '%s\n'
  echo "::endgroup::"
  echo "projects=$ALL_PROJECTS" >>"$GITHUB_OUTPUT"
  exit 0
fi
echo "Base ref: $BASE"
echo "::endgroup::"

CHANGED_FILES=$(git diff --name-only "$BASE" "$GITHUB_SHA")
echo "::group::Changed Files"
echo "$CHANGED_FILES"
echo "::endgroup::"

PROJECTS=

if needs_full_rebuild; then
  PROJECTS="$ALL_PROJECTS"
else
  PROJECTS=""

  for p in $ALL_PROJECTS; do
    if echo "$CHANGED_FILES" | grep -q "^${p}/"; then
      export PROJECTS="$PROJECTS $p"
    fi
  done

  PROJECTS=$(echo "$PROJECTS" | xargs)
fi

echo "::group::Projects to rebuild"
echo "$PROJECTS" | xargs printf '%s\n'
echo "::endgroup::"

echo "projects=$PROJECTS" >>"$GITHUB_OUTPUT"
