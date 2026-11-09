# CMake package config for the PDFium binary distribution.

include(FindPackageHandleStandardArgs)

find_path(PDFium_INCLUDE_DIR
    NAMES fpdfview.h
    PATHS "${CMAKE_CURRENT_LIST_DIR}"
    PATH_SUFFIXES include)

if(WIN32)
    find_file(PDFium_LIBRARY
        NAMES pdfium.dll
        PATHS "${CMAKE_CURRENT_LIST_DIR}"
        PATH_SUFFIXES bin)
    find_file(PDFium_IMPLIB
        NAMES pdfium.dll.lib
        PATHS "${CMAKE_CURRENT_LIST_DIR}"
        PATH_SUFFIXES lib)

    add_library(pdfium SHARED IMPORTED)
    set_target_properties(pdfium PROPERTIES
        IMPORTED_LOCATION "${PDFium_LIBRARY}"
        IMPORTED_IMPLIB "${PDFium_IMPLIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${PDFium_INCLUDE_DIR};${PDFium_INCLUDE_DIR}/cpp")

    find_package_handle_standard_args(PDFium
        REQUIRED_VARS PDFium_LIBRARY PDFium_IMPLIB PDFium_INCLUDE_DIR)
else()
    find_library(PDFium_LIBRARY
        NAMES pdfium
        PATHS "${CMAKE_CURRENT_LIST_DIR}"
        PATH_SUFFIXES lib)

    add_library(pdfium SHARED IMPORTED)
    set_target_properties(pdfium PROPERTIES
        IMPORTED_LOCATION "${PDFium_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${PDFium_INCLUDE_DIR};${PDFium_INCLUDE_DIR}/cpp")

    find_package_handle_standard_args(PDFium
        REQUIRED_VARS PDFium_LIBRARY PDFium_INCLUDE_DIR)
endif()
