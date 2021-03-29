atools is a static library extending Qt for exception handling,
a log4j like logging framework, Flight Simulator related utilities like BGL reader
and more.

-------------------------------------------------------------------------------
Modules (by directory in "src"):

* fs
Microsoft Flight Simulator related functionality. Contains a class that automatically finds
flight simulator installations and related paths.

** fs/ap
Simple runways.xml reading tool. Needs MakeRunways by Peter Dowson.

** fs/bgl
A complete collection of classes that read airport and navigation data information from FS BGL files.

** fs/db
A collection of classes that takes the airport and navigation information read from the BGL files and
writes them into a relational database format (currently Sqlite).
See atools/resources/sql/fs/db/README.txt for schema for more information about the database schema.

** fs/scenery
Supports reading of the flight simulator scenery.cfg file.

** fs/pln
Support for reading and writing flight simulator flight plan (PLN/XML) files.

** fs/lb
Flight simulator logbook reading functionality.

* io
Simple binary file reading functionality also using exceptions to ease error handling. Also a
file/log rolling class and a reader for ini files.

* geo
Simple geometry module containing point and rectangle classes as well as various complex calculations.

* gui
GUI and dialog helper classes.

* logging
log4j like logging using the QDebug class. Supports log level filtering into multiple files and file rollover
to keep multiple log files.

* settings
Wrapper around the QSettings class to provide the settings system wide as a singleton.

* sql
Wrapper around Qt SQL classes but with added exception handling to avoid excessive boilerplate
coding for error checks.

* util
Miscellaneous utilities.

* zip
A copy of the unsupported and Qt zip class. Improved for better error handling.

------------------------------------------------------------------------------
-- LICENSE -------------------------------------------------------------------
------------------------------------------------------------------------------

This software is licensed under GPL3 or any later version.

The source code for this application is available at Github:
https://github.com/albar965/atools

Copyright 2015-2020 Alexander Barthel (https://albar965.github.io/contact.html).

-------------------------------------------------------------------------------
French translation copyright 2017 Patrick JUNG alias Patbest (patrickjung@laposte.net).

