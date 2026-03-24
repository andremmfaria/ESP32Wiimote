#!/usr/bin/env sh
set -eu

# Usage:
#   scripts/list-large-cpp-files.sh            # defaults to 500 lines
#   scripts/list-large-cpp-files.sh 800        # custom threshold

threshold="${1:-500}"

if ! [ "$threshold" -eq "$threshold" ] 2>/dev/null; then
    printf 'error: threshold must be an integer\n' >&2
    exit 1
fi

git ls-files -co --exclude-standard | while IFS= read -r file; do
    case "$file" in
        *.c|*.cc|*.cpp|*.cxx|*.h|*.hh|*.hpp|*.hxx|*.ino|*.ipp)
            ;;
        *)
            continue
            ;;
    esac

    lines=$(wc -l < "$file" | tr -d '[:space:]')
    if [ "$lines" -gt "$threshold" ]; then
        printf '%s\t%s\n' "$lines" "$file"
    fi
done | sort -nr
