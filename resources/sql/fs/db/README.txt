General Notes about the FS Scenery Library Database Schema
----------------------------------------------------------

* Foreign keys are tested but per default not enabled during loading (for performance reasons)
* Boolean columns have prefixes like "has_" or "is_" and contain 0 or 1 values but are not nullable
* Frequencies are scaled up to integer values: MHz * 1000 for COM, VOR, ILS and kHz * 100 for NDB frequencies
* Altitude and runway dimensions are in feet and radio navaid ranges are in NM.
  See separate comments for the table columns for used units.
* Headings are degree true.

* Types are stored as strings and can be looked up as enumerations in the C++ code. Note that some values do not
  correspond directly to the enums but are converted by string conversion functions.
  For example QString approachFixTypeToStr(atools::fs::bgl::ap::ApproachFixType type);

* See the following links for more information:
  http://www.fsdeveloper.com/wiki/index.php?title=BGL_File_Format
  https://msdn.microsoft.com/en-us/library/cc526978.aspx
