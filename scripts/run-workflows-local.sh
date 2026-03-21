#!/usr/bin/env bash

set -euo pipefail

usage() {
  cat <<'EOF'
Usage: scripts/run-workflows-local.sh [ci|release] [options] [-- <extra act args>]

Run this repository's GitHub Actions workflows locally with act.

Modes:
  ci                  Run .github/workflows/platformio-ci.yml (default)
  release             Run .github/workflows/release.yml

Options:
  -j, --job JOB       Run a single job from the selected workflow
      --event EVENT   CI event to simulate: push or pull_request (default: push)
      --branch NAME   Branch to simulate for CI runs
      --tag TAG       Release tag to simulate, defaults to library.properties version
      --list          List jobs in the selected workflow
      --dry-run       Show the execution plan without running containers
  -v, --verbose       Enable verbose act output
      --json-logs     Emit act logs in JSON format
      --reuse         Reuse existing act containers
      --secret-file   Load secrets from a specific act secrets file
      --allow-publish Allow release jobs that create GitHub releases/uploads
      --arch ARCH     Container architecture for act (default: linux/amd64)
  -h, --help          Show this help

Environment:
  ACT_IMAGE           Override the image used for ubuntu-latest
  ACT_CONTAINER_ARCH  Override the act container architecture
  ACT_RUNNER_TEMP     Override the writable temp directory inside the container
  ACT_TOOL_CACHE      Override the writable tool cache directory inside the container
  GITHUB_TOKEN        Passed to act when set

Examples:
  scripts/run-workflows-local.sh ci
  scripts/run-workflows-local.sh ci --event pull_request --branch develop
  scripts/run-workflows-local.sh ci --job test --dry-run
  scripts/run-workflows-local.sh release
  scripts/run-workflows-local.sh release --job validate-tag-version --tag 1.5.0
  scripts/run-workflows-local.sh release --allow-publish --tag 1.5.0
EOF
}

die() {
  printf 'Error: %s\n' "$1" >&2
  exit 1
}

require_command() {
  command -v "$1" >/dev/null 2>&1 || die "Missing required command: $1"
}

default_branch() {
  local branch

  branch="$(git -C "$repo_root" rev-parse --abbrev-ref HEAD 2>/dev/null || true)"
  if [[ -n "$branch" && "$branch" != "HEAD" ]]; then
    printf '%s\n' "$branch"
  else
    printf 'develop\n'
  fi
}

read_library_version() {
  sed -n 's/^version=//p' "$repo_root/library.properties" | head -n 1 | tr -d '[:space:]'
}

validate_semver() {
  local tag="$1"

  [[ "$tag" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || die "Invalid tag '$tag'. Expected plain semver like 1.5.0"
}

build_local_release_workflow() {
  local source_file="$1"
  local output_file="$2"

  awk '
    function indent_len(line) {
      match(line, /^[ ]*/)
      return RLENGTH
    }

    BEGIN {
      skip = 0
      step_indent = -1
    }

    {
      if (skip) {
        current_indent = indent_len($0)

        if ($0 ~ /^[[:space:]]*-[[:space:]]+name:/ && current_indent == step_indent) {
          skip = 0
        } else if ($0 ~ /^[^[:space:]]/ || (current_indent < step_indent && $0 !~ /^[[:space:]]*$/)) {
          skip = 0
        } else {
          next
        }
      }

      if ($0 ~ /^[[:space:]]*-[[:space:]]+name:[[:space:]]+(Create GitHub Release|Upload firmware to release)[[:space:]]*$/) {
        skip = 1
        step_indent = indent_len($0)
        next
      }

      print
    }
  ' "$source_file" > "$output_file"
}

build_event_payload() {
  local event_name="$1"
  local event_ref="$2"
  local event_ref_name="$3"
  local branch_name="$4"
  local event_file="$5"

  if [[ "$event_name" == "pull_request" ]]; then
    cat >"$event_file" <<EOF
{
  "ref": "$event_ref",
  "ref_name": "$event_ref_name",
  "pull_request": {
    "number": 1,
    "head": {
      "ref": "$branch_name"
    },
    "base": {
      "ref": "develop"
    }
  },
  "repository": {
    "full_name": "andremmfaria/ESP32Wiimote",
    "default_branch": "develop"
  },
  "sender": {
    "login": "local"
  },
  "head_commit": {
    "id": "local"
  }
}
EOF
    return
  fi

  cat >"$event_file" <<EOF
{
  "ref": "$event_ref",
  "ref_name": "$event_ref_name",
  "repository": {
    "full_name": "andremmfaria/ESP32Wiimote",
    "default_branch": "develop"
  },
  "sender": {
    "login": "local"
  },
  "head_commit": {
    "id": "local"
  },
  "before": "0000000000000000000000000000000000000000",
  "after": "1111111111111111111111111111111111111111"
}
EOF
}

workflow="ci"
job=""
ci_event="push"
branch=""
tag=""
list_only=0
dry_run=0
verbose=0
json_logs=0
reuse=0
allow_publish=0
secret_file=""
container_arch="${ACT_CONTAINER_ARCH:-linux/amd64}"
act_image="${ACT_IMAGE:-catthehacker/ubuntu:full-latest}"
extra_args=()
temp_workflow_file=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    ci|release)
      workflow="$1"
      shift
      ;;
    -j|--job)
      [[ $# -ge 2 ]] || die "Missing value for $1"
      job="$2"
      shift 2
      ;;
    --event)
      [[ $# -ge 2 ]] || die "Missing value for $1"
      ci_event="$2"
      shift 2
      ;;
    --branch)
      [[ $# -ge 2 ]] || die "Missing value for $1"
      branch="$2"
      shift 2
      ;;
    --tag)
      [[ $# -ge 2 ]] || die "Missing value for $1"
      tag="$2"
      shift 2
      ;;
    --list)
      list_only=1
      shift
      ;;
    --dry-run)
      dry_run=1
      shift
      ;;
    -v|--verbose)
      verbose=1
      shift
      ;;
    --json-logs)
      json_logs=1
      shift
      ;;
    --reuse)
      reuse=1
      shift
      ;;
    --secret-file)
      [[ $# -ge 2 ]] || die "Missing value for $1"
      secret_file="$2"
      shift 2
      ;;
    --allow-publish)
      allow_publish=1
      shift
      ;;
    --arch)
      [[ $# -ge 2 ]] || die "Missing value for $1"
      container_arch="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    --)
      shift
      extra_args+=( "$@" )
      break
      ;;
    *)
      die "Unknown argument: $1"
      ;;
  esac
done

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "$script_dir/.." && pwd)"

require_command act
require_command docker
require_command git

docker info >/dev/null 2>&1 || die "Docker daemon is not available"

branch="${branch:-$(default_branch)}"

case "$workflow" in
  ci)
    [[ "$ci_event" == "push" || "$ci_event" == "pull_request" ]] || die "CI event must be push or pull_request"
    workflow_file="$repo_root/.github/workflows/platformio-ci.yml"
    event_name="$ci_event"
    event_ref="refs/heads/$branch"
    event_ref_name="$branch"
    ;;
  release)
    workflow_file="$repo_root/.github/workflows/release.yml"
    event_name="push"
    library_version="$(read_library_version)"
    [[ -n "$library_version" ]] || die "Unable to read version from library.properties"

    if [[ -z "$tag" ]]; then
      tag="$library_version"
    fi

    validate_semver "$tag"
    event_ref="refs/tags/$tag"
    event_ref_name="$tag"

    if [[ $allow_publish -eq 0 ]]; then
      temp_workflow_file="$(mktemp --suffix=.yml)"
      build_local_release_workflow "$workflow_file" "$temp_workflow_file"
      workflow_file="$temp_workflow_file"
      printf 'Info: using local release workflow that skips publish/upload steps and runs the remaining jobs.\n' >&2
      printf 'Info: pass --allow-publish to run the original workflow with GitHub release actions.\n' >&2
    fi

    if [[ $allow_publish -eq 1 && -z "${GITHUB_TOKEN:-}" && -z "$secret_file" && ! -f "$repo_root/.secrets.act" ]]; then
      die "Publishing release jobs require GITHUB_TOKEN or an act secrets file"
    fi
    ;;
esac

[[ -f "$workflow_file" ]] || die "Workflow file not found: $workflow_file"

if [[ $list_only -eq 1 ]]; then
  cd "$repo_root"
  exec act -W "$workflow_file" -l
fi

event_file="$(mktemp)"
artifact_dir="$repo_root/.act/artifacts"
runner_temp_dir="${ACT_RUNNER_TEMP:-/tmp/act-runner-temp}"
tool_cache_dir="${ACT_TOOL_CACHE:-/tmp/act-toolcache}"
mkdir -p "$artifact_dir"

cleanup() {
  rm -f "$event_file"
  if [[ -n "$temp_workflow_file" ]]; then
    rm -f "$temp_workflow_file"
  fi
}

trap cleanup EXIT

build_event_payload "$event_name" "$event_ref" "$event_ref_name" "$branch" "$event_file"

act_command=(
  act
  "$event_name"
  -W "$workflow_file"
  -e "$event_file"
  -P "ubuntu-latest=$act_image"
  --container-architecture "$container_arch"
  --artifact-server-path "$artifact_dir"
  --env "RUNNER_TEMP=$runner_temp_dir"
  --env "RUNNER_TOOL_CACHE=$tool_cache_dir"
  --env "AGENT_TOOLSDIRECTORY=$tool_cache_dir"
)

if [[ -n "$job" ]]; then
  act_command+=( -j "$job" )
fi

if [[ $dry_run -eq 1 ]]; then
  act_command+=( -n )
fi

if [[ $verbose -eq 1 ]]; then
  act_command+=( -v )
fi

if [[ $json_logs -eq 1 ]]; then
  act_command+=( --json )
fi

if [[ $reuse -eq 1 ]]; then
  act_command+=( --reuse )
fi

if [[ -n "$secret_file" ]]; then
  act_command+=( --secret-file "$secret_file" )
elif [[ -f "$repo_root/.secrets.act" ]]; then
  act_command+=( --secret-file "$repo_root/.secrets.act" )
fi

if [[ -n "${GITHUB_TOKEN:-}" ]]; then
  act_command+=( -s "GITHUB_TOKEN=$GITHUB_TOKEN" )
fi

if [[ ${#extra_args[@]} -gt 0 ]]; then
  act_command+=( "${extra_args[@]}" )
fi

printf 'Running %s workflow with act\n' "$workflow"
printf 'Workflow file: %s\n' "$workflow_file"
printf 'Event: %s\n' "$event_name"
printf 'Event ref: %s\n' "$event_ref"
if [[ -n "$job" ]]; then
  printf 'Job: %s\n' "$job"
fi
if [[ $verbose -eq 1 ]]; then
  printf 'Act command:'
  printf ' %q' "${act_command[@]}"
  printf '\n'
fi

cd "$repo_root"
exec "${act_command[@]}"
