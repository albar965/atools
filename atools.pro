#*****************************************************************************
# Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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
# This project has no deploy or install target. The include and library should
# be used directly from the source tree.
#
# =============================================================================
# End of configuration documentation
# =============================================================================

QT += sql xml svg core widgets network
QT -= gui
CONFIG += build_all c++14 staticlib
CONFIG -= debug_and_release debug_and_release_target

TARGET = atools
TARGET_NAME=$$TARGET

TEMPLATE = lib

# =======================================================================
# Copy ennvironment variables into qmake variables

GIT_PATH=$$(ATOOLS_GIT_PATH)
SIMCONNECT_PATH=$$(ATOOLS_SIMCONNECT_PATH)
DEPLOY_BASE=$$(DEPLOY_BASE)
QUIET=$$(ATOOLS_QUIET)
ATOOLS_NO_FS=$$(ATOOLS_NO_FS)
ATOOLS_NO_GRIB=$$(ATOOLS_NO_GRIB)

# =======================================================================
# Fill defaults for unset

CONFIG(debug, debug|release) : CONF_TYPE=debug
CONFIG(release, debug|release) : CONF_TYPE=release

isEmpty(DEPLOY_BASE) : DEPLOY_BASE=$$PWD/../deploy

# =======================================================================
# Set compiler flags and paths

INCLUDEPATH += $$PWD/src

unix {
  isEmpty(GIT_PATH) : GIT_PATH=git
}

win32 {
  DEFINES += _USE_MATH_DEFINES
  DEFINES += NOMINMAX

  !isEmpty(SIMCONNECT_PATH) {
    DEFINES += SIMCONNECT_BUILD
    INCLUDEPATH += $$SIMCONNECT_PATH"\inc"
    LIBS += $$SIMCONNECT_PATH"\lib\SimConnect.lib"
  }
}

macx {
  # Compatibility down to OS X Mountain Lion 10.8
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.8
}

isEmpty(GIT_PATH) {
  GIT_REVISION='\\"UNKNOWN\\"'
} else {
  GIT_REVISION='\\"$$system('$$GIT_PATH' rev-parse --short HEAD)\\"'
}

DEFINES += GIT_REVISION_ATOOLS=$$GIT_REVISION
DEFINES += QT_NO_CAST_FROM_BYTEARRAY
DEFINES += QT_NO_CAST_TO_ASCII

# =======================================================================
# Include build_options.pro with additional variables

exists($$PWD/../build_options.pro) {
   include($$PWD/../build_options.pro)

   !isEqual(QUIET, "true") {
     message($$PWD/../build_options.pro found.)
   }
} else {
   !isEqual(QUIET, "true") {
     message($$PWD/../build_options.pro not found.)
   }
}

# =======================================================================
# Print values when running qmake

!isEqual(QUIET, "true") {
message(-----------------------------------)
message(GIT_PATH: $$GIT_PATH)
message(GIT_REVISION: $$GIT_REVISION)
message(ATOOLS_NO_FS: $$ATOOLS_NO_FS)
message(ATOOLS_NO_GRIB: $$ATOOLS_NO_GRIB)
message(SIMCONNECT_PATH: $$SIMCONNECT_PATH)
message(DEFINES: $$DEFINES)
message(INCLUDEPATH: $$INCLUDEPATH)
message(LIBS: $$LIBS)
message(TARGET_NAME: $$TARGET_NAME)
message(QT_INSTALL_PREFIX: $$[QT_INSTALL_PREFIX])
message(QT_INSTALL_LIBS: $$[QT_INSTALL_LIBS])
message(QT_INSTALL_PLUGINS: $$[QT_INSTALL_PLUGINS])
message(QT_INSTALL_TRANSLATIONS: $$[QT_INSTALL_TRANSLATIONS])
message(CONFIG: $$CONFIG)
message(-----------------------------------)
}

# =====================================================================
# General Files

HEADERS += \
  src/atools.h \
  src/exception.h \
  src/fs/sc/connecthandler.h \
  src/fs/sc/datareaderthread.h \
  src/fs/sc/simconnectaircraft.h \
  src/fs/sc/simconnectapi.h \
  src/fs/sc/simconnectdata.h \
  src/fs/sc/simconnectdatabase.h \
  src/fs/sc/simconnectdummy.h \
  src/fs/sc/simconnecthandler.h \
  src/fs/sc/simconnectreply.h \
  src/fs/sc/simconnecttypes.h \
  src/fs/sc/simconnectuseraircraft.h \
  src/fs/sc/weatherrequest.h \
  src/fs/sc/xpconnecthandler.h \
  src/fs/userdata/airspacereaderbase.h \
  src/fs/userdata/airspacereaderopenair.h \
  src/fs/userdata/datamanagerbase.h \
  src/fs/userdata/logdatamanager.h \
  src/fs/weather/metar.h \
  src/fs/weather/metarindex.h \
  src/fs/weather/metarparser.h \
  src/fs/weather/noaaweatherdownloader.h \
  src/fs/weather/weathernetdownload.h \
  src/fs/weather/weathernetsingle.h \
  src/fs/weather/weathertypes.h \
  src/fs/weather/xpweatherreader.h \
  src/geo/calculations.h \
  src/geo/line.h \
  src/geo/linestring.h \
  src/geo/pos.h \
  src/geo/rect.h \
  src/geo/simplespatialindex.h \
  src/grib/windquery.h \
  src/gui/actionstatesaver.h \
  src/gui/actiontextsaver.h \
  src/gui/application.h \
  src/gui/consoleapplication.h \
  src/gui/dialog.h \
  src/gui/dockwidgethandler.h \
  src/gui/errorhandler.h \
  src/gui/filehistoryhandler.h \
  src/gui/griddelegate.h \
  src/gui/helphandler.h \
  src/gui/itemviewzoomhandler.h \
  src/gui/mapposhistory.h \
  src/gui/palettesettings.h \
  src/gui/signalblocker.h \
  src/gui/tabwidgethandler.h \
  src/gui/tools.h \
  src/gui/translator.h \
  src/gui/widgetstate.h \
  src/gui/widgetutil.h \
  src/httpserver/httpconnectionhandler.h \
  src/httpserver/httpconnectionhandlerpool.h \
  src/httpserver/httpcookie.h \
  src/httpserver/httpglobal.h \
  src/httpserver/httplistener.h \
  src/httpserver/httprequest.h \
  src/httpserver/httprequesthandler.h \
  src/httpserver/httpresponse.h \
  src/httpserver/httpsession.h \
  src/httpserver/httpsessionstore.h \
  src/httpserver/staticfilecontroller.h \
  src/io/abstractinireader.h \
  src/io/binarystream.h \
  src/io/fileroller.h \
  src/io/inireader.h \
  src/io/tempfile.h \
  src/logging/loggingconfig.h \
  src/logging/loggingguiabort.h \
  src/logging/logginghandler.h \
  src/logging/loggingtypes.h \
  src/logging/loggingutil.h \
  src/settings/settings.h \
  src/sql/sqldatabase.h \
  src/sql/sqlexception.h \
  src/sql/sqlexport.h \
  src/sql/sqlquery.h \
  src/sql/sqlrecord.h \
  src/sql/sqlscript.h \
  src/sql/sqltransaction.h \
  src/sql/sqlutil.h \
  src/templateengine/template.h \
  src/templateengine/templatecache.h \
  src/templateengine/templateglobal.h \
  src/templateengine/templateloader.h \
  src/util/csvreader.h \
  src/util/filesystemwatcher.h \
  src/util/heap.h \
  src/util/htmlbuilder.h \
  src/util/httpdownloader.h \
  src/util/paintercontextsaver.h \
  src/util/properties.h \
  src/util/roundedpolygon.h \
  src/util/timedcache.h \
  src/util/updatecheck.h \
  src/util/version.h \
  src/win/activationcontext.h \
  src/wmm/GeomagnetismHeader.h \
  src/wmm/magdectool.h \
  src/zip/gzip.h \
  src/zip/zipreader.h \
  src/zip/zipwriter.h

SOURCES += \
  src/atools.cpp \
  src/exception.cpp \
  src/fs/sc/connecthandler.cpp \
  src/fs/sc/datareaderthread.cpp \
  src/fs/sc/simconnectaircraft.cpp \
  src/fs/sc/simconnectapi.cpp \
  src/fs/sc/simconnectdata.cpp \
  src/fs/sc/simconnectdatabase.cpp \
  src/fs/sc/simconnectdummy.cpp \
  src/fs/sc/simconnecthandler.cpp \
  src/fs/sc/simconnectreply.cpp \
  src/fs/sc/simconnectuseraircraft.cpp \
  src/fs/sc/weatherrequest.cpp \
  src/fs/sc/xpconnecthandler.cpp \
  src/fs/userdata/airspacereaderbase.cpp \
  src/fs/userdata/airspacereaderopenair.cpp \
  src/fs/userdata/datamanagerbase.cpp \
  src/fs/userdata/logdatamanager.cpp \
  src/fs/weather/metar.cpp \
  src/fs/weather/metarindex.cpp \
  src/fs/weather/metarparser.cpp \
  src/fs/weather/noaaweatherdownloader.cpp \
  src/fs/weather/weathernetdownload.cpp \
  src/fs/weather/weathernetsingle.cpp \
  src/fs/weather/weathertypes.cpp \
  src/fs/weather/xpweatherreader.cpp \
  src/geo/calculations.cpp \
  src/geo/line.cpp \
  src/geo/linestring.cpp \
  src/geo/pos.cpp \
  src/geo/rect.cpp \
  src/geo/simplespatialindex.cpp \
  src/grib/windquery.cpp \
  src/gui/actionstatesaver.cpp \
  src/gui/actiontextsaver.cpp \
  src/gui/application.cpp \
  src/gui/consoleapplication.cpp \
  src/gui/dialog.cpp \
  src/gui/dockwidgethandler.cpp \
  src/gui/errorhandler.cpp \
  src/gui/filehistoryhandler.cpp \
  src/gui/griddelegate.cpp \
  src/gui/helphandler.cpp \
  src/gui/itemviewzoomhandler.cpp \
  src/gui/mapposhistory.cpp \
  src/gui/palettesettings.cpp \
  src/gui/signalblocker.cpp \
  src/gui/tabwidgethandler.cpp \
  src/gui/tools.cpp \
  src/gui/translator.cpp \
  src/gui/widgetstate.cpp \
  src/gui/widgetutil.cpp \
  src/httpserver/httpconnectionhandler.cpp \
  src/httpserver/httpconnectionhandlerpool.cpp \
  src/httpserver/httpcookie.cpp \
  src/httpserver/httpglobal.cpp \
  src/httpserver/httplistener.cpp \
  src/httpserver/httprequest.cpp \
  src/httpserver/httprequesthandler.cpp \
  src/httpserver/httpresponse.cpp \
  src/httpserver/httpsession.cpp \
  src/httpserver/httpsessionstore.cpp \
  src/httpserver/staticfilecontroller.cpp \
  src/io/abstractinireader.cpp \
  src/io/binarystream.cpp \
  src/io/fileroller.cpp \
  src/io/inireader.cpp \
  src/io/tempfile.cpp \
  src/logging/loggingconfig.cpp \
  src/logging/loggingguiabort.cpp \
  src/logging/logginghandler.cpp \
  src/logging/loggingutil.cpp \
  src/settings/settings.cpp \
  src/sql/sqldatabase.cpp \
  src/sql/sqlexception.cpp \
  src/sql/sqlexport.cpp \
  src/sql/sqlquery.cpp \
  src/sql/sqlrecord.cpp \
  src/sql/sqlscript.cpp \
  src/sql/sqltransaction.cpp \
  src/sql/sqlutil.cpp \
  src/templateengine/template.cpp \
  src/templateengine/templatecache.cpp \
  src/templateengine/templateloader.cpp \
  src/util/csvreader.cpp \
  src/util/filesystemwatcher.cpp \
  src/util/heap.cpp \
  src/util/htmlbuilder.cpp \
  src/util/httpdownloader.cpp \
  src/util/paintercontextsaver.cpp \
  src/util/properties.cpp \
  src/util/roundedpolygon.cpp \
  src/util/timedcache.cpp \
  src/util/updatecheck.cpp \
  src/util/version.cpp \
  src/win/activationcontext.cpp \
  src/wmm/GeomagnetismLibrary.c \
  src/wmm/magdectool.cpp \
  src/zip/gzip.cpp \
  src/zip/zip.cpp

# =====================================================================
# Flight simulator files

!isEqual(ATOOLS_NO_FS, "true") {
HEADERS += \
  src/fs/bgl/ap/airport.h \
  src/fs/bgl/ap/approach.h \
  src/fs/bgl/ap/approachleg.h \
  src/fs/bgl/ap/approachtypes.h \
  src/fs/bgl/ap/apron.h \
  src/fs/bgl/ap/apron2.h \
  src/fs/bgl/ap/apronedgelight.h \
  src/fs/bgl/ap/com.h \
  src/fs/bgl/ap/del/deleteairport.h \
  src/fs/bgl/ap/del/deletecom.h \
  src/fs/bgl/ap/del/deleterunway.h \
  src/fs/bgl/ap/del/deletestart.h \
  src/fs/bgl/ap/fence.h \
  src/fs/bgl/ap/helipad.h \
  src/fs/bgl/ap/jetway.h \
  src/fs/bgl/ap/parking.h \
  src/fs/bgl/ap/rw/runway.h \
  src/fs/bgl/ap/rw/runwayapplights.h \
  src/fs/bgl/ap/rw/runwayend.h \
  src/fs/bgl/ap/rw/runwayvasi.h \
  src/fs/bgl/ap/start.h \
  src/fs/bgl/ap/taxipath.h \
  src/fs/bgl/ap/taxipoint.h \
  src/fs/bgl/ap/transition.h \
  src/fs/bgl/bglbase.h \
  src/fs/bgl/bglfile.h \
  src/fs/bgl/bglposition.h \
  src/fs/bgl/boundary.h \
  src/fs/bgl/boundarysegment.h \
  src/fs/bgl/converter.h \
  src/fs/bgl/header.h \
  src/fs/bgl/nav/airwaysegment.h \
  src/fs/bgl/nav/airwaywaypoint.h \
  src/fs/bgl/nav/dme.h \
  src/fs/bgl/nav/glideslope.h \
  src/fs/bgl/nav/ils.h \
  src/fs/bgl/nav/ilsvor.h \
  src/fs/bgl/nav/localizer.h \
  src/fs/bgl/nav/marker.h \
  src/fs/bgl/nav/navbase.h \
  src/fs/bgl/nav/ndb.h \
  src/fs/bgl/nav/tacan.h \
  src/fs/bgl/nav/vor.h \
  src/fs/bgl/nav/waypoint.h \
  src/fs/bgl/nl/namelist.h \
  src/fs/bgl/nl/namelistentry.h \
  src/fs/bgl/record.h \
  src/fs/bgl/recordtypes.h \
  src/fs/bgl/section.h \
  src/fs/bgl/sectiontype.h \
  src/fs/bgl/subsection.h \
  src/fs/bgl/util.h \
  src/fs/common/airportindex.h \
  src/fs/common/binarygeometry.h \
  src/fs/common/globereader.h \
  src/fs/common/magdecreader.h \
  src/fs/common/metadatawriter.h \
  src/fs/common/morareader.h \
  src/fs/common/procedurewriter.h \
  src/fs/common/xpgeometry.h \
  src/fs/db/airwayresolver.h \
  src/fs/db/ap/airportfilewriter.h \
  src/fs/db/ap/airportwriter.h \
  src/fs/db/ap/approachlegwriter.h \
  src/fs/db/ap/approachwriter.h \
  src/fs/db/ap/apronlightwriter.h \
  src/fs/db/ap/apronwriter.h \
  src/fs/db/ap/comwriter.h \
  src/fs/db/ap/deleteairportwriter.h \
  src/fs/db/ap/deleteprocessor.h \
  src/fs/db/ap/fencewriter.h \
  src/fs/db/ap/helipadwriter.h \
  src/fs/db/ap/legbasewriter.h \
  src/fs/db/ap/parkingwriter.h \
  src/fs/db/ap/rw/runwayendwriter.h \
  src/fs/db/ap/rw/runwaywriter.h \
  src/fs/db/ap/startwriter.h \
  src/fs/db/ap/taxipathwriter.h \
  src/fs/db/ap/transitionlegwriter.h \
  src/fs/db/ap/transitionwriter.h \
  src/fs/db/databasemeta.h \
  src/fs/db/datawriter.h \
  src/fs/db/dbairportindex.h \
  src/fs/db/meta/bglfilewriter.h \
  src/fs/db/meta/sceneryareawriter.h \
  src/fs/db/nav/airwaysegmentwriter.h \
  src/fs/db/nav/boundarywriter.h \
  src/fs/db/nav/ilswriter.h \
  src/fs/db/nav/markerwriter.h \
  src/fs/db/nav/ndbwriter.h \
  src/fs/db/nav/tacanwriter.h \
  src/fs/db/nav/vorwriter.h \
  src/fs/db/nav/waypointwriter.h \
  src/fs/db/routeedgewriter.h \
  src/fs/db/runwayindex.h \
  src/fs/db/writerbase.h \
  src/fs/db/writerbasebasic.h \
  src/fs/dfd/dfdcompiler.h \
  src/fs/fspaths.h \
  src/fs/navdatabase.h \
  src/fs/navdatabaseerrors.h \
  src/fs/navdatabaseoptions.h \
  src/fs/navdatabaseprogress.h \
  src/fs/ns/navserver.h \
  src/fs/ns/navservercommon.h \
  src/fs/ns/navserverworker.h \
  src/fs/online/onlinedatamanager.h \
  src/fs/online/onlinetypes.h \
  src/fs/online/statustextparser.h \
  src/fs/online/whazzuptextparser.h \
  src/fs/perf/aircraftperf.h \
  src/fs/perf/aircraftperfconstants.h \
  src/fs/perf/aircraftperfhandler.h \
  src/fs/pln/flightplan.h \
  src/fs/pln/flightplanconstants.h \
  src/fs/pln/flightplanentry.h \
  src/fs/pln/flightplanio.h \
  src/fs/progresshandler.h \
  src/fs/scenery/addoncfg.h \
  src/fs/scenery/addoncomponent.h \
  src/fs/scenery/addonpackage.h \
  src/fs/scenery/fileresolver.h \
  src/fs/scenery/sceneryarea.h \
  src/fs/scenery/scenerycfg.h \
  src/fs/userdata/userdatamanager.h \
  src/fs/util/coordinates.h \
  src/fs/util/fsutil.h \
  src/fs/util/morsecode.h \
  src/fs/util/tacanfrequencies.h \
  src/fs/xp/airwaypostprocess.h \
  src/fs/xp/scenerypacks.h \
  src/fs/xp/xpairportwriter.h \
  src/fs/xp/xpairspacewriter.h \
  src/fs/xp/xpairwaywriter.h \
  src/fs/xp/xpcifpwriter.h \
  src/fs/xp/xpconstants.h \
  src/fs/xp/xpdatacompiler.h \
  src/fs/xp/xpfixwriter.h \
  src/fs/xp/xpnavwriter.h \
  src/fs/xp/xpwriter.h

SOURCES += \
  src/fs/bgl/ap/airport.cpp \
  src/fs/bgl/ap/approach.cpp \
  src/fs/bgl/ap/approachleg.cpp \
  src/fs/bgl/ap/approachtypes.cpp \
  src/fs/bgl/ap/apron.cpp \
  src/fs/bgl/ap/apron2.cpp \
  src/fs/bgl/ap/apronedgelight.cpp \
  src/fs/bgl/ap/com.cpp \
  src/fs/bgl/ap/del/deleteairport.cpp \
  src/fs/bgl/ap/del/deletecom.cpp \
  src/fs/bgl/ap/del/deleterunway.cpp \
  src/fs/bgl/ap/del/deletestart.cpp \
  src/fs/bgl/ap/fence.cpp \
  src/fs/bgl/ap/helipad.cpp \
  src/fs/bgl/ap/jetway.cpp \
  src/fs/bgl/ap/parking.cpp \
  src/fs/bgl/ap/rw/runway.cpp \
  src/fs/bgl/ap/rw/runwayapplights.cpp \
  src/fs/bgl/ap/rw/runwayend.cpp \
  src/fs/bgl/ap/rw/runwayvasi.cpp \
  src/fs/bgl/ap/start.cpp \
  src/fs/bgl/ap/taxipath.cpp \
  src/fs/bgl/ap/taxipoint.cpp \
  src/fs/bgl/ap/transition.cpp \
  src/fs/bgl/bglbase.cpp \
  src/fs/bgl/bglfile.cpp \
  src/fs/bgl/bglposition.cpp \
  src/fs/bgl/boundary.cpp \
  src/fs/bgl/boundarysegment.cpp \
  src/fs/bgl/converter.cpp \
  src/fs/bgl/header.cpp \
  src/fs/bgl/nav/airwaysegment.cpp \
  src/fs/bgl/nav/airwaywaypoint.cpp \
  src/fs/bgl/nav/dme.cpp \
  src/fs/bgl/nav/glideslope.cpp \
  src/fs/bgl/nav/ils.cpp \
  src/fs/bgl/nav/ilsvor.cpp \
  src/fs/bgl/nav/localizer.cpp \
  src/fs/bgl/nav/marker.cpp \
  src/fs/bgl/nav/navbase.cpp \
  src/fs/bgl/nav/ndb.cpp \
  src/fs/bgl/nav/tacan.cpp \
  src/fs/bgl/nav/vor.cpp \
  src/fs/bgl/nav/waypoint.cpp \
  src/fs/bgl/nl/namelist.cpp \
  src/fs/bgl/nl/namelistentry.cpp \
  src/fs/bgl/record.cpp \
  src/fs/bgl/recordtypes.cpp \
  src/fs/bgl/section.cpp \
  src/fs/bgl/sectiontype.cpp \
  src/fs/bgl/subsection.cpp \
  src/fs/bgl/util.cpp \
  src/fs/common/airportindex.cpp \
  src/fs/common/binarygeometry.cpp \
  src/fs/common/globereader.cpp \
  src/fs/common/magdecreader.cpp \
  src/fs/common/metadatawriter.cpp \
  src/fs/common/morareader.cpp \
  src/fs/common/procedurewriter.cpp \
  src/fs/common/xpgeometry.cpp \
  src/fs/db/airwayresolver.cpp \
  src/fs/db/ap/airportfilewriter.cpp \
  src/fs/db/ap/airportwriter.cpp \
  src/fs/db/ap/approachlegwriter.cpp \
  src/fs/db/ap/approachwriter.cpp \
  src/fs/db/ap/apronlightwriter.cpp \
  src/fs/db/ap/apronwriter.cpp \
  src/fs/db/ap/comwriter.cpp \
  src/fs/db/ap/deleteairportwriter.cpp \
  src/fs/db/ap/deleteprocessor.cpp \
  src/fs/db/ap/fencewriter.cpp \
  src/fs/db/ap/helipadwriter.cpp \
  src/fs/db/ap/legbasewriter.cpp \
  src/fs/db/ap/parkingwriter.cpp \
  src/fs/db/ap/rw/runwayendwriter.cpp \
  src/fs/db/ap/rw/runwaywriter.cpp \
  src/fs/db/ap/startwriter.cpp \
  src/fs/db/ap/taxipathwriter.cpp \
  src/fs/db/ap/transitionlegwriter.cpp \
  src/fs/db/ap/transitionwriter.cpp \
  src/fs/db/databasemeta.cpp \
  src/fs/db/datawriter.cpp \
  src/fs/db/dbairportindex.cpp \
  src/fs/db/meta/bglfilewriter.cpp \
  src/fs/db/meta/sceneryareawriter.cpp \
  src/fs/db/nav/airwaysegmentwriter.cpp \
  src/fs/db/nav/boundarywriter.cpp \
  src/fs/db/nav/ilswriter.cpp \
  src/fs/db/nav/markerwriter.cpp \
  src/fs/db/nav/ndbwriter.cpp \
  src/fs/db/nav/tacanwriter.cpp \
  src/fs/db/nav/vorwriter.cpp \
  src/fs/db/nav/waypointwriter.cpp \
  src/fs/db/routeedgewriter.cpp \
  src/fs/db/runwayindex.cpp \
  src/fs/db/writerbasebasic.cpp \
  src/fs/dfd/dfdcompiler.cpp \
  src/fs/fspaths.cpp \
  src/fs/navdatabase.cpp \
  src/fs/navdatabaseerrors.cpp \
  src/fs/navdatabaseoptions.cpp \
  src/fs/navdatabaseprogress.cpp \
  src/fs/ns/navserver.cpp \
  src/fs/ns/navservercommon.cpp \
  src/fs/ns/navserverworker.cpp \
  src/fs/online/onlinedatamanager.cpp \
  src/fs/online/onlinetypes.cpp \
  src/fs/online/statustextparser.cpp \
  src/fs/online/whazzuptextparser.cpp \
  src/fs/perf/aircraftperf.cpp \
  src/fs/perf/aircraftperfconstants.cpp \
  src/fs/perf/aircraftperfhandler.cpp \
  src/fs/pln/flightplan.cpp \
  src/fs/pln/flightplanconstants.cpp \
  src/fs/pln/flightplanentry.cpp \
  src/fs/pln/flightplanio.cpp \
  src/fs/progresshandler.cpp \
  src/fs/scenery/addoncfg.cpp \
  src/fs/scenery/addoncomponent.cpp \
  src/fs/scenery/addonpackage.cpp \
  src/fs/scenery/fileresolver.cpp \
  src/fs/scenery/sceneryarea.cpp \
  src/fs/scenery/scenerycfg.cpp \
  src/fs/userdata/userdatamanager.cpp \
  src/fs/util/coordinates.cpp \
  src/fs/util/fsutil.cpp \
  src/fs/util/morsecode.cpp \
  src/fs/util/tacanfrequencies.cpp \
  src/fs/xp/airwaypostprocess.cpp \
  src/fs/xp/scenerypacks.cpp \
  src/fs/xp/xpairportwriter.cpp \
  src/fs/xp/xpairspacewriter.cpp \
  src/fs/xp/xpairwaywriter.cpp \
  src/fs/xp/xpcifpwriter.cpp \
  src/fs/xp/xpconstants.cpp \
  src/fs/xp/xpdatacompiler.cpp \
  src/fs/xp/xpfixwriter.cpp \
  src/fs/xp/xpnavwriter.cpp \
  src/fs/xp/xpwriter.cpp
} # ATOOLS_NO_FS


# =====================================================================
# GRIB2 decoding files

!isEqual(ATOOLS_NO_GRIB, "true") {
HEADERS += \
  src/g2clib/drstemplates.h \
  src/g2clib/grib2.h \
  src/g2clib/gridtemplates.h \
  src/g2clib/pdstemplates.h \
  src/grib/gribcommon.h \
  src/grib/gribdownloader.h \
  src/grib/gribreader.h

SOURCES += \
  src/g2clib/cmplxpack.c \
  src/g2clib/compack.c \
  src/g2clib/comunpack.c \
  src/g2clib/dec_jpeg2000.c \
  src/g2clib/dec_png.c \
  src/g2clib/drstemplates.c \
  src/g2clib/enc_jpeg2000.c \
  src/g2clib/enc_png.c \
  src/g2clib/g2_addfield.c \
  src/g2clib/g2_addgrid.c \
  src/g2clib/g2_addlocal.c \
  src/g2clib/g2_create.c \
  src/g2clib/g2_free.c \
  src/g2clib/g2_getfld.c \
  src/g2clib/g2_gribend.c \
  src/g2clib/g2_info.c \
  src/g2clib/g2_miss.c \
  src/g2clib/g2_unpack1.c \
  src/g2clib/g2_unpack2.c \
  src/g2clib/g2_unpack3.c \
  src/g2clib/g2_unpack4.c \
  src/g2clib/g2_unpack5.c \
  src/g2clib/g2_unpack6.c \
  src/g2clib/g2_unpack7.c \
  src/g2clib/gbits.c \
  src/g2clib/getdim.c \
  src/g2clib/getpoly.c \
  src/g2clib/gridtemplates.c \
  src/g2clib/int_power.c \
  src/g2clib/jpcpack.c \
  src/g2clib/jpcunpack.c \
  src/g2clib/misspack.c \
  src/g2clib/mkieee.c \
  src/g2clib/pack_gp.c \
  src/g2clib/pdstemplates.c \
  src/g2clib/pngpack.c \
  src/g2clib/pngunpack.c \
  src/g2clib/rdieee.c \
  src/g2clib/reduce.c \
  src/g2clib/seekgb.c \
  src/g2clib/simpack.c \
  src/g2clib/simunpack.c \
  src/g2clib/specpack.c \
  src/g2clib/specunpack.c \
  src/grib/gribcommon.cpp \
  src/grib/gribdownloader.cpp \
  src/grib/gribreader.cpp
} # ATOOLS_NO_GRIB


RESOURCES += \
  atools.qrc

OTHER_FILES += \
  resources/sql/fs/db/README.txt \
  *.ts \
  .travis.yml \
  .gitignore \
  BUILD.txt \
  CHANGELOG.txt \
  LICENSE.txt \
  README.txt \
  uncrustify.cfg

TRANSLATIONS = \
  atools_de.ts \
  atools_es.ts \
  atools_fr.ts \
  atools_it.ts \
  atools_nl.ts \
  atools_zh.ts \
  atools_pt_BR.ts

# Linux specific deploy target
unix:!macx {
  # Creates a list of mkdir and cp commands separated by && to copy the header files from $$HEADERS
  # First parameter is the destination folder, e.g. "../deploy/atools/include"
  defineReplace(copyHeaderFilesCommands) {
    headers =
    dest = $$1
    destpath = $$eval(dest) # /home/YOU/Projects/deploy/atools/include

    for(name, HEADERS) {
      header = $$absolute_path($$name)               # /home/YOU/Projects/atools/src/fs/xp/xpnavwriter.h
      dpath = $$relative_path($$header, $${PWD}/src) # fs/xp/xpnavwriter.h
      headers += mkdir -pv $${destpath}/$$dirname(dpath) && cp -v $$header $${destpath}/$${dpath} &&
    }
    return($$headers)
  }

  DEPLOY_DIR=\"$$DEPLOY_BASE/$$TARGET_NAME\"
  DEPLOY_DIR_LIB=\"$$DEPLOY_BASE/$$TARGET_NAME/lib\"
  DEPLOY_DIR_INCLUDE=\"$$DEPLOY_BASE/$$TARGET_NAME/include\"

  deploy.commands = rm -Rfv $$DEPLOY_DIR &&
  deploy.commands += mkdir -pv $$DEPLOY_DIR_LIB &&
  deploy.commands += mkdir -pv $$DEPLOY_DIR_INCLUDE &&
  deploy.commands += $$copyHeaderFilesCommands($$DEPLOY_DIR_INCLUDE)
  deploy.commands += cp -Rvf $$PWD/atools*.qm $$DEPLOY_DIR &&
  deploy.commands += cp -Rvf $$OUT_PWD/libatools.a $$DEPLOY_DIR_LIB &&
  deploy.commands += cp -vf $$PWD/CHANGELOG.txt $$DEPLOY_DIR &&
  deploy.commands += cp -vf $$PWD/README.txt $$DEPLOY_DIR &&
  deploy.commands += cp -vf $$PWD/LICENSE.txt $$DEPLOY_DIR
}

# Windows specific deploy target only for release builds
win32 {
  defineReplace(p){return ($$shell_quote($$shell_path($$1)))}

  # Creates a list of xcopy commands separated by && to copy the header files from $$HEADERS
  # First parameter is the destination folder, e.g. "../deploy/atools/include"
  defineReplace(copyHeaderFilesCommands) {
    headers =
    dest = $$1
    destpath = $$eval(dest) # C:/Projects/deploy/atools/include

    for(name, HEADERS) {
      header = $$absolute_path($$name)               # C:/Users/YOU/Projects/atools/src/fs/xp/xpnavwriter.h
      dpath = $$relative_path($$header, $${PWD}/src) # fs/xp/xpnavwriter.h
      dpath2 = $${destpath}/$${dpath}
      headers += xcopy $$p($$header) $$p($$dirname(dpath2)) &&
    }
    return($$headers)
  }

  deploy.commands = rmdir /s /q $$p($$DEPLOY_BASE/$$TARGET_NAME) &
  deploy.commands += mkdir $$p($$DEPLOY_BASE/$$TARGET_NAME) &&
  deploy.commands += mkdir $$p($$DEPLOY_BASE/$$TARGET_NAME/lib) &&
  deploy.commands += mkdir $$p($$DEPLOY_BASE/$$TARGET_NAME/include) &&
  deploy.commands += xcopy /T /E $$p($$PWD/src) $$p($$DEPLOY_BASE/$$TARGET_NAME/include) &&
  deploy.commands += $$copyHeaderFilesCommands($$DEPLOY_BASE/$$TARGET_NAME/include)
  deploy.commands += xcopy $$p($$OUT_PWD/libatools.a) $$p($$DEPLOY_BASE/$$TARGET_NAME/lib) &&
  deploy.commands += xcopy $$p($${PWD}/*.qm) $$p($$DEPLOY_BASE/$$TARGET_NAME) &&
  deploy.commands += xcopy $$p($$PWD/CHANGELOG.txt) $$p($$DEPLOY_BASE/$$TARGET_NAME) &&
  deploy.commands += xcopy $$p($$PWD/README.txt) $$p($$DEPLOY_BASE/$$TARGET_NAME) &&
  deploy.commands += xcopy $$p($$PWD/LICENSE.txt) $$p($$DEPLOY_BASE/$$TARGET_NAME)
}

# =====================================================================
# Additional targets

# Deploy needs compiling before
deploy.depends = all

QMAKE_EXTRA_TARGETS += deploy all
