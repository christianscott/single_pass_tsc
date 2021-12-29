#!/usr/bin/env bash

set -euo pipefail

BOLD=$(tput bold)
NORMAL=$(tput sgr0)

log_info() {
  echo -e "${BOLD}test.sh INFO: $@${NORMAL}"
}

usage() {
    cat <<USAGE
Compares the output of a program with the expected output saved in files (AKA "snapshot" tests). Input and output files
are located under the fixtures/ directory. The .input file is passed as input to the program, then the data that program
writes to stderr and stdout are compared with the corresponding .stderr and .stdout files.

usage: $0 --bin \$path_to_binary [--update]

flags:
  --bin:    path to the binary under test
  --update: update the contents of .stdout and .stderr files to match the current output of the binary
USAGE
}

main() {
  if [[ $# -lt 1 ]]
  then
    usage
    exit 1
  fi

  local update='false'
  local bin
  while [[ $# -gt 0 ]]
  do
    local key="$1"
    case "$key" in
    help | --usage | --help)
      usage
      exit
      ;;
    --bin)
      bin="$2"
      shift 2
      ;;
    --update)
      update='true'
      shift
      ;;
    *)
      echo "unrecognised argument '$key'. run \`$0 help\` to display usage information"
      exit 1
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

    log_info "testing $name"
    "$bin" "$input" > >(tee "$got_stdout") 2> >(tee "$got_stderr" >&2) || true # swallow errors
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
    log_info "some failing tests. pass \`--update\` to update snapshot files"
    exit 1
  else
    log_info "all tests passed!"
  fi
}

main "$@"