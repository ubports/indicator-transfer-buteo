project(buteo-transfers-plugins)
cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Standard install paths
include(GNUInstallDirs)

find_package(PkgConfig REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5Xml REQUIRED)

pkg_check_modules(GMODULE REQUIRED gmodule-2.0>=2.36)
pkg_check_modules(TRANSFER_INDICATOR REQUIRED indicator-transfer)
pkg_check_modules(URL_DISPATCHER REQUIRED url-dispatcher-1)
pkg_check_modules(ACCOUNTS_QT5 REQUIRED accounts-qt5)

#i18n
set(GETTEXT_PACKAGE "indicator-transfer-buteo")
add_definitions(-DGETTEXT_PACKAGE="${GETTEXT_PACKAGE}"
                -DGNOMELOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}")

# config file
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/config.h"
               IMMEDIATE @ONLY)

enable_testing()
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(po)

# Coverage reporting
include(EnableCoverageReport)
enable_coverage_report(
  TARGETS
    tst-transfer-plugin
  TESTS
    tst-transfer-plugin
)

# uninstall target
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
               "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
               IMMEDIATE @ONLY)
add_custom_target(uninstall "${CMAKE_COMMAND}"
                  -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
