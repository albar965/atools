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

#include "util/updatecheck.h"
#include "util/version.h"

#include <QNetworkReply>
#include <QCoreApplication>

namespace atools {
namespace util {

UpdateCheck::UpdateCheck(bool forceDebug)
  : curProgramVersion(QCoreApplication::applicationVersion()), debug(forceDebug)
{

}

UpdateCheck::UpdateCheck(const QString& programVersion, bool forceDebug)
  : curProgramVersion(programVersion), debug(forceDebug)
{

}

UpdateCheck::~UpdateCheck()
{
  endRequest();
}

void UpdateCheck::checkForUpdates(const QString& versionsAlreadChecked, bool notifyForEmptyUpdates,
                                  atools::util::UpdateChannels updateChannels)
{
  qDebug() << Q_FUNC_INFO << url << curProgramVersion;

  endRequest();

  alreadyChecked = versionsAlreadChecked;
  notifyEmptyUpdates = notifyForEmptyUpdates;
  channels = updateChannels;

  // Post the request
  QNetworkRequest request(url);
  reply = networkManager.get(request);

  if(reply != nullptr)
  {
    // Connect signals for this request
    connect(reply, &QNetworkReply::finished, this, &UpdateCheck::httpFinished);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(reply, &QNetworkReply::errorOccurred, this, &UpdateCheck::httpError);
#else
    connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), this, &UpdateCheck::httpError);
#endif
  }
}

/* Called by reply */
void UpdateCheck::httpFinished()
{
  qDebug() << Q_FUNC_INFO;

  if(reply != nullptr)
  {
    if(reply->error() == QNetworkReply::NoError)
    {
      // Read received file
      QString update = reply->readAll();
      UpdateList updates;
      readUpdateMessage(updates, update);

      if(!updates.isEmpty() || notifyEmptyUpdates /* also send empty updates */)
        emit updateFound(updates);
    }
    else if(reply->error() != QNetworkReply::OperationCanceledError)
      qWarning() << "Request for" << url << "failed. Reason:" << reply->errorString();
  }

  endRequest();
}

/* Called by reply */
void UpdateCheck::httpError(QNetworkReply::NetworkError code)
{
  qDebug() << Q_FUNC_INFO;

  Q_UNUSED(code)

  if(reply != nullptr)
  {
    qWarning() << "Request for" << url << "failed. Reason:" << reply->errorString();
    emit updateFailed(reply->errorString());
  }
  else
    emit updateFailed(tr("Unknown Error"));

  endRequest();
}

/* Parse the update file */
void UpdateCheck::readUpdateMessage(UpdateList& updates, QString update)
{
  Version programVersion(curProgramVersion);
  Version alreadyCheckedVersion(alreadyChecked);
  QTextStream versioninfo(&update, QIODevice::ReadOnly);

  QHash<UpdateChannels, Update> map;
  UpdateChannels currentChannel = NONE;
  // Changelog will continue until a new keyword, a section or an empty line is found
  bool changelogContinuation = false;
  while(!versioninfo.atEnd())
  {
    QString rawLine = versioninfo.readLine();
    QString line = rawLine.trimmed();

    // Skip comments and empty lines
    if(line.isEmpty() || line.startsWith("#"))
    {
      changelogContinuation = false;
      continue;
    }

    if(line.startsWith("["))
    {
      // Start of a section
      QString section = line.toLower();
      if(section == "[stable]")
        currentChannel = STABLE;
      else if(section == "[beta]")
        currentChannel = BETA;
      else if(section == "[develop]")
        currentChannel = DEVELOP;

      changelogContinuation = false;
    }
    else
    {
      if(channels & currentChannel)
      {
        QString key = line.section('=', 0, 0).trimmed().toLower();
        QString rawValue = line.section('=', 1).trimmed();
        QString value = rawValue.trimmed();

        if(key == "version")
        {
          Version version(value);
          if(!alreadyCheckedVersion.isValid() || version > alreadyCheckedVersion || debug)
          {
            // Check if there is a newer version
            if(version > programVersion || debug)
            {
              map[currentChannel].version = value;
              map[currentChannel].channel = currentChannel;
            }
            // else Leave version empty if it is to be skipped
          }
          changelogContinuation = false;
        }
        else if(key == "changelog" || key == "description")
        {
          // Start of changelog
          map[currentChannel].changelog = rawValue;
          changelogContinuation = true;
        }
        else if(changelogContinuation)
          // More changelog lines
          map[currentChannel].changelog += " " + line;
      }
    }
  }

  // Copy only valid updates that have a version
  for(auto it = map.begin(); it != map.end(); ++it)
  {
    Update& upd = it.value();
    if(!upd.version.isEmpty())
    {
      // Remove line breaks and multiple spaces
      upd.changelog = upd.changelog.simplified();
      updates.append(upd);
    }
  }

  // Put the newest versions on top
  std::sort(updates.begin(), updates.end(), [](const Update& upd1, const Update& upd2) -> bool
      {
        return atools::util::Version(upd1.version) > atools::util::Version(upd2.version);
      });
}

void UpdateCheck::setForceDebug(bool value)
{
  debug = value;
}

/* Remove reply */
void UpdateCheck::endRequest()
{
  if(reply != nullptr)
  {
    disconnect(reply, &QNetworkReply::finished, this, &UpdateCheck::httpFinished);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    disconnect(reply, &QNetworkReply::errorOccurred, this, &UpdateCheck::httpError);
#else
    disconnect(reply, static_cast<void (QNetworkReply::*)(
                                    QNetworkReply::NetworkError)>(&QNetworkReply::error), this, &UpdateCheck::httpError);
#endif
    reply->abort();
    reply->deleteLater();
    reply = nullptr;

    notifyEmptyUpdates = false;
  }
}

} // namespace util
} // namespace atools
