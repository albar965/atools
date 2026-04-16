/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "gui/dataexchange.h"

#include "atools.h"
#include "gui/application.h"
#include "gui/dataexchangeflags.h"
#include "settings/settings.h"
#include "util/properties.h"

#include <QDataStream>
#include <QSharedMemory>
#include <QDateTime>
#include <QDir>
#include <QThread>

using atools::settings::Settings;

namespace atools {
namespace gui {

static const qsizetype SHARED_MEMORY_SIZE = 8192;
static const int FETCH_TIMER_INTERVAL_MS = 500;
static const qint64 MAX_TIME_DIFFENCE_MS = 1000;

DataExchangeFetcher::DataExchangeFetcher(bool verboseParam)
  : verbose(verboseParam)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO;

  setObjectName("DataExchangeFetcher");
  timer = new QTimer(this);
  timer->setSingleShot(true);
  connect(timer, &QTimer::timeout, this, &DataExchangeFetcher::fetchSharedMemory);
}

DataExchangeFetcher::~DataExchangeFetcher()
{
  if(timer != nullptr)
    timer->stop();

  ATOOLS_DELETE_LOG(timer);
}

void DataExchangeFetcher::startTimer()
{
  if(verbose)
    qDebug() << Q_FUNC_INFO;

  if(timer != nullptr)
    timer->start(FETCH_TIMER_INTERVAL_MS);
  else
    qWarning() << Q_FUNC_INFO << "Timer is null";
}

void DataExchangeFetcher::fetchSharedMemory()
{
  // sharedMemory is accessed exclusively by this thread here

  atools::util::Properties properties;

  if(sharedMemory != nullptr)
  {
    // Already attached
    if(sharedMemory->lock())
    {
      if(verbose)
        qDebug() << Q_FUNC_INFO << "locked";

      // Read size only for now
      quint32 size;
      // Create a view on the shared memory for reading - does not copy
      QByteArray sizeBytes = QByteArray::fromRawData(static_cast<const char *>(sharedMemory->constData()), SHARED_MEMORY_SIZE);
      QDataStream in(&sizeBytes, QIODevice::ReadOnly);
      in >> size;

      if(size > 0)
      {
        // Read properties if size is given - skip timestamp
        in.skipRawData(sizeof(qint64));
        in >> properties;
      }

      if(verbose)
        qDebug() << Q_FUNC_INFO << "Updating timestamp" << QDateTime::currentMSecsSinceEpoch();

      // Write back a null size and update timestamp to allow the other process detect a crash
      QByteArray bytesEmpty;
      QDataStream out(&bytesEmpty, QIODevice::WriteOnly);
      out << static_cast<quint32>(0) << QDateTime::currentMSecsSinceEpoch();
      memcpy(sharedMemory->data(), bytesEmpty.constData(), static_cast<size_t>(bytesEmpty.size()));

      sharedMemory->unlock();
    }
  }
  else
    qWarning() << Q_FUNC_INFO << "sharedMemory is null";

  if(!properties.isEmpty())
  {
    // Found properties, i.e. a message - send signal
    qInfo() << Q_FUNC_INFO << properties;
    emit dataFetched(properties);
  }

  timer->start(FETCH_TIMER_INTERVAL_MS);
}

// ================================================================================================
DataExchange::DataExchange(const QString& programGuid, bool noDataExchange, bool verboseParam)
  : QObject(nullptr), verbose(verboseParam)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << programGuid << "noDataExchange" << noDataExchange;
  exit = false;

  // Copy command line parameters and add activate option to bring other to front
  atools::util::Properties properties(atools::gui::Application::getStartupOptions());

  // sharedMemory is accessess exclusively by the constructor here
  // Detect other running application instance with same settings - this is unsafe on Unix since sharedMemory can remain after crashes
  if(sharedMemory == nullptr)
    sharedMemory = new QSharedMemory(programGuid + "-" + atools::cleanPath(QFileInfo(Settings::getPath()).canonicalFilePath()));

  // Create and attach
  if(sharedMemory->create(SHARED_MEMORY_SIZE, QSharedMemory::ReadWrite))
  {
    if(verbose)
      qDebug() << Q_FUNC_INFO << "Created" << sharedMemory->key() << sharedMemory->nativeKey();

    // Created =====================================================================
    // Write null size and timestamp into the shared memory segment - not sending message here
    // A crash is detected if timestamp is not updated
    QByteArray bytes;
    QDataStream out(&bytes, QIODevice::WriteOnly);
    out << static_cast<quint32>(0) << QDateTime::currentMSecsSinceEpoch();

    if(sharedMemory->lock())
    {
      memcpy(sharedMemory->data(), bytes.constData(), static_cast<size_t>(bytes.size()));
      sharedMemory->unlock();
    }
  }
  else
  {
    // Attach to already present one if not attached =========================================
    if(sharedMemory->attach(QSharedMemory::ReadWrite))
    {
      if(verbose)
        qDebug() << Q_FUNC_INFO << "Attached" << sharedMemory->key() << sharedMemory->nativeKey();

      // Read timestamp to detect freeze or crash
      if(!noDataExchange && sharedMemory->lock())
      {
        qint64 datetime;
        quint32 size;
        // Create a view on the shared memory for reading - does not copy but needs a lock
        QByteArray sizeBytes = QByteArray::fromRawData(static_cast<const char *>(sharedMemory->constData()), SHARED_MEMORY_SIZE);
        QDataStream in(&sizeBytes, QIODevice::ReadOnly);
        in >> size >> datetime;

        qint64 timeDiff = QDateTime::fromMSecsSinceEpoch(datetime).msecsTo(QDateTime::currentDateTimeUtc());

        if(verbose)
          qDebug() << Q_FUNC_INFO << "Time difference" << timeDiff;

        // If timestamp is older than MAX_TIME_DIFFENCE_MS other might be crashed or frozen, start normally - otherwise send message
        if(timeDiff < MAX_TIME_DIFFENCE_MS)
        {
          if(verbose)
            qDebug() << Q_FUNC_INFO << "Sending commands";

          if(!properties.contains(atools::gui::dataexchange::STARTUP_COMMAND_QUIT))
            // Always raise other window except on quit
            properties.setPropertyStr(atools::gui::dataexchange::STARTUP_COMMAND_ACTIVATE, QStringLiteral());

          if(verbose)
            qDebug() << Q_FUNC_INFO << "Sending" << properties;

          // Get properties for size
          QByteArray propBytes = properties.asByteArray();

          // Write off for other process
          QByteArray bytes;
          QDataStream out(&bytes, QIODevice::WriteOnly);
          // Use original unchanged timestamp here to avoid updating due to restarts
          out << static_cast<quint32>(propBytes.size()) << datetime;
          out.writeRawData(propBytes.constData(), propBytes.size());

          if(bytes.size() < sharedMemory->size())
          {
            memcpy(sharedMemory->data(), bytes.constData(), static_cast<size_t>(bytes.size()));
            exit = true;
          }
          else
            qWarning() << Q_FUNC_INFO << "sharedMemory is too small" << sharedMemory->size();

        } // if(QDateTime::fromMSecsSinceEpoch(date ...
        else if(verbose)
          qDebug() << Q_FUNC_INFO << "Starting normally";

        sharedMemory->unlock();
      } // if(sharedMemory->lock())
    }
  }

  if(verbose && sharedMemory != nullptr)
    qDebug() << Q_FUNC_INFO << "key" << sharedMemory->key() << "native key" << sharedMemory->nativeKey();

  if(properties.contains(atools::gui::dataexchange::STARTUP_COMMAND_QUIT))
    exit = true;

  if(verbose)
    qDebug() << Q_FUNC_INFO << "exit" << exit;
}

DataExchange::~DataExchange()
{
  disable();
}

void DataExchange::disable()
{
  if(dataFetcherThread != nullptr)
  {
    dataFetcherThread->quit();
    dataFetcherThread->wait();
  }

  ATOOLS_DELETE_LATER_LOG(dataFetcherThread);
  ATOOLS_DELETE_LATER_LOG(dataFetcher);

  if(sharedMemory != nullptr)
    sharedMemory->detach();
  ATOOLS_DELETE_LATER_LOG(sharedMemory);
}

void DataExchange::startTimer()
{
  if(verbose)
    qDebug() << Q_FUNC_INFO;

  dataFetcher = new DataExchangeFetcher(verbose);
  dataFetcher->setSharedMemory(sharedMemory);

  // Connect thread signal to main thread slot
  connect(dataFetcher, &DataExchangeFetcher::dataFetched, this, &DataExchange::dataFetched, Qt::QueuedConnection);

  dataFetcherThread = new QThread(this);
  dataFetcherThread->setObjectName("DataFetcherThread");

  // Move worker to thread
  dataFetcher->moveToThread(dataFetcherThread);

  // Start timer in thread context once started
  connect(dataFetcherThread, &QThread::started, dataFetcher, &DataExchangeFetcher::startTimer);
  dataFetcherThread->start();
}

} // namespace gui
} // namespace atools
