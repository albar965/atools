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
    src/fs/fspaths.h

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
    src/fs/fspaths.cpp

unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES += \
    LICENSE.txt \
    README.txt \
    uncrustify.cfg

RESOURCES += \
    atools.qrc
