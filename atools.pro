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
unix:DEFINES += GIT_REVISION_ATOOLS='\\"$$system(git rev-parse --short HEAD)\\"'

TARGET = atools
TEMPLATE = lib
CONFIG += staticlib

HEADERS += src/atools.h \
    src/logging/logginghandler.h \
    src/sql/sqldatabase.h \
    src/sql/sqlquery.h \
    src/sql/sqlexception.h \
    src/sql/sqlscript.h \
    src/sql/sqlutil.h \
    src/sql/sqlexport.h \
    src/io/binarystream.h \
    src/settings/settings.h \
    src/gui/errorhandler.h \
    src/gui/dialog.h \
    src/gui/translator.h \
    src/geo/calculations.h \
    src/geo/pos.h \
    src/fs/lb/logbook.h \
    src/fs/lb/logbookentry.h \
    src/fs/lb/types.h \
    src/fs/ap/airportloader.h \
    src/exception.h \
    src/logging/loggingutil.h \
    src/logging/loggingconfig.h \
    src/io/fileroller.h \
    src/fs/lb/logbookloader.h \
    src/logging/loggingdefs.h \
    src/fs/lb/logbookentryfilter.h \
    src/fs/fspaths.h \
    src/fs/bgl/ap/del/deleteairport.h \
    src/fs/bgl/ap/del/deletecom.h \
    src/fs/bgl/ap/del/deleterunway.h \
    src/fs/bgl/ap/rw/runway.h \
    src/fs/bgl/ap/rw/runwayapplights.h \
    src/fs/bgl/ap/rw/runwayend.h \
    src/fs/bgl/ap/rw/runwayvasi.h \
    src/fs/bgl/ap/airport.h \
    src/fs/bgl/ap/approach.h \
    src/fs/bgl/ap/com.h \
    src/fs/bgl/ap/parking.h \
    src/fs/bgl/ap/transition.h \
    src/fs/bgl/nav/dme.h \
    src/fs/bgl/nav/glideslope.h \
    src/fs/bgl/nav/ils.h \
    src/fs/bgl/nav/ilsvor.h \
    src/fs/bgl/nav/localizer.h \
    src/fs/bgl/nav/marker.h \
    src/fs/bgl/nav/navbase.h \
    src/fs/bgl/nav/ndb.h \
    src/fs/bgl/nav/routeentry.h \
    src/fs/bgl/nav/routewaypoint.h \
    src/fs/bgl/nav/vor.h \
    src/fs/bgl/nav/waypoint.h \
    src/fs/bgl/nl/namelist.h \
    src/fs/bgl/nl/namelistentry.h \
    src/fs/bgl/bglbase.h \
    src/fs/bgl/bglexception.h \
    src/fs/bgl/bglfile.h \
    src/fs/bgl/bglposition.h \
    src/fs/bgl/converter.h \
    src/fs/bgl/header.h \
    src/fs/bgl/record.h \
    src/fs/bgl/recordtypes.h \
    src/fs/bgl/section.h \
    src/fs/bgl/sectiontype.h \
    src/fs/bgl/subsection.h \
    src/fs/bgl/util.h \
    src/fs/bglreaderoptions.h \
    src/fs/scenery/fileresolver.h \
    src/fs/scenery/sceneryarea.h \
    src/fs/scenery/scenerycfg.h \
    src/fs/scenery/inireader.h \
    src/fs/writer/writerbase.h \
    src/fs/writer/writerbasebasic.h \
    src/fs/writer/ap/rw/runwayendwriter.h \
    src/fs/writer/ap/rw/runwaywriter.h \
    src/fs/writer/ap/airportwriter.h \
    src/fs/writer/ap/approachwriter.h \
    src/fs/writer/ap/comwriter.h \
    src/fs/writer/ap/deleteairportwriter.h \
    src/fs/writer/ap/deleteprocessor.h \
    src/fs/writer/ap/parkingwriter.h \
    src/fs/writer/ap/transitionwriter.h \
    src/fs/writer/meta/bglfilewriter.h \
    src/fs/writer/meta/sceneryareawriter.h \
    src/fs/writer/nav/ilswriter.h \
    src/fs/writer/nav/markerwriter.h \
    src/fs/writer/nav/ndbwriter.h \
    src/fs/writer/nav/temproutewriter.h \
    src/fs/writer/nav/vorwriter.h \
    src/fs/writer/nav/waypointwriter.h \
    src/fs/writer/airportindex.h \
    src/fs/writer/datawriter.h \
    src/fs/writer/routeresolver.h \
    src/fs/writer/runwayindex.h \
    src/fs/navdatabase.h \
    src/fs/bgl/ap/helipad.h \
    src/fs/bgl/ap/start.h \
    src/fs/writer/ap/helipadwriter.h \
    src/fs/writer/ap/startwriter.h

SOURCES += src/atools.cpp \
    src/logging/logginghandler.cpp \
    src/sql/sqldatabase.cpp \
    src/sql/sqlquery.cpp \
    src/sql/sqlexception.cpp \
    src/sql/sqlscript.cpp \
    src/sql/sqlutil.cpp \
    src/sql/sqlexport.cpp \
    src/io/binarystream.cpp \
    src/settings/settings.cpp \
    src/gui/errorhandler.cpp \
    src/gui/dialog.cpp \
    src/gui/translator.cpp \
    src/geo/calculations.cpp \
    src/fs/lb/logbook.cpp \
    src/fs/lb/logbookentry.cpp \
    src/fs/ap/airportloader.cpp \
    src/exception.cpp \
    src/logging/loggingutil.cpp \
    src/logging/loggingconfig.cpp \
    src/io/fileroller.cpp \
    src/fs/lb/logbookloader.cpp \
    src/fs/lb/types.cpp \
    src/fs/lb/logbookentryfilter.cpp \
    src/fs/fspaths.cpp \
    src/fs/bgl/ap/del/deleteairport.cpp \
    src/fs/bgl/ap/del/deletecom.cpp \
    src/fs/bgl/ap/del/deleterunway.cpp \
    src/fs/bgl/ap/rw/runway.cpp \
    src/fs/bgl/ap/rw/runwayapplights.cpp \
    src/fs/bgl/ap/rw/runwayend.cpp \
    src/fs/bgl/ap/rw/runwayvasi.cpp \
    src/fs/bgl/ap/airport.cpp \
    src/fs/bgl/ap/approach.cpp \
    src/fs/bgl/ap/com.cpp \
    src/fs/bgl/ap/parking.cpp \
    src/fs/bgl/ap/transition.cpp \
    src/fs/bgl/nav/dme.cpp \
    src/fs/bgl/nav/glideslope.cpp \
    src/fs/bgl/nav/ils.cpp \
    src/fs/bgl/nav/ilsvor.cpp \
    src/fs/bgl/nav/localizer.cpp \
    src/fs/bgl/nav/marker.cpp \
    src/fs/bgl/nav/navbase.cpp \
    src/fs/bgl/nav/ndb.cpp \
    src/fs/bgl/nav/routeentry.cpp \
    src/fs/bgl/nav/routewaypoint.cpp \
    src/fs/bgl/nav/vor.cpp \
    src/fs/bgl/nav/waypoint.cpp \
    src/fs/bgl/nl/namelist.cpp \
    src/fs/bgl/nl/namelistentry.cpp \
    src/fs/bgl/bglbase.cpp \
    src/fs/bgl/bglfile.cpp \
    src/fs/bgl/bglposition.cpp \
    src/fs/bgl/converter.cpp \
    src/fs/bgl/header.cpp \
    src/fs/bgl/record.cpp \
    src/fs/bgl/recordtypes.cpp \
    src/fs/bgl/section.cpp \
    src/fs/bgl/sectiontype.cpp \
    src/fs/bgl/subsection.cpp \
    src/fs/bgl/bglexception.cpp \
    src/fs/bgl/util.cpp \
    src/fs/bglreaderoptions.cpp \
    src/fs/scenery/fileresolver.cpp \
    src/fs/scenery/sceneryarea.cpp \
    src/fs/scenery/scenerycfg.cpp \
    src/fs/scenery/inireader.cpp \
    src/fs/writer/writerbasebasic.cpp \
    src/fs/writer/ap/rw/runwayendwriter.cpp \
    src/fs/writer/ap/rw/runwaywriter.cpp \
    src/fs/writer/ap/airportwriter.cpp \
    src/fs/writer/ap/approachwriter.cpp \
    src/fs/writer/ap/comwriter.cpp \
    src/fs/writer/ap/deleteairportwriter.cpp \
    src/fs/writer/ap/deleteprocessor.cpp \
    src/fs/writer/ap/parkingwriter.cpp \
    src/fs/writer/ap/transitionwriter.cpp \
    src/fs/writer/meta/bglfilewriter.cpp \
    src/fs/writer/meta/sceneryareawriter.cpp \
    src/fs/writer/nav/ilswriter.cpp \
    src/fs/writer/nav/markerwriter.cpp \
    src/fs/writer/nav/ndbwriter.cpp \
    src/fs/writer/nav/temproutewriter.cpp \
    src/fs/writer/nav/vorwriter.cpp \
    src/fs/writer/nav/waypointwriter.cpp \
    src/fs/writer/airportindex.cpp \
    src/fs/writer/datawriter.cpp \
    src/fs/writer/routeresolver.cpp \
    src/fs/writer/runwayindex.cpp \
    src/fs/navdatabase.cpp \
    src/fs/bgl/ap/helipad.cpp \
    src/fs/bgl/ap/start.cpp \
    src/fs/writer/ap/helipadwriter.cpp \
    src/fs/writer/ap/startwriter.cpp

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
