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

#include "track/trackdownloader.h"

#include "track/trackreader.h"
#include "util/httpdownloader.h"

using atools::util::HttpDownloader;

namespace atools {
namespace track {

// AUSOTS
// curl  "https://www.airservicesaustralia.com/flextracks/text.asp?ver=1" > AUSOTS.html

// NAT
// curl  "https://notams.aim.faa.gov/nat.html" > NAT.html

// PACOTS
// https://www.notams.faa.gov/dinsQueryWeb/advancedNotamMapAction.do
// ["queryType"] = "pacificTracks", ["actionType"] = "advancedNOTAMFunctions"
// curl --data "queryType=pacificTracks&actionType=advancedNOTAMFunctions" https://www.notams.faa.gov/dinsQueryWeb/advancedNotamMapAction.do >PACOTS.html

TrackDownloader::TrackDownloader(QObject *parent, bool logVerbose) : QObject(parent)
{
  // Initialize NAT downloader ============================================================
  HttpDownloader *natDownloader = new HttpDownloader(parent, logVerbose);
  natDownloader->setUrl("https://notams.aim.faa.gov/nat.html");
  connect(natDownloader, &HttpDownloader::downloadFinished, this, &TrackDownloader::natDownloadFinished);
  connect(natDownloader, &HttpDownloader::downloadFailed, this, &TrackDownloader::natDownloadFailed);
  downloaders.insert(NAT, natDownloader);
  trackList.insert(NAT, atools::track::TrackVectorType());

  // Initialize PACOTS downloader ============================================================
  HttpDownloader *pacotsDownloader = new HttpDownloader(parent, logVerbose);
  pacotsDownloader->setUrl("https://www.notams.faa.gov/dinsQueryWeb/advancedNotamMapAction.do");
  connect(pacotsDownloader, &HttpDownloader::downloadFinished, this, &TrackDownloader::pacotsDownloadFinished);
  connect(pacotsDownloader, &HttpDownloader::downloadFailed, this, &TrackDownloader::pacotsDownloadFailed);
  pacotsDownloader->setPostParameters({
        {"queryType", "pacificTracks"},
        {"actionType", "advancedNOTAMFunctions"}
      });
  downloaders.insert(PACOTS, pacotsDownloader);
  trackList.insert(PACOTS, atools::track::TrackVectorType());

  // Initialize AUSOTS downloader ============================================================
  HttpDownloader *ausotsDownloader = new HttpDownloader(parent, logVerbose);
  ausotsDownloader->setUrl("https://www.airservicesaustralia.com/flextracks/text.asp?ver=1");
  connect(ausotsDownloader, &HttpDownloader::downloadFinished, this, &TrackDownloader::ausotsDownloadFinished);
  connect(ausotsDownloader, &HttpDownloader::downloadFailed, this, &TrackDownloader::ausotsDownloadFailed);
  downloaders.insert(AUSOTS, ausotsDownloader);
  trackList.insert(AUSOTS, atools::track::TrackVectorType());
}

TrackDownloader::~TrackDownloader()
{
  qDeleteAll(downloaders);
}

void TrackDownloader::natDownloadFinished(const QByteArray& data, QString)
{
  TrackReader reader;
  reader.readTracks(data, NAT);
  trackList[NAT] = reader.getTracks();

  emit downloadFinished(trackList.value(NAT), NAT);
}

void TrackDownloader::pacotsDownloadFinished(const QByteArray& data, QString)
{
  TrackReader reader;
  reader.readTracks(data, PACOTS);
  trackList[PACOTS] = reader.getTracks();

  emit downloadFinished(trackList.value(PACOTS), PACOTS);
}

void TrackDownloader::ausotsDownloadFinished(const QByteArray& data, QString)
{
  TrackReader reader;
  reader.readTracks(data, AUSOTS);
  trackList[AUSOTS] = reader.getTracks();

  emit downloadFinished(trackList.value(AUSOTS), AUSOTS);
}

void TrackDownloader::natDownloadFailed(const QString& error, int errorCode, QString downloadUrl)
{
  emit downloadFailed(error, errorCode, downloadUrl, NAT);
}

void TrackDownloader::pacotsDownloadFailed(const QString& error, int errorCode, QString downloadUrl)
{
  emit downloadFailed(error, errorCode, downloadUrl, PACOTS);
}

void TrackDownloader::ausotsDownloadFailed(const QString& error, int errorCode, QString downloadUrl)
{
  emit downloadFailed(error, errorCode, downloadUrl, AUSOTS);
}

void TrackDownloader::setUrl(TrackType type, const QString& url)
{
  downloaders[type]->setUrl(url);
}

void TrackDownloader::setUrl(TrackType type, const QString& url, const QHash<QString, QString>& parameters)
{
  downloaders[type]->setUrl(url);
  downloaders[type]->setPostParameters(parameters);
}

void TrackDownloader::startAllDownloads()
{
  for(HttpDownloader *downloader : downloaders.values())
    downloader->startDownload();
}

void TrackDownloader::startDownload(TrackType type)
{
  downloaders[type]->startDownload();
}

void TrackDownloader::cancelAllDownloads()
{
  for(HttpDownloader *downloader : downloaders.values())
    downloader->cancelDownload();
}

const atools::track::TrackVectorType& TrackDownloader::getTracks(TrackType type)
{
  return trackList[type];
}

void TrackDownloader::clearTracks()
{
  for(atools::track::TrackType key : trackList.keys())
    trackList[key].clear();
}

bool TrackDownloader::hasAnyTracks()
{
  int num = 0;
  for(const TrackVectorType& tracks : trackList.values())
    num += tracks.size();
  return num > 0;
}

bool TrackDownloader::hasTracks(TrackType type)
{
  return !trackList.value(type).isEmpty();
}

int TrackDownloader::removeInvalid()
{
  int num = 0;
  for(atools::track::TrackType key : trackList.keys())
    num += TrackReader::removeInvalid(trackList[key]);
  return num;
}

} // namespace track
} // namespace atools
