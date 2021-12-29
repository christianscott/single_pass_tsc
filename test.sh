#!/usr/bin/env bash

set -euo pipefail

main() {
  local update='false'
  local bin
  while [[ $# -gt 0 ]]
  do
    local key="$1"
    case "$key" in
    --bin)
      bin="$2"
      shift 2
      ;;
    --update)
      update='true'
      shift
      ;;
    esac
  done

  if [[ ! -f "$bin" ]]
  then
    echo "no such file '$bin'"
    exit 1
  fi

  if [[ ! -x "$bin" ]]
  then
    echo "file '$bin' is not marked as executable"
    exit 1
  fi

  local tmpdir
  tmpdir=$(mktemp -d /tmp/single_pass_tsc_tests.XXXXXX)
  local has_errs='false'
  for input in fixtures/*.input;
  do
    local name=$(basename "${input%.input}")

    local want_stdout="./fixtures/$name.stdout"
    local got_stdout="$tmpdir/$name.stdout"
    local want_stderr="./fixtures/$name.stderr"
    local got_stderr="$tmpdir/$name.stderr"

    "$bin" "$input" > >(tee "$got_stdout") 2> >(tee "$got_stderr" >&2) || has_errs='true'
    git diff --no-index "$want_stdout" "$got_stdout" || has_errs='true'
    git diff --no-index "$want_stderr" "$got_stderr" || has_errs='true'

    if [[ "$update" == 'true' ]]
    then
      echo "updating fixtures for $name"
      cp "$got_stdout" "$want_stdout"
      cp "$got_stderr" "$want_stderr"
    fi
  done

  rm -rf "$tmpdir"

  if [[ "$has_errs" == 'true' ]]
  then
    bold=$(tput bold)
    normal=$(tput sgr0)
    echo -e "${bold}test.sh: some failing tests. pass \`--update\` to update snapshot files${normal}"
    exit 1
  fi
}

main "$@"