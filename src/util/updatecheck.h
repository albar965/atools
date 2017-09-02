/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_UPDATECHECK_H
#define ATOOLS_UPDATECHECK_H

#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace atools {
namespace util {

/* Updates can be fetched for each channel in one call */
enum UpdateChannel
{
  NONE = 0,
  STABLE = 1 << 0,
  BETA = 1 << 1,
  DEVELOP = 1 << 2
};

Q_DECLARE_FLAGS(UpdateChannels, UpdateChannel);
Q_DECLARE_OPERATORS_FOR_FLAGS(atools::util::UpdateChannels);

struct Update
{
  atools::util::UpdateChannels channel; /* The used update channel */
  QString url, /* URL with general information like a release page */
          version, /* the offered version */
          changelog, /* HTML changelog */
          download; /* A download URL for the used operating system */
};

typedef QVector<atools::util::Update> UpdateList;

/*
 * This class provides methods to search for updates by loading a file from a HTTP server.
 * Three channels, stable, beta and development are available.
 *
 * Access is asynchronous. The signal passes a list of found updates to the client.
 *
 * Example:
 *
 * [stable]
 * version=1.4.4
 * url=https://albar965.github.io/release/2017/07/04/littlenavmap-stable-144-released.html
 * changelog=<ul> <li>Added light (Positron) and dark (Dark Matter) maps from CARTO.</li> ...
 * downloadwin=https://github.com/albar965/littlenavmap/releases/download/v1.4.4/LittleNavmap-win-1.4.4.zip
 * downloadmac=https://github.com/albar965/littlenavmap/releases/download/v1.4.4/LittleNavmap-macOS-1.4.4.dmg
 * downloadlinux=https://github.com/albar965/littlenavmap/releases/download/v1.4.4/LittleNavmap-linux-1.4.4.tar.gz
 *
 * # [beta] Comment
 * # version=1.6.0.beta
 * # url=https://albar965.github.io/release/2017/07/04/littlenavmap-stable-144-released.html
 * # changelog=<ul><li>First beta</li><li>Second beta</li></ul><p>Notes: Do not delete</p>
 *
 * [develop]
 * version=1.5.3.develop
 * downloadwin=https://www.dropbox.com/s/v98jflpxphns4x0/LittleNavmap-20170730-1853.zip
 * changelog=<ul> <li>Fixed missing encoding in PLN XML files since changing to Qt 5.9.</li> ...
 */

class UpdateCheck :
  public QObject
{
  Q_OBJECT

public:
  explicit UpdateCheck(bool forceDebug);

  /*
   * @param programVersion Current program version
   * @param forceDebug always report update for testing purposes
   */
  explicit UpdateCheck(const QString& programVersion, bool forceDebug);
  virtual ~UpdateCheck();

  /*
   * Check for updates and send a message updateFound or updateFailed.
   *
   * @param versionsAlreadChecked Versions to skip from the message
   * @param notifyForEmptyUpdates Also send a signal if nothing was found
   * @param updateChannels Channels to check
   */
  void checkForUpdates(const QString& versionsAlreadChecked, bool notifyForEmptyUpdates,
                       atools::util::UpdateChannels updateChannels);

  /* Get the URL of the text file that contains the update information */
  const QUrl& getUrl() const
  {
    return url;
  }

  void setUrl(const QString& value)
  {
    url = QUrl(value);
  }

signals:
  /* Sent if updates were found */
  void updateFound(UpdateList updates);

  /* Sent on error */
  void updateFailed(QString errorString);

private:
  void httpFinished();
  void endRequest();
  void httpError(QNetworkReply::NetworkError code);
  void readUpdateMessage(atools::util::UpdateList& updates, QString update);

  atools::util::UpdateChannels channels = atools::util::STABLE;

  /* Version of calling program */
  QString curProgramVersion;

  /* Skip all version earlier or equal this one */
  QString alreadyChecked;

  QNetworkAccessManager networkManager;
  QNetworkReply *reply = nullptr;
  QUrl url;

  /* Also send message if nothing was found */
  bool notifyEmptyUpdates = false, debug = false;

};

} // namespace util
} // namespace atools

#endif // ATOOLS_UPDATECHECK_H
