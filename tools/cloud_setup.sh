#!/usr/bin/env bash
# Prepares a cloud session on Ubuntu to build, test and format-check Kinetra as CI
# does: the Debian and Ubuntu packages of docs/development.md#dependencies-and-options and
# CI's clang-format version. .claude/settings.json runs it as a SessionStart hook, and
# Kinetra-Lab's tools/cloud_setup.sh runs it with Lab's own package. See
# tools/README.md#cloud_setupsh.
#
# Usage: tools/cloud_setup.sh [--cloud] [extra apt package...]
#   Runs when called with --cloud or in a Claude Code cloud session (CLAUDE_CODE_REMOTE=true).
#   Installs only missing tools and packages and prints one status line when active. An
#   explicit setup reports failure with exit 1. The Claude hook exits 0 to deliver the line.
#   The output of apt and uv goes to the log it names.
set -uo pipefail

fail_on_setup_error=false
if [[ "${1:-}" == "--cloud" ]]; then
    fail_on_setup_error=true
    shift
elif [[ "${CLAUDE_CODE_REMOTE:-}" != "true" ]]; then
    exit 0
fi

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
advice=()

missing=()
for package in "${packages[@]}"; do
    status="$(dpkg-query -W -f='${Status}' "$package" 2> /dev/null)"
    [[ "$status" == "install ok installed" ]] || missing+=("$package")
done
if [[ ${#missing[@]} -eq 0 ]]; then
    report+=("packages present")
else
    # dpkg left halfway, by the hook's limit or a VM reclaimed during an install, would make
    # every later install fail until it is configured.
    if [[ -n "$(dpkg --audit 2> /dev/null)" ]]; then
        "${as_root[@]}" env DEBIAN_FRONTEND=noninteractive dpkg --configure -a >> "$log" 2>&1
    fi
    # apt-get update exits 0 when an index does not download, and that is wanted here: the
    # cloud image also lists sources outside Ubuntu's archive (download.docker.com), while
    # every package this script installs is in the archive. A failed fetch goes into the line.
    # CI's Ubuntu step can fail on it instead (APT::Update::Error-Mode=any), because its
    # container lists only Ubuntu's sources and falls back to the image's.
    update_output="$(timeout -k 10 120 "${as_root[@]}" apt-get update -q 2>&1)"
    update_status=$?
    printf '%s\n' "$update_output" >> "$log"
    index_note=""
    if grep -q -E '^[WE]: (Failed to fetch|Some index files failed to download)' <<< "$update_output"; then
        index_note="an apt index did not download"
    fi
    # Only the downloads get a time limit: a stalled mirror would otherwise hold the session's
    # start until the hook's own limit, while stopping dpkg halfway would break the next run.
    if [[ $update_status -eq 0 ]] &&
        timeout -k 10 300 "${as_root[@]}" apt-get install -y -q --no-install-recommends \
            --download-only "${missing[@]}" >> "$log" 2>&1 &&
        "${as_root[@]}" env DEBIAN_FRONTEND=noninteractive apt-get install -y -q \
            --no-install-recommends "${missing[@]}" >> "$log" 2>&1; then
        report+=("installed ${missing[*]}")
        [[ -z "$index_note" ]] || report+=("$index_note")
    else
        failures+=("apt-get did not install ${missing[*]}")
        [[ -z "$index_note" ]] || failures+=("$index_note")
        advice+=("apt-get update && apt-get install --no-install-recommends ${missing[*]}")
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
    advice+=("pip install clang-format==$clang_format_version into a virtual environment")
elif ! bin="$(uv tool dir --bin 2>> "$log")" || [[ -z "$bin" ]]; then
    failures+=("uv names no tool directory, so clang-format $clang_format_version is not installed")
    advice+=("pip install clang-format==$clang_format_version into a virtual environment")
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
        advice+=("uv tool install --force clang-format==$clang_format_version")
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
            advice+=("put $bin first on PATH")
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
    echo "Cloud setup failed: $(join "${failures[@]}"). Log: $log. By hand: $(join "${advice[@]}")."
    if [[ "$fail_on_setup_error" == "true" ]]; then
        exit 1
    fi
fi
exit 0
