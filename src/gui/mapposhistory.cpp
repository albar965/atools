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

#include "gui/mapposhistory.h"

#include <QDebug>
#include <QDateTime>
#include <QDataStream>
#include <QFile>

namespace atools {
namespace gui {

MapPosHistoryEntry::MapPosHistoryEntry()
{

}

MapPosHistoryEntry::MapPosHistoryEntry(const MapPosHistoryEntry& other)
{
  *this = other;
}

MapPosHistoryEntry::MapPosHistoryEntry(atools::geo::Pos position, double mapDistance, qint64 mapTimestamp)
  : timestamp(mapTimestamp), pos(position), distance(mapDistance)
{

}

bool MapPosHistoryEntry::operator==(const MapPosHistoryEntry& other) const
{
  return std::abs(distance - other.distance) < 0.001 &&
         pos == other.pos;
}

bool MapPosHistoryEntry::operator!=(const MapPosHistoryEntry& other) const
{
  return !operator==(other);
}

QDataStream& operator<<(QDataStream& out, const MapPosHistoryEntry& obj)
{
  out << obj.timestamp << obj.pos << obj.distance;
  return out;
}

QDataStream& operator>>(QDataStream& in, MapPosHistoryEntry& obj)
{
  in >> obj.timestamp >> obj.pos >> obj.distance;
  return in;
}

QDebug operator<<(QDebug debug, const MapPosHistoryEntry& entry)
{
  QDebugStateSaver save(debug);
  debug.nospace() << "MapPosHistoryEntry(" << entry.pos << ","
                  << entry.distance << "," << entry.timestamp << ")";
  return debug;
}

// -----------------------------------------------------------------------

MapPosHistory::MapPosHistory(QObject *parent)
  : QObject(parent)
{
}

MapPosHistory::~MapPosHistory()
{
}

const MapPosHistoryEntry& MapPosHistory::next()
{
  if(active)
  {
    if(currentIndex < entries.size() - 1)
    {
      currentIndex++;
      emit historyChanged(0, currentIndex, entries.size() - 1);
      return entries.at(currentIndex);
    }
  }
  return EMPTY_MAP_POS;
}

const MapPosHistoryEntry& MapPosHistory::back()
{
  if(active)
  {
    if(currentIndex > 0)
    {
      currentIndex--;
      emit historyChanged(0, currentIndex, entries.size() - 1);
      return entries.at(currentIndex);
    }
  }
  return EMPTY_MAP_POS;
}

const MapPosHistoryEntry& MapPosHistory::current() const
{
  if(!entries.isEmpty())
    return entries.at(currentIndex);

  return EMPTY_MAP_POS;
}

void MapPosHistory::addEntry(atools::geo::Pos pos, double distance)
{
  if(!active)
    return;

  MapPosHistoryEntry newEntry(pos, distance, QDateTime::currentMSecsSinceEpoch());
  const MapPosHistoryEntry& curEntry = current();

  if(newEntry == curEntry)
    return;

  if(curEntry.getTimestamp() > newEntry.getTimestamp() - MAX_MS_FOR_NEW_ENTRY)
  {
    // Entries are added too close - overwrite the current one
    entries[currentIndex] = newEntry;
  }
  else
  {
    if(currentIndex != -1)
    {
      int size = entries.size();
      // Prune forward history
      for(int i = currentIndex + 1; i < size; i++)
        entries.removeLast();
    }

    entries.append(newEntry);
    currentIndex++;

    while(entries.size() > MAX_NUMBER_OF_ENTRIES)
    {
      // Remove additional entries
      entries.removeFirst();
      currentIndex--;
    }

    emit historyChanged(0, currentIndex, entries.size() - 1);
  }
}

void MapPosHistory::saveState(const QString& filename) const
{
  QFile historyFile(filename);

  if(historyFile.open(QIODevice::WriteOnly))
  {
    QDataStream out(&historyFile);
    out.setVersion(QDataStream::Qt_5_5);

    out << FILE_MAGIC_NUMBER << FILE_VERSION << currentIndex << entries;
    historyFile.close();
  }
  else
    qWarning() << "Cannot write history" << historyFile.fileName() << ":" << historyFile.errorString();
}

void MapPosHistory::restoreState(const QString& filename)
{
  entries.clear();
  currentIndex = -1;

  QFile historyFile(filename);

  if(historyFile.exists())
  {
    if(historyFile.open(QIODevice::ReadOnly))
    {
      quint32 magic;
      quint16 version;
      QDataStream in(&historyFile);
      in.setVersion(QDataStream::Qt_5_5);
      in >> magic;

      if(magic == FILE_MAGIC_NUMBER)
      {
        in >> version;
        if(version == FILE_VERSION)
          in >> currentIndex >> entries;
        else
          qWarning() << "Cannot read history" << historyFile.fileName() << ". Invalid version number:" <<
            version;
      }
      else
        qWarning() << "Cannot read history" << historyFile.fileName() << ". Invalid magic number:" << magic;

      historyFile.close();
    }
    else
      qWarning() << "Cannot read history" << historyFile.fileName() << ":" << historyFile.errorString();
  }

  if(entries.isEmpty())
    emit historyChanged(0, 0, 0);
  else
    emit historyChanged(0, currentIndex, entries.size() - 1);
}

void MapPosHistory::activate()
{
  active = true;
}

void MapPosHistory::clear()
{
  entries.clear();
  currentIndex = -1;
}

void MapPosHistory::registerMetaTypes()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  qRegisterMetaTypeStreamOperators<atools::gui::MapPosHistoryEntry>();
  qRegisterMetaTypeStreamOperators<QList<atools::gui::MapPosHistoryEntry> >();
#endif
}

} // namespace gui
} // namespace atools
