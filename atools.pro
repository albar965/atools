#-------------------------------------------------
#
# Project created by QtCreator 2015-08-17T13:16:57
#
#-------------------------------------------------

QT       += sql xml core widgets

QT       -= gui

#QMAKE_CXXFLAGS += -std=c++11
CONFIG += c++11

INCLUDEPATH += $$PWD/src

win32:DEFINES += GIT_REVISION_ATOOLS='\\"$$system('C:\\Git\\bin\\git' rev-parse --short HEAD)\\"'
win32:DEFINES += _USE_MATH_DEFINES
unix:DEFINES += GIT_REVISION_ATOOLS='\\"$$system(git rev-parse --short HEAD)\\"'

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
    src/fs/bgl/ap/apronlight.h \
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
    src/fs/bgl/bglexception.h \
    src/fs/bgl/bglfile.h \
    src/fs/bgl/bglposition.h \
    src/fs/bgl/boundary.h \
    src/fs/bgl/boundaryline.h \
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
    src/fs/bglreaderoptions.h \
    src/fs/bgl/record.h \
    src/fs/bgl/recordtypes.h \
    src/fs/bgl/section.h \
    src/fs/bgl/sectiontype.h \
    src/fs/bgl/subsection.h \
    src/fs/bgl/util.h \
    src/fs/db/airportindex.h \
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
    src/fs/db/boundarylinewriter.h \
    src/fs/db/boundarywriter.h \
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
    src/fs/scenery/inireader.h \
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
    src/logging/loggingdefs.h \
    src/logging/logginghandler.h \
    src/logging/loggingutil.h \
    src/settings/settings.h \
    src/sql/sqldatabase.h \
    src/sql/sqlexception.h \
    src/sql/sqlexport.h \
    src/sql/sqlquery.h \
    src/sql/sqlscript.h \
    src/sql/sqlutil.h \
    src/fs/db/progresshandler.h \
    src/fs/bglreaderprogressinfo.h \
    src/gui/tablezoomhandler.h \
    src/gui/widgetstate.h \
    src/gui/widgettools.h \
    src/gui/helphandler.h \
    src/geo/linestring.h \
    src/gui/actiontextsaver.h \
    src/fs/bgl/nav/airwayentry.h \
    src/fs/bgl/nav/airwaywaypoint.h \
    src/fs/db/nav/tempairwaywriter.h \
    src/fs/db/airwayresolver.h \
    src/fs/db/routeedgewriter.h \
    src/fs/sc/simconnectdata.h \
    src/fs/sc/simconnectreply.h \
    src/fs/sc/types.h \
    src/sql/sqlrecord.h \
    src/util/heap.h \
    src/util/htmlbuilder.h \
    src/util/morsecode.h \
    src/gui/filehistoryhandler.h \
    src/gui/mapposhistory.h \
    src/fs/db/databasemeta.h \
    src/zip/zipwriter.h \
    src/zip/zipreader.h \
    src/gui/application.h

SOURCES += src/atools.cpp \
    src/exception.cpp \
    src/fs/ap/airportloader.cpp \
    src/fs/bgl/ap/airport.cpp \
    src/fs/bgl/ap/approach.cpp \
    src/fs/bgl/ap/approachleg.cpp \
    src/fs/bgl/ap/approachtypes.cpp \
    src/fs/bgl/ap/apron2.cpp \
    src/fs/bgl/ap/apron.cpp \
    src/fs/bgl/ap/apronlight.cpp \
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
    src/fs/bgl/bglexception.cpp \
    src/fs/bgl/bglfile.cpp \
    src/fs/bgl/bglposition.cpp \
    src/fs/bgl/boundary.cpp \
    src/fs/bgl/boundaryline.cpp \
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
    src/fs/bglreaderoptions.cpp \
    src/fs/bgl/record.cpp \
    src/fs/bgl/recordtypes.cpp \
    src/fs/bgl/section.cpp \
    src/fs/bgl/sectiontype.cpp \
    src/fs/bgl/subsection.cpp \
    src/fs/bgl/util.cpp \
    src/fs/db/airportindex.cpp \
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
    src/fs/db/boundarylinewriter.cpp \
    src/fs/db/boundarywriter.cpp \
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
    src/fs/scenery/inireader.cpp \
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
    src/fs/db/progresshandler.cpp \
    src/fs/bglreaderprogressinfo.cpp \
    src/gui/tablezoomhandler.cpp \
    src/gui/widgetstate.cpp \
    src/gui/widgettools.cpp \
    src/gui/helphandler.cpp \
    src/geo/linestring.cpp \
    src/gui/actiontextsaver.cpp \
    src/fs/bgl/nav/airwayentry.cpp \
    src/fs/bgl/nav/airwaywaypoint.cpp \
    src/fs/db/nav/tempairwaywriter.cpp \
    src/fs/db/airwayresolver.cpp \
    src/fs/db/routeedgewriter.cpp \
    src/fs/sc/simconnectdata.cpp \
    src/fs/sc/simconnectreply.cpp \
    src/sql/sqlrecord.cpp \
    src/util/heap.cpp \
    src/util/htmlbuilder.cpp \
    src/util/morsecode.cpp \
    src/gui/filehistoryhandler.cpp \
    src/gui/mapposhistory.cpp \
    src/fs/db/databasemeta.cpp \
    src/zip/zip.cpp \
    src/gui/application.cpp


unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES += \
    LICENSE.txt \
    README.txt \
    uncrustify.cfg \
    CHANGELOG.txt \
    BUILD.txt

RESOURCES += \
    atools.qrc
