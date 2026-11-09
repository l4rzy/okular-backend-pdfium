# Okular PDFium backend

A Qt 6 / Okular 6 generator plugin for PDFium. Supports PDF rendering, text
search and selection, links, bookmarks, and document metadata.

## Build

Requires CMake, a C++17 compiler, ECM, Qt 6, KDE Frameworks 6 CoreAddons, and
Okular 6 development files.

```sh
./scripts/download-pdfium.sh
make
```

Install with `sudo make install`. The `PREFIX`, `BUILD_DIR`, `BUILD_TYPE`, and
`JOBS` Make variables can customize the build. CPack packages can be created
with `make deb` or `make rpm` (`dpkg-deb` and `rpmbuild` are required).

## Run locally

```sh
./scripts/run-okular.sh document.pdf
```

In Okular, enable **Settings → Configure Okular → General → Show backend
selection dialog**, then select **PDFium**. The plugin and bundled PDFium
library are installed under the configured prefix.
