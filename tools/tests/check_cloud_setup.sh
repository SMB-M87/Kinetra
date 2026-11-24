#!/usr/bin/env bash
set -euo pipefail

setup="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/cloud_setup.sh"
scratch="$(mktemp -d)"
trap 'rm -rf -- "$scratch"' EXIT

export PATH="$scratch:$PATH" TMPDIR="$scratch"
export PACKAGE_LOG="$scratch/packages" PACKAGE_STATUS='install ok installed'
export CLANG_FORMAT_VERSION="$(sed -n 's/^clang_format_version=//p' "$setup")"
cat > "$scratch/dpkg-query" <<'EOF'
#!/usr/bin/env bash
printf '%s\n' "${@: -1}" >> "$PACKAGE_LOG"
printf '%s\n' "$PACKAGE_STATUS"
EOF
cat > "$scratch/clang-format" <<'EOF'
#!/usr/bin/env bash
printf 'clang-format version %s\n' "$CLANG_FORMAT_VERSION"
EOF
printf '#!/usr/bin/env bash\nexit 1\n' > "$scratch/apt-get"
printf '#!/usr/bin/env bash\nexit 0\n' > "$scratch/dpkg"
printf '#!/usr/bin/env bash\necho 0\n' > "$scratch/id"
chmod +x "$scratch/"{apt-get,clang-format,dpkg,dpkg-query,id}

output="$(CLAUDE_CODE_REMOTE=false bash "$setup")"
[[ -z "$output" && ! -e "$PACKAGE_LOG" ]]

output="$(CLAUDE_CODE_REMOTE=false bash "$setup" --cloud nlohmann-json3-dev)"
[[ "$output" == 'Cloud setup: packages present;'* ]]
grep -Fxq -- nlohmann-json3-dev "$PACKAGE_LOG"
if grep -Fxq -- --cloud "$PACKAGE_LOG"; then
    echo 'Cloud setup passed its --cloud option to the package manager.' >&2
    exit 1
fi

export PACKAGE_STATUS=''
output="$(CLAUDE_CODE_REMOTE=true bash "$setup")"
[[ "$output" == 'Cloud setup failed: apt-get did not install '* ]]

if output="$(CLAUDE_CODE_REMOTE=false bash "$setup" --cloud)"; then
    echo 'Explicit cloud setup reported success after an apt failure.' >&2
    exit 1
fi
[[ "$output" == 'Cloud setup failed: apt-get did not install '* ]]
