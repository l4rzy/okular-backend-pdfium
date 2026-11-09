.PHONY: all pdfium configure build install package deb rpm clean

BUILD_DIR ?= build
BUILD_TYPE ?= Release
PREFIX ?= /usr
JOBS ?= $(shell nproc)

all: build

pdfium:
	@./scripts/download-pdfium.sh

configure:
	cmake -S . -B "$(BUILD_DIR)" \
		-DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" \
		-DCMAKE_INSTALL_PREFIX="$(PREFIX)"

build: configure
	cmake --build "$(BUILD_DIR)" --parallel "$(JOBS)"

install: build
	cmake --install "$(BUILD_DIR)"

package: build
	cd "$(BUILD_DIR)" && cpack

deb: build
	cd "$(BUILD_DIR)" && cpack -G DEB

rpm: build
	cd "$(BUILD_DIR)" && cpack -G RPM

clean:
	cmake -E rm -rf "$(BUILD_DIR)"
