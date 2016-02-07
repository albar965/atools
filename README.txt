atools is a small static library extending Qt for exception handling,
a log4j like logging framework, Flight Simulator related utilities like BGL reader 
and more.

-------------------------------------------------------------------------------
Modules (by directory in "src"):

* fs
Microsoft Flight Simulator related functionality. Contains a class that automatically finds
flight simulator installtions and related paths.

** fs/ap
Simple runways.xml reading tool. Needs MakeRunways by Peter Dowson.

** fs/bgl
A complete collection of classes that read airport and navigation data information from FS BGL files.
** fs/writer
A collection of classes that takes the airport and navigation information read from the BGL files and 
writes them into a relational database format (currently MySql, MariaDB or Sqlite).
** fs/scenery
Supports reading of the flight simulator scenery.cfg file.

** lb
Flight simulator logbook reading functionality.

* io
Simple binary file reading functionality also using exceptions to ease error handling.

* geo
Simple geomentry module.

* gui
GUI and dialog helper classes.

* logging
log4j like logging using the QDebug class. Supports log level filtering into multiple files and file rollover 
to keep multiple log files.

* settings
Wrapper around the QSettings class to provide the settings system wide as a singleton.

* sql
Wrapper around Qt SQL classes but with added exception handling to avoid boilerplate coding.

-------------------------------------------------------------------------------
This software is licensed under GPL3 or any later version.

The source code for this application is available at Github:
https://github.com/albar965/atools

Copyright 2015-2016 Alexander Barthel (albar965@mailbox.org).

Acknowledgements

Thanks to the community around fsdeveloper.com. The BGL reader would not be possible
without their documentation at http://www.fsdeveloper.com/wiki/index.php?title=BGL_File_Format.

Thanks to lc0277 (http://lc0277.nerim.net) for providing an example how to read
the logbook file.
