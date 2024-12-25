/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_TRACKDOWNLOADER_H
#define ATOOLS_TRACKDOWNLOADER_H

#include "track/tracktypes.h"

#include <QObject>

namespace atools {
namespace util {
class HttpDownloader;
}
namespace track {

/*
 * Downloads HTML pages asynchronously from various services for NAT and PACOTS and fills a list
 * of Track objects.
 *
 * Default URLs are:
 * NAT: https://notams.aim.faa.gov/nat.html
 * PACOTS: https://www.notams.faa.gov/dinsQueryWeb/advancedNotamMapAction.do
 *         Uses POST with parameters "queryType=pacificTracks&actionType=advancedNOTAMFunctions"
 */
class TrackDownloader :
  public QObject
{
  Q_OBJECT

public:
  explicit TrackDownloader(QObject *parent, bool logVerbose = false);
  virtual ~TrackDownloader() override;

  TrackDownloader(const TrackDownloader& other) = delete;
  TrackDownloader& operator=(const TrackDownloader& other) = delete;

  /* Set URL for GET request for given track type */
  void setUrl(atools::track::TrackType type, const QString& url);

  /* Set URL and parameters for POST request for given track type */
  void setUrl(atools::track::TrackType type, const QString& url, const QHash<QString, QString>& parameters);
  void setUrl(atools::track::TrackType type, const QString& url, const QStringList& parameters);

  /* Start downloads. downloadFinished or downloadFailed will be emitted once done. */
  void startAllDownloads();
  void startDownload(atools::track::TrackType type);
  void cancelAllDownloads();

  /* Get downloaded tracks. */
  const TrackVectorType& getTracks(atools::track::TrackType type);

  /* Remove downloaded tracks. */
  void clearTracks();

  /* true if any track or a track for the given type exists. */
  bool hasAnyTracks();
  bool hasTracks(atools::track::TrackType type);

  /* Removes invalid tracks, reports as warning and returns removed number. */
  int removeInvalid();

  /* Public default values */
  static const QHash<atools::track::TrackType, QString> URL;
  static const QHash<atools::track::TrackType, QStringList> PARAM;

  /* Set to true to ignore any certificate validation or other SSL errors.
   * downloadSslErrors is emitted in case of SSL errors.
   * Sets value to all downloaders. */
  void setIgnoreSslErrors(bool value);

signals:
  /* Emitted when HTML page was downloaded and parsed */
  void trackDownloadFinished(const atools::track::TrackVectorType& tracks, atools::track::TrackType type);

  /* Emitted if download failed */
  void trackDownloadFailed(const QString& error, int errorCode, QString downloadUrl, atools::track::TrackType type);

  /* Emitted on SSL errors. Call setIgnoreSslErrors to ignore future errors and continue.  */
  void trackDownloadSslErrors(const QStringList& errors, const QString& downloadUrl);

private:
  void natDownloadFinished(const QByteArray& data, QString downloadUrl);
  void pacotsDownloadFinished(const QByteArray& data, QString downloadUrl);

  void natDownloadFailed(const QString& error, int errorCode, QString downloadUrl);
  void pacotsDownloadFailed(const QString& error, int errorCode, QString downloadUrl);

  /* Download classes */
  QHash<atools::track::TrackType, atools::util::HttpDownloader *> downloaders;

  /* List of tracks for each type */
  QHash<atools::track::TrackType, atools::track::TrackVectorType> trackList;

  bool verbose = false;
};

} // namespace track
} // namespace atools

#endif // ATOOLS_TRACKDOWNLOADER_H
