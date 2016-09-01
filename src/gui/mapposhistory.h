/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_GUI_MAPPOSHISTORY_H
#define ATOOLS_GUI_MAPPOSHISTORY_H

#include "geo/pos.h"

#include <QObject>
#include <QApplication>

namespace atools {
namespace gui {

/*
 * A entry for the map position history. Can be saved to a QDataStream.
 */
class MapPosHistoryEntry
{
public:
  MapPosHistoryEntry();
  MapPosHistoryEntry(const MapPosHistoryEntry& other);
  MapPosHistoryEntry(atools::geo::Pos position, double mapDistance, qint64 mapTimestamp = 0L);
  ~MapPosHistoryEntry();

  /* Does not compare the timestamp */
  bool operator==(const MapPosHistoryEntry& other) const;
  bool operator!=(const MapPosHistoryEntry& other) const;

  /*
   * @return zoom distance
   */
  double getDistance() const
  {
    return distance;
  }

  /*
   * @return timestamp when this entry was created - milliseconds since epoch
   */
  qint64 getTimestamp() const
  {
    return timestamp;
  }

  const atools::geo::Pos& getPos() const
  {
    return pos;
  }

  /* If false the entry is invalid. This can happen when accessing an empty history stack */
  bool isValid() const
  {
    return pos.isValid();
  }

private:
  friend QDebug operator<<(QDebug debug, const MapPosHistoryEntry& entry);

  friend QDataStream& operator<<(QDataStream& out, const MapPosHistoryEntry& obj);

  friend QDataStream& operator>>(QDataStream& in, MapPosHistoryEntry& obj);

  qint64 timestamp = 0L;
  atools::geo::Pos pos;
  double distance = 0.;
};

const MapPosHistoryEntry EMPTY_MAP_POS;

/*
 * Maintains a history list of position/zoom distance combinations.
 *
 * To use this class register the stream operators:
 *  qRegisterMetaTypeStreamOperators<atools::geo::Pos>();
 *  qRegisterMetaTypeStreamOperators<atools::gui::MapPosHistoryEntry>();
 *  qRegisterMetaTypeStreamOperators<QList<atools::gui::MapPosHistoryEntry> >();
 *
 */
class MapPosHistory :
  public QObject
{
  Q_OBJECT

public:
  explicit MapPosHistory(QObject *parent = 0);
  virtual ~MapPosHistory();

  /* Get next entry in the history and emit signal historyChanged */
  const MapPosHistoryEntry& next();

  /* Get last entry in the history and emit signal historyChanged */
  const MapPosHistoryEntry& back();

  /* Get current entry in the history */
  const MapPosHistoryEntry& current() const;

  /* add an entry to the history potentially pruning the history for forward actions.
   * Will also emit signal historyChanged  */
  void addEntry(atools::geo::Pos pos, double distance);

  /* Save history to file */
  void saveState(const QString& filename);

  /* load history from file */
  void restoreState(const QString& filename);

signals:
  /* Emitted when the history changes
   * @param minIndex Lowest entry index in the history stack
   * @param curIndex Current entry in the history stack
   * @param maxIndex Maximum entry index in the history stack
   */
  void historyChanged(int minIndex, int curIndex, int maxIndex);

private:
  // Aggregate all entry that are close than this value
  const int MAX_MS_FOR_NEW_ENTRY = 200;
  const int MAX_NUMBER_OF_ENTRIES = 50;

  static Q_DECL_CONSTEXPR quint32 FILE_MAGIC_NUMBER = 0x4C8D1F09;
  static Q_DECL_CONSTEXPR quint16 FILE_VERSION = 1;

  QList<MapPosHistoryEntry> entries;
  qint32 currentIndex = -1;

};

} // namespace gui
} // namespace atools

Q_DECLARE_METATYPE(atools::gui::MapPosHistoryEntry);
Q_DECLARE_TYPEINFO(atools::gui::MapPosHistoryEntry, Q_PRIMITIVE_TYPE);

#endif // ATOOLS_GUI_MAPPOSHISTORY_H
