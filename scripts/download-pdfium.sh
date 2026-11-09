#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
work_dir=$(mktemp -d "$repo_dir/.pdfium-download.XXXXXX")
pdfium_dir="$repo_dir/pdfium"
archive_url="https://github.com/bblanchon/pdfium-binaries/releases/latest/download/pdfium-linux-x64.tgz"
trap 'rm -rf -- "$work_dir"' EXIT

mkdir -p "$work_dir/pdfium"
curl --fail --location --retry 3 --output "$work_dir/pdfium-linux-x64.tgz" "$archive_url"
tar --extract --gzip --file "$work_dir/pdfium-linux-x64.tgz" \
    --directory "$work_dir/pdfium" --no-same-owner

cp "$repo_dir/cmake/PDFiumConfig.cmake" "$work_dir/pdfium/PDFiumConfig.cmake"
for required_file in include/fpdfview.h lib/libpdfium.so LICENSE; do
    if [[ ! -f "$work_dir/pdfium/$required_file" ]]; then
        echo "PDFium archive is missing $required_file" >&2
        exit 1
    fi
done
if [[ ! -d "$work_dir/pdfium/licenses" ]]; then
    echo "PDFium archive is missing its licenses directory" >&2
    exit 1
fi

# Replace the ignored dependency directory only after the new download passes
# validation, restoring the old directory if the final move fails.
if [[ -e "$pdfium_dir" ]]; then
    mv -- "$pdfium_dir" "$work_dir/previous"
fi
if ! mv -- "$work_dir/pdfium" "$pdfium_dir"; then
    if [[ -e "$work_dir/previous" ]]; then
        mv -- "$work_dir/previous" "$pdfium_dir"
    fi
    exit 1
fi

echo "Installed latest PDFium binaries in $pdfium_dir"
