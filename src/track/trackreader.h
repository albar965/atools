/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#ifndef ATOOLS_TRACKREADER_H
#define ATOOLS_TRACKREADER_H

#include "track/tracktypes.h"

#include <QApplication>

class QTextStream;

namespace atools {
namespace track {

/*
 * Parses HTML pages from the various services for NAT, AUSOTS and PACOTS and returns a list
 * of Track objects.
 *
 * AUSOTS: https://www.airservicesaustralia.com/flextracks/text.asp?ver=1
 * NAT: https://notams.aim.faa.gov/nat.html
 * PACOTS:  https://www.notams.faa.gov/dinsQueryWeb/advancedNotamMapAction.do
 */
class TrackReader
{
  Q_DECLARE_TR_FUNCTIONS(TrackReader)

public:
  TrackReader();
  ~TrackReader();

  /* Read tracks from HTML or text pages. throws an exception on error.
   * Track objects are added to the internal list without clearing it before.*/
  void readTracks(const QString& filename, atools::track::TrackType type);
  void readTracks(QTextStream& stream, atools::track::TrackType type);
  void readTracks(const QByteArray& data, atools::track::TrackType type);

  /* Clear list of tracks */
  void clear()
  {
    tracks.clear();
  }

  /* Get list of tracks after reading. */
  const atools::track::TrackVectorType& getTracks() const
  {
    return tracks;
  }

  /* Removes invalid tracks, reports as warning and returns removed number. */
  int removeInvalid();
  static int removeInvalid(atools::track::TrackVectorType& trackVector);

private:
  /* Read all lines from stream into a list. Lines are simplified and empty ones are dropped. */
  QStringList readLines(QTextStream& stream);

  /* Extract all sections between begin and end. begin and end are included.
   * Returns a list of lines with all text between sections cleared. */
  QStringList extractSections(const QStringList& lines, const QString& begin, const QString& end);

  /* Convert month acronyms of full names into month numbers. */
  int monthFromStr(const QString& str);

  /* Convert e.g. "58/20" to "5820N" and "5530/20" to "H5530". */
  QStringList toNatWaypoints(const QStringList& str);

  /* Extract all tracks into the track vector that begin with nameRegexp and end with a ")".
   * nameRegexp needs to have a group returning the name. */
  void extractTracks(const QStringList& lines, const QRegularExpression& nameRegexp, TrackType type, bool removeEmpty);

  /* Extract NAT tracks into the track vector. */
  void extractNatTracks(const QStringList& lines);

  /* Extract PACOTS tracks into the track vector. */
  void extractPacotsTracks(const QStringList& lines);

  /* Extract the PACOTS flex tracks into the track vector. */
  void extractPacotsTracksFlex(const QStringList& text);

  /* Extract AUSOTS tracks into the track vector. */
  void extractAusotsTracks(const QStringList& lines);

  atools::track::TrackVectorType tracks;

};

} // namespace track
} // namespace atools

#endif // ATOOLS_TRACKREADER_H
