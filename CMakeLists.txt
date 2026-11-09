cmake_minimum_required(VERSION 3.20)
project(okular-backend-pdfium VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(BUILD_SHARED_LIBS ON)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins")

find_package(ECM 6.0 REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
include(KDEInstallDirs6)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(KF6CoreAddons REQUIRED)
find_package(Okular6 REQUIRED)
find_package(PDFium CONFIG REQUIRED
    PATHS "${CMAKE_CURRENT_SOURCE_DIR}/pdfium")

kcoreaddons_add_plugin(okularGenerator_pdfium
    INSTALL_NAMESPACE "okular_generators"
    SOURCES src/pdfiumgenerator.cpp src/pdfiumgenerator.h
        src/navigation.cpp src/navigation.h src/metadata.cpp src/metadata.h
        src/text.cpp src/text.h src/pdfiumhelpers.h)
target_link_libraries(okularGenerator_pdfium PRIVATE
    Okular::Core KF6::CoreAddons Qt6::Gui pdfium)

# Keep the bundled engine in a prefix-relative private library directory.
# CMake supplies the PDFium path automatically for build-tree use; the
# installed plugin uses a relative RPATH to find the installed engine.
set(PDFIUM_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/okular-backend-pdfium")
file(RELATIVE_PATH PDFIUM_RELATIVE_RPATH
    "/${KDE_INSTALL_PLUGINDIR}/okular_generators"
    "/${PDFIUM_INSTALL_DIR}")
set_target_properties(okularGenerator_pdfium PROPERTIES
    PREFIX ""
    INSTALL_RPATH "$ORIGIN/${PDFIUM_RELATIVE_RPATH}")
install(FILES "${PDFium_LIBRARY}" DESTINATION "${PDFIUM_INSTALL_DIR}")
install(DIRECTORY "${PDFium_DIR}/licenses/"
    DESTINATION "${KDE_INSTALL_DATADIR}/doc/okular-backend-pdfium/licenses")
install(FILES "${PDFium_DIR}/LICENSE"
    DESTINATION "${KDE_INSTALL_DATADIR}/doc/okular-backend-pdfium")

set(CPACK_PACKAGE_NAME "okular-backend-pdfium")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR "Okular PDFium backend contributors")
set(CPACK_PACKAGE_CONTACT "PDFium backend contributors")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "PDFium backend plugin for Okular")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/l4rzy/okular-backend-pdfium")
set(CPACK_RESOURCE_FILE_LICENSE "${PDFium_DIR}/LICENSE")

set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "okular")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^i[3-6]86$")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^armv7")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")
endif()
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

set(CPACK_RPM_PACKAGE_LICENSE "GPL-2.0-or-later AND MIT")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Publishing")
set(CPACK_RPM_PACKAGE_REQUIRES "okular")
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)

include(CPack)
