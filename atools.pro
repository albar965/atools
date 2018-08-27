#*****************************************************************************
# Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

QT       += sql xml svg core widgets network
QT       -= gui

CONFIG += c++14

# Use to debug release builds
#CONFIG+=force_debug_info

INCLUDEPATH += $$PWD/src

DEFINES += QT_NO_CAST_FROM_BYTEARRAY
DEFINES += QT_NO_CAST_TO_ASCII
#DEFINES += QT_NO_CAST_FROM_ASCII

unix {
  DEFINES += GIT_REVISION_ATOOLS='\\"$$system(git rev-parse --short HEAD)\\"'

  # Enable to allow scenery database loading on Linux or macOS
  #DEFINES+=DEBUG_FS_PATHS
}

win32 {
  DEFINES += GIT_REVISION_ATOOLS='\\"$$system('C:\\Git\\bin\\git' rev-parse --short HEAD)\\"'
  DEFINES += _USE_MATH_DEFINES
  DEFINES += NOMINMAX

  SIMCONNECT="C:\Program Files (x86)\Microsoft Games\Microsoft Flight Simulator X SDK"
  INCLUDEPATH += "C:\Program Files (x86)\Microsoft Games\Microsoft Flight Simulator X SDK\SDK\Core Utilities Kit\SimConnect SDK\inc"
  LIBS += "C:\Program Files (x86)\Microsoft Games\Microsoft Flight Simulator X SDK\SDK\Core Utilities Kit\SimConnect SDK\lib\SimConnect.lib"
}

macx {
  # Compatibility down to OS X Mountain Lion 10.8
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.8
}

TARGET = atools
TEMPLATE = lib
CONFIG += staticlib

HEADERS += src/atools.h \
    src/exception.h \
    src/fs/ap/airportloader.h \
    src/fs/bgl/ap/airport.h \
    src/fs/bgl/ap/approach.h \
    src/fs/bgl/ap/approachleg.h \
    src/fs/bgl/ap/approachtypes.h \
    src/fs/bgl/ap/apron2.h \
    src/fs/bgl/ap/apron.h \
    src/fs/bgl/ap/com.h \
    src/fs/bgl/ap/del/deleteairport.h \
    src/fs/bgl/ap/del/deletecom.h \
    src/fs/bgl/ap/del/deleterunway.h \
    src/fs/bgl/ap/del/deletestart.h \
    src/fs/bgl/ap/fence.h \
    src/fs/bgl/ap/helipad.h \
    src/fs/bgl/ap/jetway.h \
    src/fs/bgl/ap/parking.h \
    src/fs/bgl/ap/rw/runwayapplights.h \
    src/fs/bgl/ap/rw/runwayend.h \
    src/fs/bgl/ap/rw/runway.h \
    src/fs/bgl/ap/rw/runwayvasi.h \
    src/fs/bgl/ap/start.h \
    src/fs/bgl/ap/taxipath.h \
    src/fs/bgl/ap/taxipoint.h \
    src/fs/bgl/ap/transition.h \
    src/fs/bgl/bglbase.h \
    src/fs/bgl/bglfile.h \
    src/fs/bgl/bglposition.h \
    src/fs/bgl/boundary.h \
    src/fs/bgl/converter.h \
    src/fs/bgl/header.h \
    src/fs/bgl/nav/dme.h \
    src/fs/bgl/nav/glideslope.h \
    src/fs/bgl/nav/ils.h \
    src/fs/bgl/nav/ilsvor.h \
    src/fs/bgl/nav/localizer.h \
    src/fs/bgl/nav/marker.h \
    src/fs/bgl/nav/navbase.h \
    src/fs/bgl/nav/ndb.h \
    src/fs/bgl/nav/vor.h \
    src/fs/bgl/nav/waypoint.h \
    src/fs/bgl/nl/namelistentry.h \
    src/fs/bgl/nl/namelist.h \
    src/fs/bgl/record.h \
    src/fs/bgl/recordtypes.h \
    src/fs/bgl/section.h \
    src/fs/bgl/sectiontype.h \
    src/fs/bgl/subsection.h \
    src/fs/bgl/util.h \
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
    src/fs/db/datawriter.h \
    src/fs/db/meta/bglfilewriter.h \
    src/fs/db/meta/sceneryareawriter.h \
    src/fs/db/nav/ilswriter.h \
    src/fs/db/nav/markerwriter.h \
    src/fs/db/nav/ndbwriter.h \
    src/fs/db/nav/vorwriter.h \
    src/fs/db/nav/waypointwriter.h \
    src/fs/db/runwayindex.h \
    src/fs/db/writerbasebasic.h \
    src/fs/db/writerbase.h \
    src/fs/fspaths.h \
    src/fs/lb/logbookentryfilter.h \
    src/fs/lb/logbookentry.h \
    src/fs/lb/logbook.h \
    src/fs/lb/logbookloader.h \
    src/fs/lb/types.h \
    src/fs/navdatabase.h \
    src/fs/pln/flightplanentry.h \
    src/fs/pln/flightplan.h \
    src/fs/scenery/fileresolver.h \
    src/fs/scenery/sceneryarea.h \
    src/fs/scenery/scenerycfg.h \
    src/geo/calculations.h \
    src/geo/pos.h \
    src/geo/rect.h \
    src/gui/dialog.h \
    src/gui/errorhandler.h \
    src/gui/translator.h \
    src/io/binarystream.h \
    src/io/fileroller.h \
    src/logging/loggingconfig.h \
    src/logging/logginghandler.h \
    src/logging/loggingutil.h \
    src/settings/settings.h \
    src/sql/sqldatabase.h \
    src/sql/sqlexception.h \
    src/sql/sqlexport.h \
    src/sql/sqlquery.h \
    src/sql/sqlscript.h \
    src/sql/sqlutil.h \
    src/gui/widgetstate.h \
    src/gui/helphandler.h \
    src/geo/linestring.h \
    src/gui/actiontextsaver.h \
    src/fs/bgl/nav/airwaywaypoint.h \
    src/fs/db/airwayresolver.h \
    src/fs/db/routeedgewriter.h \
    src/fs/sc/simconnectdata.h \
    src/fs/sc/simconnectreply.h \
    src/fs/sc/simconnecttypes.h \
    src/sql/sqlrecord.h \
    src/util/heap.h \
    src/util/htmlbuilder.h \
    src/gui/filehistoryhandler.h \
    src/gui/mapposhistory.h \
    src/fs/db/databasemeta.h \
    src/zip/zipwriter.h \
    src/zip/zipreader.h \
    src/gui/application.h \
    src/util/version.h \
    src/fs/bgl/boundarysegment.h \
    src/fs/bgl/nav/airwaysegment.h \
    src/fs/bgl/ap/apronedgelight.h \
    src/fs/navdatabaseprogress.h \
    src/fs/navdatabaseoptions.h \
    src/io/inireader.h \
    src/fs/db/nav/boundarywriter.h \
    src/fs/db/nav/airwaysegmentwriter.h \
    src/fs/navdatabaseerrors.h \
    src/fs/sc/simconnecthandler.h \
    src/fs/sc/simconnectdatabase.h \
    src/fs/sc/simconnectdummy.h \
    src/fs/sc/simconnectaircraft.h \
    src/fs/sc/simconnectuseraircraft.h \
    src/fs/sc/datareaderthread.h \
    src/fs/sc/weatherrequest.h \
    src/gui/widgetutil.h \
    src/util/timedcache.h \
    src/util/paintercontextsaver.h \
    src/fs/db/ap/airportfilewriter.h \
    src/geo/line.h \
    src/gui/itemviewzoomhandler.h \
    src/gui/griddelegate.h \
    src/fs/bgl/nav/tacan.h \
    src/fs/db/nav/tacanwriter.h \
    src/fs/util/tacanfrequencies.h \
    src/fs/util/morsecode.h \
    src/gui/palettesettings.h \
    src/gui/actionstatesaver.h \
    src/win/activationcontext.h \
    src/fs/sc/simconnectapi.h \
    src/fs/scenery/addonpackage.h \
    src/fs/scenery/addoncomponent.h \
    src/fs/progresshandler.h \
    src/fs/xp/airwaypostprocess.h \
    src/fs/xp/xpconstants.h \
    src/fs/xp/xpwriter.h \
    src/fs/xp/xpnavwriter.h \
    src/fs/xp/xpfixwriter.h \
    src/fs/xp/xpairwaywriter.h \
    src/fs/xp/xpairportwriter.h \
    src/fs/util/fsutil.h \
    src/fs/xp/xpdatacompiler.h \
    src/fs/common/xpgeometry.h \
    src/fs/common/binarygeometry.h \
    src/fs/xp/xpcifpwriter.h \
    src/fs/common/globereader.h \
    src/fs/common/magdecreader.h \
    src/fs/util/coordinates.h \
    src/fs/pln/flightplanconstants.h \
    src/util/updatecheck.h \
    src/fs/sc/connecthandler.h \
    src/fs/ns/navservercommon.h \
    src/fs/ns/navserver.h \
    src/fs/ns/navserverworker.h \
    src/fs/sc/xpconnecthandler.h \
    src/gui/consoleapplication.h \
    src/logging/loggingguiabort.h \
    src/fs/xp/xpairspacewriter.h \
    src/fs/common/metadatawriter.h \
    src/fs/common/airportindex.h \
    src/fs/db/dbairportindex.h \
    src/fs/dfd/dfdcompiler.h \
    src/fs/common/procedurewriter.h \
    src/fs/scenery/addoncfg.h \
    src/fs/userdata/userdatamanager.h \
    src/util/csvreader.h \
    src/fs/pln/flightplanio.h \
    src/geo/simplespatialindex.h \
    src/fs/weather/weathernetdownload.h \
    src/fs/weather/weathernetsingle.h \
    src/fs/weather/metarparser.h \
    src/fs/weather/metar.h \
    src/fs/weather/weathertypes.h \
    src/fs/weather/xpweatherreader.h \
    src/util/httpdownloader.h \
    src/fs/online/statustextparser.h \
    src/fs/online/whazzuptextparser.h \
    src/fs/online/onlinedatamanager.h \
    src/fs/online/onlinetypes.h \
    src/zip/gzip.h \
    src/sql/sqltransaction.h \
    src/logging/loggingtypes.h \
    src/fs/xp/scenerypacks.h \
    src/fs/common/morareader.h \
    src/util/roundedpolygon.h \
    src/util/filesystemwatcher.h

SOURCES += src/atools.cpp \
    src/exception.cpp \
    src/fs/ap/airportloader.cpp \
    src/fs/bgl/ap/airport.cpp \
    src/fs/bgl/ap/approach.cpp \
    src/fs/bgl/ap/approachleg.cpp \
    src/fs/bgl/ap/approachtypes.cpp \
    src/fs/bgl/ap/apron2.cpp \
    src/fs/bgl/ap/apron.cpp \
    src/fs/bgl/ap/com.cpp \
    src/fs/bgl/ap/del/deleteairport.cpp \
    src/fs/bgl/ap/del/deletecom.cpp \
    src/fs/bgl/ap/del/deleterunway.cpp \
    src/fs/bgl/ap/del/deletestart.cpp \
    src/fs/bgl/ap/fence.cpp \
    src/fs/bgl/ap/helipad.cpp \
    src/fs/bgl/ap/jetway.cpp \
    src/fs/bgl/ap/parking.cpp \
    src/fs/bgl/ap/rw/runwayapplights.cpp \
    src/fs/bgl/ap/rw/runway.cpp \
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
    src/fs/bgl/converter.cpp \
    src/fs/bgl/header.cpp \
    src/fs/bgl/nav/dme.cpp \
    src/fs/bgl/nav/glideslope.cpp \
    src/fs/bgl/nav/ils.cpp \
    src/fs/bgl/nav/ilsvor.cpp \
    src/fs/bgl/nav/localizer.cpp \
    src/fs/bgl/nav/marker.cpp \
    src/fs/bgl/nav/navbase.cpp \
    src/fs/bgl/nav/ndb.cpp \
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
    src/fs/db/datawriter.cpp \
    src/fs/db/meta/bglfilewriter.cpp \
    src/fs/db/meta/sceneryareawriter.cpp \
    src/fs/db/nav/ilswriter.cpp \
    src/fs/db/nav/markerwriter.cpp \
    src/fs/db/nav/ndbwriter.cpp \
    src/fs/db/nav/vorwriter.cpp \
    src/fs/db/nav/waypointwriter.cpp \
    src/fs/db/runwayindex.cpp \
    src/fs/db/writerbasebasic.cpp \
    src/fs/fspaths.cpp \
    src/fs/lb/logbook.cpp \
    src/fs/lb/logbookentry.cpp \
    src/fs/lb/logbookentryfilter.cpp \
    src/fs/lb/logbookloader.cpp \
    src/fs/lb/types.cpp \
    src/fs/navdatabase.cpp \
    src/fs/pln/flightplan.cpp \
    src/fs/pln/flightplanentry.cpp \
    src/fs/scenery/fileresolver.cpp \
    src/fs/scenery/sceneryarea.cpp \
    src/fs/scenery/scenerycfg.cpp \
    src/geo/calculations.cpp \
    src/geo/pos.cpp \
    src/geo/rect.cpp \
    src/gui/dialog.cpp \
    src/gui/errorhandler.cpp \
    src/gui/translator.cpp \
    src/io/binarystream.cpp \
    src/io/fileroller.cpp \
    src/logging/loggingconfig.cpp \
    src/logging/logginghandler.cpp \
    src/logging/loggingutil.cpp \
    src/settings/settings.cpp \
    src/sql/sqldatabase.cpp \
    src/sql/sqlexception.cpp \
    src/sql/sqlexport.cpp \
    src/sql/sqlquery.cpp \
    src/sql/sqlscript.cpp \
    src/sql/sqlutil.cpp \
    src/gui/widgetstate.cpp \
    src/gui/helphandler.cpp \
    src/geo/linestring.cpp \
    src/gui/actiontextsaver.cpp \
    src/fs/db/airwayresolver.cpp \
    src/fs/db/routeedgewriter.cpp \
    src/fs/sc/simconnectdata.cpp \
    src/fs/sc/simconnectreply.cpp \
    src/sql/sqlrecord.cpp \
    src/util/heap.cpp \
    src/util/htmlbuilder.cpp \
    src/gui/filehistoryhandler.cpp \
    src/gui/mapposhistory.cpp \
    src/fs/db/databasemeta.cpp \
    src/zip/zip.cpp \
    src/gui/application.cpp \
    src/util/version.cpp \
    src/fs/bgl/boundarysegment.cpp \
    src/fs/bgl/nav/airwaywaypoint.cpp \
    src/fs/bgl/ap/apronedgelight.cpp \
    src/fs/navdatabaseprogress.cpp \
    src/fs/navdatabaseoptions.cpp \
    src/io/inireader.cpp \
    src/fs/db/nav/boundarywriter.cpp \
    src/fs/db/nav/airwaysegmentwriter.cpp \
    src/fs/navdatabaseerrors.cpp \
    src/fs/sc/simconnecthandler.cpp \
    src/fs/sc/simconnectdatabase.cpp \
    src/fs/sc/simconnectdummy.cpp \
    src/fs/sc/simconnectaircraft.cpp \
    src/fs/sc/simconnectuseraircraft.cpp \
    src/fs/sc/datareaderthread.cpp \
    src/fs/sc/weatherrequest.cpp \
    src/gui/widgetutil.cpp \
    src/util/timedcache.cpp \
    src/fs/bgl/nav/airwaysegment.cpp \
    src/util/paintercontextsaver.cpp \
    src/fs/db/ap/airportfilewriter.cpp \
    src/geo/line.cpp \
    src/gui/itemviewzoomhandler.cpp \
    src/gui/griddelegate.cpp \
    src/fs/bgl/nav/tacan.cpp \
    src/fs/db/nav/tacanwriter.cpp \
    src/fs/util/tacanfrequencies.cpp \
    src/fs/util/morsecode.cpp \
    src/gui/palettesettings.cpp \
    src/gui/actionstatesaver.cpp \
    src/win/activationcontext.cpp \
    src/fs/sc/simconnectapi.cpp \
    src/fs/scenery/addonpackage.cpp \
    src/fs/scenery/addoncomponent.cpp \
    src/fs/progresshandler.cpp \
    src/fs/xp/airwaypostprocess.cpp \
    src/fs/xp/xpconstants.cpp \
    src/fs/xp/xpwriter.cpp \
    src/fs/xp/xpnavwriter.cpp \
    src/fs/xp/xpfixwriter.cpp \
    src/fs/xp/xpairwaywriter.cpp \
    src/fs/xp/xpairportwriter.cpp \
    src/fs/util/fsutil.cpp \
    src/fs/xp/xpdatacompiler.cpp \
    src/fs/common/xpgeometry.cpp \
    src/fs/common/binarygeometry.cpp \
    src/fs/xp/xpcifpwriter.cpp \
    src/fs/common/globereader.cpp \
    src/fs/common/magdecreader.cpp \
    src/fs/util/coordinates.cpp \
    src/fs/pln/flightplanconstants.cpp \
    src/util/updatecheck.cpp \
    src/fs/sc/connecthandler.cpp \
    src/fs/ns/navserver.cpp \
    src/fs/ns/navservercommon.cpp \
    src/fs/ns/navserverworker.cpp \
    src/fs/sc/xpconnecthandler.cpp \
    src/gui/consoleapplication.cpp \
    src/logging/loggingguiabort.cpp \
    src/fs/xp/xpairspacewriter.cpp \
    src/fs/common/metadatawriter.cpp \
    src/fs/common/airportindex.cpp \
    src/fs/db/dbairportindex.cpp \
    src/fs/dfd/dfdcompiler.cpp \
    src/fs/common/procedurewriter.cpp \
    src/fs/scenery/addoncfg.cpp \
    src/fs/userdata/userdatamanager.cpp \
    src/util/csvreader.cpp \
    src/fs/pln/flightplanio.cpp \
    src/geo/simplespatialindex.cpp \
    src/fs/weather/weathernetdownload.cpp \
    src/fs/weather/weathernetsingle.cpp \
    src/fs/weather/metarparser.cpp \
    src/fs/weather/metar.cpp \
    src/fs/weather/weathertypes.cpp \
    src/fs/weather/xpweatherreader.cpp \
    src/util/httpdownloader.cpp \
    src/fs/online/statustextparser.cpp \
    src/fs/online/whazzuptextparser.cpp \
    src/fs/online/onlinedatamanager.cpp \
    src/fs/online/onlinetypes.cpp \
    src/zip/gzip.cpp \
    src/sql/sqltransaction.cpp \
    src/fs/xp/scenerypacks.cpp \
    src/fs/common/morareader.cpp \
    src/util/roundedpolygon.cpp \
    src/util/filesystemwatcher.cpp


unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES += \
    LICENSE.txt \
    README.txt \
    uncrustify.cfg \
    CHANGELOG.txt \
    BUILD.txt \
    resources/sql/fs/db/README.txt

RESOURCES += \
    atools.qrc

TRANSLATIONS = atools_fr.ts \
               atools_it.ts \
               atools_nl.ts \
               atools_de.ts \
               atools_es.ts \
               atools_pt_BR.ts
