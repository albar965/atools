/*
 * BglReaderOptions.cpp
 *
 *  Created on: 21.05.2015
 *      Author: alex
 */

#include "bglreaderoptions.h"

#include <QList>
#include <QDebug>

namespace atools {
namespace fs {

BglReaderOptions::BglReaderOptions()
{
  // TODO BglReaderOptions::BglReaderOptions
  // : opts::OptionGroupBase("bglreader", "BglReader description", "BglReader help description"), // TODO help
  // verbose(false), noDeletes(false), noFilterRunways(false), noIncomplete(false), debugAutocommit(false)
  // addOption("verbose", 'v', "Verbose logging", verbose);
  // addOptionFilename("scenery", 's', "Scenery.cfg file", sceneryFile);
  // addOptionFilename("basepath", 'b', "Base path for all bgl files file", basepath);
  // addOption("no-delete", 'd', "Do not process airport deletes", noDeletes);
  // addOption("no-filter-runways", 'f', "Do not filter out dummy runways", noFilterRunways);
  // addOption("no-incomplete", 'i', "Do not write incomplete objects", noIncomplete);
  // addOption("file-filter", 0, "Filter files by regular expression", fileFilterRegexpStr);
  // addOption("airport-icao-filter", 0, "Filter airports by regular expression for ICAO",
  // airportIcaoFilterRegexpStr);
  // addOption("debug-autocommit", 0, "Use autocommit (slow)",
  // Glib::OptionEntry::FLAG_HIDDEN, debugAutocommit);
}

// TODO BglReaderOptions::on_post_parse
// bool BglReaderOptions::on_post_parse(Glib::OptionContext& context, Glib::OptionGroup& group)
// {
// if(!remainingArgs.empty())
// throw Glib::OptionError(Glib::OptionError::UNKNOWN_OPTION,
// "Found extra arguments: " + remainingArgs.at(0) + ", ...");

// testFile(sceneryFile, "Scenery.cfg");
// testDirectory(basepath, "Base path");

// for(vecustrings::const_iterator iter = fileFilterRegexpStr.begin();
// iter != fileFilterRegexpStr.end();
// ++iter)
// if(!iter->empty())
// fileFilterRegexp.push_back(Glib::Regex::create(*iter, Glib::REGEX_OPTIMIZE | Glib::REGEX_CASELESS));

// for(vecustrings::const_iterator iter = airportIcaoFilterRegexpStr.begin();
// iter != airportIcaoFilterRegexpStr.end(); ++iter)
// if(!iter->empty())
// airportIcaoFilterRegexp.push_back(Glib::Regex::create(*iter, Glib::REGEX_OPTIMIZE |
// Glib::REGEX_CASELESS));
// return true;
// }

bool BglReaderOptions::doesFilenameMatch(const QString& filename) const
{
  if(fileFilterRegexp.isEmpty())
    return true;

  for(const QRegularExpression& iter : fileFilterRegexp)
    if(iter.match(filename).hasMatch())
      return true;

  return false;
}

bool BglReaderOptions::doesAirportIcaoMatch(const QString& icao) const
{
  if(airportIcaoFilterRegexp.empty())
    return true;

  for(const QRegularExpression& iter : airportIcaoFilterRegexp)
    if(iter.match(icao).hasMatch())
      return true;

  return false;
}

QDebug operator<<(QDebug out, const BglReaderOptions& opts)
{
  out << "Options[verbose " << opts.verbose
  << ", sceneryFile \"" << opts.sceneryFile
  << "\", basepath \"" << opts.basepath
  << "\", noDeletes " << opts.noDeletes
  << "\", noIncomplete " << opts.noIncomplete
  << ", debugAutocommit " << opts.debugAutocommit;

  out << ", File filter [";
  out << opts.fileFilterRegexpStr;
  out << "]";

  out << ", Airport filter [";
  out << opts.airportIcaoFilterRegexpStr;
  out << "]";
  out << "]";
  return out;
}

} // namespace fs
} // namespace atools
