#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
plugin_dir="$project_dir/build/plugins"
pdfium_lib_dir="$project_dir/pdfium/lib"

if [ ! -f "$plugin_dir/okular_generators/okularGenerator_pdfium.so" ]; then
    echo "PDFium plugin not found. Run cmake -S . -B build && cmake --build build first." >&2
    exit 1
fi

if [ ! -f "$pdfium_lib_dir/libpdfium.so" ]; then
    echo "PDFium library not found: $pdfium_lib_dir/libpdfium.so" >&2
    exit 1
fi

export QT_PLUGIN_PATH="$plugin_dir${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}"
export LD_LIBRARY_PATH="$pdfium_lib_dir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
exec okular "$@"
