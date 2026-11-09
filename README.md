# PDFium backend for Okular

Minimal Qt 6 / Okular 6 plugin: opens PDFs, renders pages, and supports text
selection, copying, search, clickable PDF links, and a bookmark table of contents
using the PDFium binary in `pdfium/`.
Rendering and text extraction run on Okular's background threads.
At high zoom, Okular can request tiles; only the requested region is allocated
and rendered, using the same page scale as a full-page render.
Obsolete renders stop through PDFium's progressive-render pause callback.
Cancelled renders and text extraction discard partial results.
Text selection requires embedded text; scanned images have no OCR support.
Links support destinations within the PDF and external URLs. The contents panel
shows the PDF's bookmark hierarchy; documents without bookmarks have no TOC.
Document properties include the PDF's title, author, subject, keywords, creator,
producer, creation/modification dates, page count, version, and encryption status.
Dates are displayed in the local time zone and locale; malformed dates are shown
as stored in the PDF.
No forms, external-document destinations, or printing support.
Password-protected PDFs are not supported by this MVP.

Requires CMake, a C++17 compiler, ECM, Qt 6, KDE Frameworks 6 CoreAddons, and
Okular 6 development files.

Fetch the latest Linux x64 PDFium binaries and license notices before building:

```sh
./scripts/download-pdfium.sh
```

```sh
make                 # configure and build
sudo make install    # install under /usr
make deb             # create a Debian package in build/
make rpm             # create an RPM package in build/
```

Set `BUILD_DIR`, `BUILD_TYPE`, `PREFIX`, or `JOBS` to customize the Makefile
targets. CPack package generation requires `dpkg-deb` for DEB and `rpmbuild`
for RPM. Packages depend on Okular and include PDFium plus its bundled license
notices.

Restart Okular after installation. Enable **Settings → Configure Okular →
General → Show backend selection dialog**, open a PDF, and choose **PDFium**.
The plugin has a low priority so installing it does not replace the default.
The install includes `libpdfium.so` under `lib/okular-backend-pdfium/` and
PDFium's licenses.

To run without installing:

```sh
./scripts/run-okular.sh document.pdf
```

The launcher uses `build/plugins` and `pdfium/lib` relative to the project root
and forwards all arguments to Okular. Enable the backend selection dialog for
the local run too.
