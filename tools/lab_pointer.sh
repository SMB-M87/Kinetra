#!/usr/bin/env bash
# Moves Kinetra-Lab's submodule pointer to Kinetra main: opens Lab's pointer-only pull request,
# or adds a commit to the one that is open, and turns on auto-merge, so GitHub merges it once
# Lab's CI passes. .github/workflows/lab-pointer.yml runs it after each push to main, hourly
# and by hand; the steps it replaces are in docs/workflow.md#changes-across-kinetra-and-lab.
#
# Usage: tools/lab_pointer.sh [--dry-run]
#   --dry-run  read everything and say what it would do, but write nothing.
# Environment:
#   LAB_TOKEN      a token that may write contents and pull requests in Kinetra-Lab (the
#                  workflow passes the LAB_POINTER_TOKEN secret); without it, gh's own login.
#   KINETRA_TOKEN  a token that reads Kinetra (the workflow's own); without it, gh's own login.
#   KINETRA_REPO, LAB_REPO  default SMB-M87/Kinetra and SMB-M87/Kinetra-Lab.
# Needs bash, gh and jq.
#
# It writes nothing when Lab main already records Kinetra main, and nothing while another open
# Lab pull request changes external/Kinetra: that pull request records the Kinetra commit it was
# verified with and merges first, and a later run catches up. Every API failure stops it with a
# non-zero status, so a failed run shows in Kinetra's Actions instead of passing as "nothing to do".
set -euo pipefail

kinetra_repo=${KINETRA_REPO:-SMB-M87/Kinetra}
lab_repo=${LAB_REPO:-SMB-M87/Kinetra-Lab}
branch=chore/kinetra-pointer
dry_run=false
case ${1:-} in
    --dry-run) dry_run=true ;;
    '') ;;
    *) echo "usage: $0 [--dry-run]" >&2; exit 2 ;;
esac

# Each repository is called with its own token: the Lab token may be limited to Kinetra-Lab.
as_lab() { if [[ -n ${LAB_TOKEN:-} ]]; then GH_TOKEN=$LAB_TOKEN "$@"; else "$@"; fi; }
as_kinetra() { if [[ -n ${KINETRA_TOKEN:-} ]]; then GH_TOKEN=$KINETRA_TOKEN "$@"; else "$@"; fi; }

# Prints a line, and adds it to the run's summary on GitHub.
note() {
    echo "$*"
    if [[ -n ${GITHUB_STEP_SUMMARY:-} ]]; then echo "$*" >> "$GITHUB_STEP_SUMMARY"; fi
}

target=$(as_kinetra gh api "repos/$kinetra_repo/commits/main" --jq .sha)
current=$(as_lab gh api "repos/$lab_repo/contents/external/Kinetra?ref=main" --jq .sha)
if [[ $current == "$target" ]]; then
    note "Lab main already records Kinetra main ${target:0:7}: nothing to do."
    exit 0
fi

# Kinetra main only moves forward. Anything else, such as a pointer to a squashed branch
# commit, needs a person.
status=$(as_kinetra gh api "repos/$kinetra_repo/compare/$current...$target" --jq .status)
if [[ $status != ahead ]]; then
    echo "::error title=Pointer not behind Kinetra main::Lab records ${current:0:7}, which is not an ancestor of Kinetra main ${target:0:7} (compare: $status). Move the pointer by hand."
    exit 1
fi

# The open pull request of this script, if any.
own=$(as_lab gh api "repos/$lab_repo/pulls?state=open&head=${lab_repo%%/*}:$branch" --jq '.[0].number // empty')

# Wait while another pull request moves the pointer. The file list is read into a variable
# first: grep -q in a pipe could end gh early, and under pipefail that would read as "no".
numbers=$(as_lab gh api --paginate "repos/$lab_repo/pulls?state=open&per_page=100" --jq '.[].number')
for number in $numbers; do
    [[ $number == "$own" ]] && continue
    files=$(as_lab gh api --paginate "repos/$lab_repo/pulls/$number/files?per_page=100" --jq '.[].filename')
    if grep -q -x 'external/Kinetra' <<< "$files"; then
        note "Waiting: $lab_repo#$number changes external/Kinetra and merges first; a later run moves the pointer to ${target:0:7}."
        exit 0
    fi
done

# The Kinetra pull requests the move brings, for the description.
merged=$(as_kinetra gh api "repos/$kinetra_repo/compare/$current...$target" --jq '
    .commits[] | .commit.message | select(test("^Merge pull request #[0-9]+"))
    | split("\n") | "- Kinetra#\(.[0] | capture("#(?<n>[0-9]+)").n): \(.[2] // "")"')
run=""
if [[ -n ${GITHUB_RUN_ID:-} ]]; then
    run=" ([run](${GITHUB_SERVER_URL}/${GITHUB_REPOSITORY}/actions/runs/${GITHUB_RUN_ID}))"
fi
title="chore(external): record Kinetra main at ${target:0:7}"
body="Pointer-only: \`external/Kinetra\` ${current:0:7} → ${target:0:7}, Kinetra \`main\`.

${merged:-- (no merged pull requests listed)}

Opened by Kinetra's \`lab-pointer\` workflow${run}. GitHub merges it with a merge commit once Lab's CI passes; it needs no agent review. A red CI here is fixed like any pointer move."

if [[ -n $own ]]; then
    base=$(as_lab gh api "repos/$lab_repo/git/ref/heads/$branch" --jq .object.sha)
    have=$(as_lab gh api "repos/$lab_repo/contents/external/Kinetra?ref=$base" --jq .sha)
    plan="add a commit to $lab_repo#$own, which records ${have:0:7}"
else
    base=$(as_lab gh api "repos/$lab_repo/git/ref/heads/main" --jq .object.sha)
    have=$current
    plan="open a pull request from $branch"
fi

if [[ $dry_run == true ]]; then
    note "Dry run: would $plan, moving the pointer from ${current:0:7} to Kinetra main ${target:0:7}, and turn on auto-merge."
    note "$merged"
    exit 0
fi

if [[ $have != "$target" ]]; then
    # One commit that changes only the gitlink, made through the API, so no clone is needed.
    tree=$(as_lab gh api "repos/$lab_repo/git/commits/$base" --jq .tree.sha)
    new_tree=$(jq -n --arg base "$tree" --arg sha "$target" \
        '{base_tree: $base, tree: [{path: "external/Kinetra", mode: "160000", type: "commit", sha: $sha}]}' |
        as_lab gh api "repos/$lab_repo/git/trees" --input - --jq .sha)
    commit=$(jq -n --arg message "$title" --arg tree "$new_tree" --arg parent "$base" \
        '{message: $message, tree: $tree, parents: [$parent]}' |
        as_lab gh api "repos/$lab_repo/git/commits" --input - --jq .sha)
    if [[ -n $own ]]; then
        # The new commit's parent is the branch head, so this is a fast-forward.
        as_lab gh api -X PATCH "repos/$lab_repo/git/refs/heads/$branch" -f sha="$commit" -F force=false > /dev/null
    else
        # A branch left by a closed pull request is this script's own; start it again from main.
        if as_lab gh api "repos/$lab_repo/git/ref/heads/$branch" > /dev/null 2>&1; then
            as_lab gh api -X DELETE "repos/$lab_repo/git/refs/heads/$branch"
        fi
        as_lab gh api "repos/$lab_repo/git/refs" -f ref="refs/heads/$branch" -f sha="$commit" > /dev/null
    fi
fi

if [[ -n $own ]]; then
    number=$own
    as_lab gh api -X PATCH "repos/$lab_repo/pulls/$number" -f title="$title" -f body="$body" > /dev/null
else
    number=$(as_lab gh api "repos/$lab_repo/pulls" -f title="$title" -f head="$branch" -f base=main \
        -f body="$body" --jq .number)
fi
as_lab gh pr merge "$number" --repo "$lab_repo" --auto --merge
note "$lab_repo#$number records Kinetra main ${target:0:7}; auto-merge is on."
