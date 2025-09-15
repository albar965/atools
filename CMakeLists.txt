#*****************************************************************************
# Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#****************************************************************************

# =============================================================================
# Set these environment variables for configuration - do not change this .pro file
# =============================================================================
#
# ATOOLS_GIT_PATH
# Optional. Path to GIT executable. Revision will be set to "UNKNOWN" if not set.
# Uses "git" on macOS and Linux as default if not set.
# Example: "C:\Git\bin\git"
#
# ATOOLS_SIMCONNECT_PATH
# Path to SimConnect SDK. SimConnect support will be omitted in build if not set.
# Example: "C:\Program Files (x86)\Microsoft Games\Microsoft Flight Simulator X SDK\SDK\Core Utilities Kit\SimConnect SDK"
#
# DEPLOY_BASE
# Optional. Target folder for "make deploy". Default is "../deploy" plus project name ($$TARGET_NAME).
#
# ATOOLS_QUIET
# Optional. Set this to "true" to avoid qmake messages.
#
# ATOOLS_NO_FS
# Optional. Set this to "true" to omit all flight simulator code except "weather" and "sc" if not needed.
# Reduces compilation time.
#
# ATOOLS_NO_GRIB
# Optional. Set this to "true" to omit all GRIB2 decoding code if not needed.
# Reduces compilation time.
#
# More components can be disabled. Note that disabling the wrong combinations can result in build errors.
# Full list is:
# ATOOLS_NO_FS,  ATOOLS_NO_GRIB,  ATOOLS_NO_GUI,  ATOOLS_NO_ROUTING,  ATOOLS_NO_SQL,  ATOOLS_NO_TRACK,
# ATOOLS_NO_USERDATA,  ATOOLS_NO_WEATHER,  ATOOLS_NO_WEB,  ATOOLS_NO_WMM,  ATOOLS_QUIET,
#
# This project has no deploy or install target. The include and library should
# be used directly from the source tree.
#
# =============================================================================
# End of configuration documentation
# =============================================================================

# Define program version here VERSION_NUMBER_TODO

cmake_minimum_required(VERSION 3.16)
project(atools VERSION 4.1.0 LANGUAGES CXX C)

# ---- Options for feature toggles ----
option(ATOOLS_NO_GUI "Disable GUI" OFF)
option(ATOOLS_NO_FS "Disable Flight Simulator code" OFF)
option(ATOOLS_NO_GRIB "Disable GRIB2 decoding" OFF)
option(ATOOLS_NO_SQL "Disable SQL" OFF)
option(ATOOLS_NO_USERDATA "Disable Userdata" OFF)
option(ATOOLS_NO_WEATHER "Disable Weather" OFF)
option(ATOOLS_NO_TRACK "Disable Track" OFF)
option(ATOOLS_NO_ROUTING "Disable Routing" OFF)
option(ATOOLS_NO_WEB "Disable Web" OFF)
option(ATOOLS_NO_WMM "Disable WMM" OFF)
option(ATOOLS_NO_NAVSERVER "Disable Navserver" OFF)
option(ATOOLS_NO_CRASHHANDLER "Disable Crashhandler" OFF)

add_definitions(
    -DVERSION_NUMBER_ATOOLS=\"${VERSION_NUMBER}\"
    -DGIT_REVISION_ATOOLS=\"${GIT_REVISION}\"
    -DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_NO_CAST_TO_ASCII
)
# ---- Qt ----
set(QT_MIN_VERSION "6.5.3")
find_package(Qt6 ${QT_MIN_VERSION} COMPONENTS Core Network Sql Xml Core5Compat REQUIRED)
if(NOT ATOOLS_NO_GUI)
    find_package(Qt6 COMPONENTS Widgets Svg REQUIRED)
endif()

# ---- Sources and Headers ----
file(GLOB_RECURSE HEADERS src/*.h src/*.hpp)
file(GLOB_RECURSE SOURCES src/*.cpp src/*.c)

# ---- Remove sources/headers based on options ----
if(ATOOLS_NO_GUI)
    list(FILTER SOURCES EXCLUDE REGEX "src/gui/.*|src/util/htmlbuilder.*|src/util/paintercontextsaver.*|src/util/polygontools.*|src/util/roundedpolygon.*")
    list(FILTER HEADERS EXCLUDE REGEX "src/gui/.*|src/util/htmlbuilder.*|src/util/paintercontextsaver.*|src/util/polygontools.*|src/util/roundedpolygon.*")
endif()
# Repeat for other options as needed...

# ---- Resources ----
qt_add_resources(ATOOLS_RESOURCES atools.qrc)
# Add navdata.qrc if needed

# ---- Library ----
add_library(atools STATIC ${SOURCES} ${HEADERS} ${ATOOLS_RESOURCES})

target_include_directories(atools PUBLIC src)

# ---- Compiler flags ----
target_compile_features(atools PUBLIC cxx_std_14)
target_compile_options(atools PRIVATE -Wall -Wextra -Wpedantic -Wno-pragmas -Wno-unknown-warning -Wno-unknown-warning-option)

# ---- Definitions ----
target_compile_definitions(atools PRIVATE
    VERSION_NUMBER_ATOOLS="\\\"${PROJECT_VERSION}\\\""
    QT_NO_CAST_FROM_BYTEARRAY
    QT_NO_CAST_TO_ASCII
)

# ---- Link Qt ----
target_link_libraries(atools
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    Qt6::Xml
    Qt::Core5Compat
)
if(NOT ATOOLS_NO_GUI)
    target_link_libraries(atools Qt6::Widgets Qt6::Svg)
endif()

# ---- Install/Deploy ----
install(TARGETS atools ARCHIVE DESTINATION lib)
install(DIRECTORY src/ DESTINATION include FILES_MATCHING PATTERN "*.h")
install(FILES CHANGELOG.txt README.txt LICENSE.txt DESTINATION .)

# ---- Custom deploy target (Linux example) ----
add_custom_target(deploy
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/deploy/atools
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/deploy/atools/lib
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/deploy/atools/include
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:atools> ${CMAKE_BINARY_DIR}/deploy/atools/lib/
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/deploy/atools/include
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/CHANGELOG.txt ${CMAKE_BINARY_DIR}/deploy/atools/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/README.txt ${CMAKE_BINARY_DIR}/deploy/atools/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/LICENSE.txt ${CMAKE_BINARY_DIR}/deploy/atools/
    BYPRODUCTS ${CMAKE_BINARY_DIR}/deploy/atools
    DEPENDS atools
)

# ---- Translations ----
file(GLOB TS_FILES *.ts)
# You can use qt_add_lupdate/qt_add_lrelease for translation support

# ---- Notes ----
# - You may need to refine the file(GLOB ...) patterns for fine-grained control.
# - For platform-specific code, use if(WIN32) or if(UNIX).
# - For SimConnect and other SDKs, add find_package or target_link_libraries as needed.
# - For forms (.ui), use qt_wrap_ui or qt_add_ui_files if needed.

# ---- End ----
