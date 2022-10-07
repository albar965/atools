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

#include "fs/sc/datareaderthread.h"

#include "fs/sc/simconnecthandler.h"
#include "fs/sc/xpconnecthandler.h"
#include "atools.h"

#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QDataStream>
#include <QCoreApplication>
#include <QDir>

namespace atools {
namespace fs {
namespace sc {

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
using Qt::hex;
using Qt::dec;
using Qt::endl;
#endif

void DataReaderThread::debugWriteWhazzup(const atools::fs::sc::SimConnectData& dataPacket)
{
  static QDateTime last;

  // VATSIM
  // ; !CLIENTS section -
  // callsign:cid:realname:clienttype:frequency:latitude:longitude:altitude:groundspeed:planned_aircraft: planned_tascruise:planned_depairport:planned_altitude:planned_destairport:server:protrevision:rating :transponder:facilitytype:visualrange:planned_revision:planned_flighttype:planned_deptime:planned_ac tdeptime:planned_hrsenroute:planned_minenroute:planned_hrsfuel:planned_minfuel:planned_altairport:pl anned_remarks:planned_route:planned_depairport_lat:planned_depairport_lon:planned_destairport_lat:pl anned_destairport_lon:atis_message:time_last_atis_received:time_logon:heading:QNH_iHg:QNH_Mb:

  // !GENERAL:
  // VERSION = 8
  // RELOAD = 2
  // UPDATE = 20180322170014
  // ATIS ALLOW MIN = 5
  // CONNECTED CLIENTS = 586

  // !CLIENTS:
  // 4XAIA:1383303:Name LLBG:PILOT::32.18188:34.82802:125:0::0::::SWEDEN:100:1:1200::::::::::::::::::::20180322165257:105:29.919:1013:

  QDateTime now = dataPacket.getUserAircraftConst().getZuluTime();

  if(!last.isValid() || (now.toMSecsSinceEpoch() >= last.toMSecsSinceEpoch() + whazzupUpdateSeconds * 1000))
  {
    QFile file(replayWhazzupFile);

    qDebug() << Q_FUNC_INFO << "Writing" << file.fileName();

    // Post log message only once on first call
    if(!last.isValid())
      emit postLogMessage(tr("Writing to \"%1\" in VATSIM format every %2 seconds.").
                          arg(file.fileName()).arg(whazzupUpdateSeconds), false, false);

    if(file.open(QIODevice::WriteOnly))
    {
      QTextStream stream(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
      stream.setCodec("UTF-8");
#endif

      // GENERAL header =====================================================================
      stream << "!GENERAL:" << endl;
      stream << "VERSION = 8" << endl;
      stream << "RELOAD = 2" << endl;
      stream << "UPDATE = " << QDateTime::currentDateTimeUtc().toString("yyyyMMddhhmmss") << endl; // 2018 0322 170014
      stream << "ATIS ALLOW MIN = 5" << endl;

      const QVector<atools::fs::sc::SimConnectAircraft>& aircraft = dataPacket.getAiAircraftConst();
      atools::fs::sc::SimConnectUserAircraft user = dataPacket.getUserAircraftConst();
      user.setFlag(atools::fs::sc::IS_USER);

      // All aircraft including user
      QVector<atools::fs::sc::SimConnectAircraft> aircraftFiltered;

#ifdef DEBUG_CREATE_WHAZZUP_TEST
      // Change all aircraft registrations
      user.setAirplaneRegistration(QString(user.getAirplaneRegistration()).replace(0, 2, "XX"));
#endif

      aircraftFiltered.append(user);

      // Apply filter =====================================================================
      for(atools::fs::sc::SimConnectAircraft ac : aircraft)
      {
        if(ac.isAnyBoat())
          continue;
        // if(!ac.isUser())
        // continue;

        QString reg = ac.getAirplaneRegistration();

#ifdef DEBUG_CREATE_WHAZZUP_TEST_FILTER
        // Remove all except one aircraft
        if(reg != "N2092Z" && !ac.isUser())
          continue;
#endif

#ifdef DEBUG_CREATE_WHAZZUP_TEST

        if(ac.isOnGround())
          continue;

        // Modify data for testing ========
        // Drop N9
        if(reg.startsWith("N9"))
          continue;

        // Change reg for N2
        if(reg.startsWith("N2"))
        {
          reg.replace(0, 2, "XX");
          ac.setAirplaneRegistration(reg);

          // atools::geo::Pos pos = ac.getPosition();
          // pos.setLatY(pos.getLatY() + 0.1f); // Move 6 NM North-East
          // pos.setLonX(pos.getLonX() + 0.1f);
          // ac.setCoordinates(pos);
        }

        //// Move N8
        // if(reg.startsWith("N8"))
        // {
        // atools::geo::Pos pos = ac.getPosition();
        // pos.setLatY(0.1f); // Move 6 NM down and east
        // pos.setLonX(0.1f);
        // ac.setCoordinates(pos);
        // }
#endif
        aircraftFiltered.append(ac);
      }
      stream << "CONNECTED CLIENTS = " << aircraftFiltered.size() << endl;

      // CLIENTS =====================================================================
      stream << "!CLIENTS:" << endl;

      // callsign:cid:realname:clienttype:frequency:latitude:longitude:altitude:groundspeed:planned_aircraft:
      // planned_tascruise:planned_depairport:planned_altitude:planned_destairport:server:protrevision:rating
      // :transponder:facilitytype:visualrange:planned_revision:planned_flighttype:planned_deptime:planned_ac
      // tdeptime:planned_hrsenroute:planned_minenroute:planned_hrsfuel:planned_minfuel:planned_altairport:pl
      // anned_remarks:planned_route:planned_depairport_lat:planned_depairport_lon:planned_destairport_lat:pl
      // anned_destairport_lon:atis_message:time_last_atis_received:time_logon:heading:QNH_iHg:QNH_Mb:

      // AAL1064:1277950:Name
      // KMIA:PILOT::37.85759:-76.72431:35099:471:B738/G:462:KMIA:35000:KLGA:USA-E:100:1:2444:::1:I:1503:0:2:
      // 32:4:20:KPHL:PBN/A1B1C1D1L1O1S1 DOF/180322 REG/N917NN EET/KZJX0044 KZDC0113 KZNY0210 KZBW0231
      // OPR/AAL PER/C RMK/TCAS SIMBRIEF /v/ SEL/AEBX:VALLY2 VALLY DCT PERMT AR16 ILM J191 PXT
      // KORRY4:0:0:0:0:::20180322145209:26:30.017:1016:
      for(atools::fs::sc::SimConnectAircraft ac : aircraftFiltered)
      {
        if(ac.isAnyBoat())
          continue;

        QString reg = ac.getAirplaneRegistration();
        QStringList text;

        QString name;
        if(ac.isUser())
          name = "User " + reg;
        else
          name = "Client " + reg;

        // callsign:cid:realname:clienttype:frequency:latitude:longitude:altitude ...
        // AAL1064:1277950:Name KMIA:PILOT::37.85759:-76.72431:35099 ...
        text << reg << QString::number(ac.getObjectId()) << name << "PILOT" << QString()
             << QString::number(ac.getPosition().getLatY(), 'g', 8) << QString::number(ac.getPosition().getLonX(), 'g', 8)
             << QString::number(atools::roundToInt(ac.getPosition().getAltitude()));

        // ... :groundspeed:planned_aircraft:planned_tascruise:planned_depairport:planned_altitude:planned_destairport ...
        // ... 471:B738/G:462:KMIA:35000:KLGA ...
        text << QString::number(ac.getGroundSpeedKts()) << ac.getAirplaneModel() << QString::number(ac.getGroundSpeedKts())
             << ac.getFromIdent() << QString::number(atools::roundToInt(ac.getPosition().getAltitude())) << ac.getToIdent();

        // ... :server:protrevision:rating:transponder:facilitytype:visualrange:planned_revision:planned_flighttype:
        // planned_deptime:planned_ac tdeptime:planned_hrsenroute:planned_minenroute:planned_hrsfuel:planned_minfuel ...
        // ... USA-E:100:1:2444:::1:I:1503:0:2:32:4:20 ...
        text << "USA-E" << "100" << "1" << ac.getTransponderCodeStr() << QString() << QString() << "1" << "I"
             << "1500" << "0" << "2" << "32" << "4" << "20";

        // ... planned_altairport:planned_remarks:planned_route:planned_depairport_lat:planned_depairport_lon:
        // planned_destairport_lat:pl anned_destairport_lon:atis_message:time_last_atis_received:time_logon:heading:QNH_iHg:QNH_Mb: ...
        // ... KPHL:PBN/A1... F /v/ SEL/AEBX:VALLY2 VALLY DCT PERMT AR16 ILM J191 PXT KORRY4:0:0:0:0 ...
        text << "KPHL" << QString() /* remarks */ << QString() /* route */ << "0" << "0" << "0" << "0";

        // ... :atis_message:time_last_atis_received:time_logon:heading:QNH_iHg:QNH_Mb:
        // ... :::20180322145209:26:30.017:1016:
        text << QString() << QString() << QString() << QString::number(atools::roundToInt(ac.getHeadingDegTrue())) << QString() << "1013";

        for(int i = text.size(); i < 40; i++)
          text.append(QString());

        stream << text.join(':') << endl;
      }
    }

    last = now;
  }
}

DataReaderThread::DataReaderThread(QObject *parent, bool verboseLog)
  : QThread(parent), verbose(verboseLog)
{
  qDebug() << Q_FUNC_INFO;
  setObjectName("DataReaderThread");

  options = atools::fs::sc::FETCH_AI_AIRCRAFT | atools::fs::sc::FETCH_AI_BOAT;
}

DataReaderThread::~DataReaderThread()
{
  qDebug() << Q_FUNC_INFO;
}

void DataReaderThread::setHandler(ConnectHandler *connectHandler)
{
  qDebug() << Q_FUNC_INFO << connectHandler->getName();
  qDebug() << "SimConnect available:" << (handler != nullptr ? handler->isLoaded() : false);

  handler = connectHandler;

}

void DataReaderThread::connectToSimulator()
{
  if(!handler->isLoaded())
  {
    QString msg = tr("No flight simulator installation found. SimConnect not loaded.");
    emit postStatus(atools::fs::sc::OK, msg);
    emit postLogMessage(msg, false, false);
  }
  else
  {
    QString msg = tr("Not connected to the simulator. Waiting ...");
    emit postStatus(atools::fs::sc::OK, msg);
    emit postLogMessage(msg, false, false);

    int rateSec = reconnectRateSec;
    int counterSec = 0;

    reconnecting = true;
    while(!terminate)
    {
      if((counterSec % rateSec) == 0)
      {
        if(handler->connect())
        {
#ifdef DEBUG_INFORMATION
          qDebug() << Q_FUNC_INFO << "Connected" << "counterSec" << counterSec << "rateSec" << rateSec;
#endif

          connected = true;
          emit connectedToSimulator();
          QString connectMsg = tr("Connected to simulator.");
          emit postStatus(atools::fs::sc::OK, connectMsg);
          emit postLogMessage(connectMsg, false, false);
          break;
        }

#ifdef DEBUG_INFORMATION
        qDebug() << Q_FUNC_INFO << "Not connected" << "counterSec" << counterSec << "rateSec" << rateSec;
#endif

      }
      counterSec++;

      // Decrease connection rate for long running sessions to avoid loss of internal SimConnect handles
      // Workaround for #891
      if(isSimConnectHandler())
      {
        if(counterSec == 1800)
        {
          rateSec *= 2;
          qDebug() << Q_FUNC_INFO << "Increasing connection rate after 30 minutes to" << rateSec << "seconds";
        }

        if(counterSec == 3600)
        {
          rateSec *= 2;
          qDebug() << Q_FUNC_INFO << "Increasing connection rate after 1 hour to" << rateSec << "seconds";
        }
      }

      // Sleep a second
      QThread::sleep(1);
    }
  }
  reconnecting = false;
}

void DataReaderThread::run()
{
  qDebug() << Q_FUNC_INFO << "update rate" << updateRate;

  setupReplay();

  // Try to connect first ============================================

  if(loadReplayFile == nullptr)
    // Connect to the simulator
    connectToSimulator();
  else
    // Using replay is always connected
    connected = true;

  qDebug() << "Datareader connected";

  waitMutex.lock();

  // Main loop  ============================================
  while(!terminate)
  {
    atools::fs::sc::SimConnectData data;
    atools::fs::sc::Options opts = options;

    if(loadReplayFile != nullptr)
    {
      // Do replay ============================================
      data.read(loadReplayFile);

      if(data.getStatus() == OK)
      {
        if(loadReplayFile->atEnd())
          loadReplayFile->seek(REPLAY_FILE_DATA_START_OFFSET);

        // Remove boat and ship traffic depending on settings for testing purposes
        QVector<SimConnectAircraft>& aiAircraft = data.getAiAircraft();
        if(!(opts & atools::fs::sc::FETCH_AI_AIRCRAFT))
        {
          QVector<SimConnectAircraft>::iterator it =
            std::remove_if(aiAircraft.begin(), aiAircraft.end(), [](const SimConnectAircraft& aircraft) -> bool
                {
                  return !aircraft.isUser() && !aircraft.isAnyBoat();
                });
          if(it != aiAircraft.end())
            aiAircraft.erase(it, aiAircraft.end());
        }

        if(!(opts & atools::fs::sc::FETCH_AI_BOAT))
        {
          QVector<SimConnectAircraft>::iterator it =
            std::remove_if(aiAircraft.begin(), aiAircraft.end(), [](const SimConnectAircraft& aircraft) -> bool
                {
                  return !aircraft.isUser() && aircraft.isAnyBoat();
                });
          if(it != aiAircraft.end())
            aiAircraft.erase(it, aiAircraft.end());
        }

        if(!replayWhazzupFile.isEmpty())
          debugWriteWhazzup(data);

#ifdef DEBUG_CREATE_WHAZZUP_TEST_FILTER

        data.getAiAircraft().erase(std::remove_if(data.getAiAircraft().begin(), data.getAiAircraft().end(),
                                                  [](const SimConnectAircraft& type) -> bool {
                return type.getAirplaneRegistration() != "N2092Z";
              }), data.getAiAircraft().end());
#endif

        emit postSimConnectData(data);
      }
      else
      {
        emit postStatus(data.getStatus(), data.getStatusText());
        emit postLogMessage(tr("Error reading \"%1\": %2.").
                            arg(loadReplayFilepath).arg(data.getStatusText()), false, true);
        closeReplay();
      }
    } // if(loadReplayFile != nullptr)
    else if(fetchData(data, aiFetchRadiusKm, opts))
    {
      // Data fetched from simconnect - send to client ============================================
      if(verbose && !data.getMetars().isEmpty())
        qDebug() << "DataReaderThread::run() num metars" << data.getMetars().size();

      emit postSimConnectData(data);

      if(saveReplayFile != nullptr && saveReplayFile->isOpen() && data.getPacketId() > 0)
        // Save only simulator packets, not weather replays
        data.write(saveReplayFile);
    }
    else
    {
      if(handler->getState() != atools::fs::sc::STATEOK)
      {
        // Error fetching data from simconnect ============================================
        connected = false;
        emit disconnectedFromSimulator();

        emit postStatus(data.getStatus(), data.getStatusText());

        qWarning() << "Error fetching data from simulator." << data.getStatusText();

        if(numErrors++ > MAX_NUMBER_OF_ERRORS)
        {
          numErrors = 0;
          emit postLogMessage(tr("Too many errors reading from simulator. Disconnected. "
                                 "Restart %1 to try again.").
                              arg(QCoreApplication::applicationName()), false, true);
          break;
        }

        if(!handler->isSimRunning())
          // Try to reconnect if we lost connection to simulator
          connectToSimulator();
      }
      else if(data.getStatus() != OK)
      {
        connected = false;
        emit disconnectedFromSimulator();

        emit postStatus(data.getStatus(), data.getStatusText());

        qWarning() << "Error fetching data from simulator." << data.getStatusText();

        emit postLogMessage(tr("Error reading from simulator: %1. Disconnected. "
                               "Restart <i>%2</i> to try again.").
                            arg(data.getStatusText()).
                            arg(QCoreApplication::applicationName()), false, true);

        if(data.getStatus() == INVALID_MAGIC_NUMBER || data.getStatus() == VERSION_MISMATCH)
        {
          emit postLogMessage(tr("Your installed version of <i>Little Xpconnect</i> "
                                 "is not compatible with this version of <i>%2</i>.").
                              arg(QCoreApplication::applicationName()), false, true);
          emit postLogMessage(tr("Install the latest version of <i>Little Xpconnect</i>."), false, true);
        }

        break;
      }
      // else
      // qWarning() << "No data fetched";
    }

    unsigned long sleepMs = 500;
    if(loadReplayFile != nullptr)
      sleepMs = static_cast<unsigned long>(static_cast<float>(replayUpdateRateMs) /
                                           static_cast<float>(replaySpeed));
    else
      sleepMs = updateRate;

    bool wakeUpSignalled = waitCondition.wait(&waitMutex, sleepMs);
    if(wakeUpSignalled && verbose)
      qDebug() << "DataReaderThread::run wakeUpSignalled";
  }

  closeReplay();

  terminate = false; // Allow restart
  connected = false;
  reconnecting = false;

  qDebug() << "Unlocking wait";
  waitMutex.unlock();

  emit disconnectedFromSimulator();
  qDebug() << Q_FUNC_INFO << "leave";
}

bool DataReaderThread::fetchData(atools::fs::sc::SimConnectData& data, int radiusKm, Options fetchOptions)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO << "enter";

  if(!handler->isLoaded())
    return true;

  QMutexLocker locker(&handlerMutex);

  bool weatherRequested = handler->getWeatherRequest().isValid();

  bool retval = false;

  if(weatherRequested)
  {
    if(verbose)
      qDebug() << "DataReaderThread::fetchData weather";

    handler->fetchWeatherData(data);

    // Weather requests and reply always have packet id 0
    data.setPacketId(0);

    // Force an empty reply to the client - even if no weather was fetched
    retval = true;
  }
  else
  {
    if(verbose)
      qDebug() << "DataReaderThread::fetchData nextPacketId" << nextPacketId;

    retval = handler->fetchData(data, radiusKm, fetchOptions);
    data.setPacketId(nextPacketId++);
  }

  data.setPacketTimestamp(QDateTime::currentDateTimeUtc());

  if(verbose)
    if(weatherRequested && !data.getMetars().isEmpty())
      qDebug() << "Weather requested and found";

  if(weatherRequested && data.getMetars().isEmpty())
    qWarning() << "Weather requested but noting found";

  handler->addWeatherRequest(WeatherRequest());

  if(verbose)
    qDebug() << Q_FUNC_INFO << "leave";

  return retval;
}

void DataReaderThread::setupReplay()
{
  if(!loadReplayFilepath.isEmpty())
  {
    loadReplayFile = new QFile(loadReplayFilepath);

    if(loadReplayFile->size() > static_cast<qint64>(REPLAY_FILE_DATA_START_OFFSET))
    {
      if(!loadReplayFile->open(QIODevice::ReadOnly))
      {
        emit postLogMessage(tr("Cannot open \"%1\".").arg(loadReplayFilepath), false, true);
        delete loadReplayFile;
        loadReplayFile = nullptr;
      }
      else
      {
        QDataStream in(loadReplayFile);
        in.setVersion(QDataStream::Qt_5_5);
        in.setFloatingPointPrecision(QDataStream::SinglePrecision);

        quint32 magicNumber, version;

        in >> magicNumber >> version >> replayUpdateRateMs;
        if(magicNumber != REPLAY_FILE_MAGIC_NUMBER)
        {
          emit postLogMessage(tr("Cannot open \"%1\". Is not a replay file - wrong magic number.").
                              arg(loadReplayFilepath), false, true);
          closeReplay();
          return;
        }
        if(version != REPLAY_FILE_VERSION)
        {
          emit postLogMessage(tr("Cannot open \"%1\". Wrong version.").arg(loadReplayFilepath), false, true);
          closeReplay();
          return;
        }

        emit postLogMessage(tr("Replaying from \"%1\".").arg(loadReplayFilepath), false, false);
        emit connectedToSimulator();
      }
    }
    else
    {
      emit postLogMessage(tr("Cannot open \"%1\". File is too small.").arg(loadReplayFilepath), false, true);
      closeReplay();
      return;
    }
  }
  else if(!saveReplayFilepath.isEmpty())
  {
    saveReplayFile = new QFile(saveReplayFilepath);
    if(!saveReplayFile->open(QIODevice::WriteOnly))
    {
      emit postLogMessage(tr("Cannot open \"%1\".").arg(saveReplayFilepath), false, true);
      delete saveReplayFile;
      saveReplayFile = nullptr;
    }
    else
    {
      emit postLogMessage(tr("Saving replay to \"%1\".").arg(saveReplayFilepath), false, false);

      // Save file header
      QDataStream out(saveReplayFile);
      out << REPLAY_FILE_MAGIC_NUMBER << REPLAY_FILE_VERSION << static_cast<quint32>(updateRate);
    }
  }
}

void DataReaderThread::closeReplay()
{
  if(saveReplayFile != nullptr)
  {
    saveReplayFile->close();
    delete saveReplayFile;
    saveReplayFile = nullptr;
  }

  if(loadReplayFile != nullptr)
  {
    loadReplayFile->close();
    delete loadReplayFile;
    loadReplayFile = nullptr;
  }
}

bool DataReaderThread::isSimconnectAvailable() const
{
  return handler->isLoaded();
}

bool DataReaderThread::canFetchWeather() const
{
  return handler->canFetchWeather();
}

bool DataReaderThread::isSimConnectHandler()
{
  return dynamic_cast<atools::fs::sc::SimConnectHandler *>(handler) != nullptr;
}

bool DataReaderThread::isXplaneHandler()
{
  return dynamic_cast<atools::fs::sc::XpConnectHandler *>(handler) != nullptr;
}

void DataReaderThread::setWeatherRequest(atools::fs::sc::WeatherRequest request)
{
  if(verbose)
    qDebug() << Q_FUNC_INFO;

  if(!canFetchWeather())
  {
    emit postSimConnectData(atools::fs::sc::SimConnectData());
    return;
  }

  if(saveReplayFile != nullptr)
  {
    // Post a dummy weather reply if replaying, do not pass to handler
    emit postSimConnectData(atools::fs::sc::SimConnectData());
    return;
  }

  {
    QMutexLocker locker(&handlerMutex);
    handler->addWeatherRequest(request);
  }

  waitCondition.wakeAll();
}

void DataReaderThread::terminateThread()
{
  setTerminate(true);
  wait();
  setTerminate(false);
}

} // namespace sc
} // namespace fs
} // namespace atools
