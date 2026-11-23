#!/usr/bin/env bash
# Prepares a Claude Code cloud session on Ubuntu to build, test and format-check Kinetra as CI
# does: the Debian and Ubuntu packages of docs/development.md#dependencies-and-options and
# CI's clang-format version. .claude/settings.json runs it as a SessionStart hook, and
# Kinetra-Lab's tools/cloud_setup.sh runs it with Lab's own package. See
# tools/README.md#cloud_setupsh.
#
# Usage: tools/cloud_setup.sh [extra apt package...]
#   Does nothing outside a cloud session (CLAUDE_CODE_REMOTE other than "true"). Installs only
#   what is missing, so a session that has everything spends well under a second here. Always
#   exits 0 and prints one line, which Claude Code adds to the agent's context; the output of
#   apt and uv goes to the log it names.
set -uo pipefail

[[ "${CLAUDE_CODE_REMOTE:-}" == "true" ]] || exit 0

# The Debian and Ubuntu package set of docs/development.md, which CI's Ubuntu run installs
# too (with git and ca-certificates, which a cloud session already has). Change all three
# together.
packages=(build-essential cmake ninja-build ccache libsdl2-dev libgl-dev libegl-dev
    libgl1-mesa-dri libegl-mesa0 "$@")
# The clang-format of CI's pinned Arch snapshot (.github/workflows/ci.yml); Ubuntu's own is
# 18. PyPI's clang-format package carries the same release, and pypi.org is on the default
# allowlist of a cloud environment, where LLVM's apt repository is not.
clang_format_version=23.1.5
log="${TMPDIR:-/tmp}/kinetra-cloud-setup.log"

as_root=()
if [[ "$(id -u)" -ne 0 ]]; then
    as_root=(sudo -n)
fi
failures=()
report=()

missing=()
for package in "${packages[@]}"; do
    status="$(dpkg-query -W -f='${Status}' "$package" 2> /dev/null)"
    [[ "$status" == "install ok installed" ]] || missing+=("$package")
done
if [[ ${#missing[@]} -eq 0 ]]; then
    report+=("packages present")
else
    # Only the downloads get a time limit: a stalled mirror would otherwise hold the session's
    # start until the hook's own limit, while stopping dpkg halfway would break the next run.
    # apt-get update succeeds when an index fails to download; Error-Mode=any makes it fail,
    # as in CI, so the failure names the update and not a package it could not find.
    if timeout -k 10 120 "${as_root[@]}" apt-get update -q -o APT::Update::Error-Mode=any \
        >> "$log" 2>&1 &&
        timeout -k 10 300 "${as_root[@]}" apt-get install -y -q --no-install-recommends \
            --download-only "${missing[@]}" >> "$log" 2>&1 &&
        "${as_root[@]}" env DEBIAN_FRONTEND=noninteractive apt-get install -y -q \
            --no-install-recommends "${missing[@]}" >> "$log" 2>&1; then
        report+=("installed ${missing[*]}")
    else
        failures+=("apt-get did not install ${missing[*]}")
    fi
fi

# Sets $found to the version line of the clang-format at $1, or of the one PATH finds.
version_of() {
    hash -r
    found="$("${1:-clang-format}" --version 2> /dev/null)"
}
wanted="clang-format version $clang_format_version"
version_of
if [[ "$found" == *"$wanted"* ]]; then
    report+=("clang-format $clang_format_version present")
elif ! command -v uv > /dev/null; then
    failures+=("uv is missing, so clang-format $clang_format_version is not installed")
elif ! bin="$(uv tool dir --bin 2>> "$log")" || [[ -z "$bin" ]]; then
    failures+=("uv names no tool directory, so clang-format $clang_format_version is not installed")
else
    # A resumed session may have the pinned version in uv's directory while the hook's PATH
    # finds Ubuntu's first; install only when uv's own copy is missing or another version.
    version_of "$bin/clang-format"
    action=""
    if [[ "$found" == *"$wanted"* ]]; then
        action="clang-format $clang_format_version present"
    elif timeout -k 10 120 uv tool install --force "clang-format==$clang_format_version" \
        >> "$log" 2>&1; then
        action="installed clang-format $clang_format_version"
    else
        failures+=("uv did not install clang-format $clang_format_version")
    fi
    if [[ -n "$action" ]]; then
        version_of
        # Ubuntu's clang-format in /usr/bin must not win over the pinned one.
        if [[ "$found" != *"$wanted"* && -n "${CLAUDE_ENV_FILE:-}" ]]; then
            line="export PATH=\"$bin:\$PATH\""
            grep -q -x -F -e "$line" "$CLAUDE_ENV_FILE" 2> /dev/null || echo "$line" >> "$CLAUDE_ENV_FILE"
            PATH="$bin:$PATH"
            version_of
        fi
        if [[ "$found" == *"$wanted"* ]]; then
            report+=("$action")
        else
            failures+=("clang-format $clang_format_version is in $bin, but PATH finds another first")
        fi
    fi
fi

# Joins the arguments with "; ".
join() {
    local IFS=$'\x1f'
    local joined="$*"
    echo "${joined//$'\x1f'/; }"
}
if [[ ${#failures[@]} -eq 0 ]]; then
    echo "Cloud setup: $(join "${report[@]}")."
else
    echo "Cloud setup failed: $(join "${failures[@]}"). Log: $log. By hand:" \
        "apt-get install --no-install-recommends ${packages[*]}; uv tool install clang-format==$clang_format_version."
fi
exit 0
