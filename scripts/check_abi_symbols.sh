#!/usr/bin/env bash
#
# check_abi_symbols.sh -- guard the libnatkit-core C ABI against accidental
# breaking changes, per the append-only versioning policy in
# docs/ABI_CONVENTIONS.md.
#
# It extracts the unmangled exported `nat_*` symbols from a built shared library
# and fails if any symbol in abi/symbols.txt is MISSING (removed/renamed =
# breaking). New symbols are allowed; the run reports them so they can be added
# to the baseline in the same commit.
#
# Usage: check_abi_symbols.sh [path/to/lib*.so]
# Falls back to build/liblibnatkit-core.so relative to the repo root.

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"
baseline="${repo_root}/abi/symbols.txt"

lib="${1:-}"
if [[ -z "${lib}" ]]; then
  lib="${repo_root}/build/liblibnatkit-core.so"
fi

if [[ ! -f "${lib}" ]]; then
  echo "ERROR: shared library not found: ${lib}" >&2
  echo "Build it first (cmake --build build) or pass the path explicitly." >&2
  exit 2
fi
if [[ ! -f "${baseline}" ]]; then
  echo "ERROR: baseline not found: ${baseline}" >&2
  exit 2
fi

# Defined text symbols (T) whose names are unmangled nat_* C ABI entry points.
current="$(nm -D --defined-only "${lib}" \
  | awk '$2=="T"{print $3}' \
  | grep -E '^nat_[a-z]' \
  | sort -u)"

expected="$(grep -vE '^\s*(#|$)' "${baseline}" | sort -u)"

missing="$(comm -23 <(printf '%s\n' "${expected}") <(printf '%s\n' "${current}"))"
added="$(comm -13 <(printf '%s\n' "${expected}") <(printf '%s\n' "${current}"))"

status=0
if [[ -n "${missing}" ]]; then
  echo "ABI CHECK FAILED: baseline symbols missing from the library" >&2
  echo "${missing}" | sed 's/^/  - /' >&2
  echo "A removed or renamed exported symbol is a breaking ABI change." >&2
  echo "Ship the change as a new _vN_ symbol instead of editing the old one." >&2
  status=1
fi

if [[ -n "${added}" ]]; then
  echo "Note: new exported symbols not yet in the baseline (append them to abi/symbols.txt):"
  echo "${added}" | sed 's/^/  + /'
fi

if [[ "${status}" -eq 0 ]]; then
  echo "ABI check passed: all $(printf '%s\n' "${expected}" | grep -c .) baseline symbols present."
fi
exit "${status}"
