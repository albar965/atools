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

#include "track/trackreader.h"
#include "exception.h"
#include "atools.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>
#include <QTimeZone>

namespace atools {
namespace track {

const static std::initializer_list<char> INVALID_CHARS = {'/', '-', ';', ':', '<', '>', '=', '(', ')'};

TrackReader::TrackReader()
{

}

TrackReader::~TrackReader()
{

}

void TrackReader::readTracks(const QString& filename, TrackType type)
{
  QFile file(filename);
  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    QTextStream stream(&file);
    readTracks(stream, type);
    file.close();
  }
  else
    throw atools::Exception(tr("Cannot open track file \"%1\". Reason: %2").arg(file.fileName()).arg(file.errorString()));
}

void TrackReader::readTracks(const QByteArray& data, TrackType type)
{
  QTextStream stream(data);
  readTracks(stream, type);
}

void TrackReader::readTracks(QTextStream& stream, TrackType type)
{
  QStringList lines = readLines(stream);

  switch(type)
  {
    case atools::track::UNKNOWN:
      qWarning() << Q_FUNC_INFO << "Track type" << static_cast<int>(type) << "not valid";
      break;

    case atools::track::NATS:
      // Read NAT - <PRE> is not reliable since the HTML is not valid
      extractNatTracks(extractSections(lines, "(NAT-", ")"));
      break;

    case atools::track::PACOTS:
      {
        // Get all lines of <PRE> elements.
        QStringList sections = extractSections(lines, "<PRE>", "</PRE>");
        // Get tracks
        extractPacotTracks(extractSections(sections, "(TDM TRK", ")"));
        // Get flex tracks
        extractPacotTracksFlex(sections);
      }
      break;

    case atools::track::AUSOTS:
      extractAusotTracks(lines);
      break;
  }
}

int TrackReader::removeInvalid()
{
  return TrackReader::removeInvalid(tracks);
}

int TrackReader::removeInvalid(TrackVectorType& trackVector)
{
  auto it = std::remove_if(trackVector.begin(), trackVector.end(), [](const Track& track) -> bool
      {
        bool valid = track.isFullyValid();
        if(!valid)
          qWarning() << "TrackReader: Invalid track " << track;
        return !valid;
      });

  int num = static_cast<int>(std::distance(it, trackVector.end()));
  if(it != trackVector.end())
    trackVector.erase(it, trackVector.end());
  return num;
}

void TrackReader::extractPacotTracksFlex(const QStringList& lines)
{
  // TRACK 1.
  static const QRegularExpression PACOTS_TRACK_REGEXP("^TRACK (\\d+).$");

  // ... 07 MAR 07:00 2020 UNTIL 07 MAR 21:00 2020. CREATED: 06 MAR 18:47 2020 ...
  static const QRegularExpression PACOTS_DATE_REGEXP("(\\d\\d) ([A-Z]+) (\\d\\d):(\\d\\d) (\\d\\d\\d\\d) UNTIL "
                                                     "(\\d\\d) ([A-Z]+) (\\d\\d):(\\d\\d) (\\d\\d\\d\\d)");

  // More than one track for each element. Validity date at end.
  // <PRE><b>Q0328/20</b> - EASTBOUND PACOTS TRACKS BETWEEN JAPAN AND NORTH AMERICA,
  // TRACK 1.
  // FLEX ROUTE : KALNA G344 CRYPT A342 CDB 55N160W 54N150W 52N140W ORNAI
  // JAPAN ROUTE : ADNAP OTR5 KALNA
  // NAR ROUTE : ACFT LDG KSEA--ORNAI SIMLU KEPKO TOU MARNR KSEA
  // ACFT LDG KPDX--ORNAI SIMLU KEPKO TOU KEIKO KPDX
  // ACFT LDG CYVR--ORNAI SIMLU KEPKO YAZ CYVR
  // RMK : ACFT LDG OTHER DEST--ORNAI SIMLU KEPKO UPR TO DEST
  // TRACK 2.
  // FLEX ROUTE : EMRON 41N160E 45N170E 48N180E 50N170W 49N160W 46N150W
  // 43N140W 41N130W TRYSH
  // JAPAN ROUTE : ADNAP OTR7 EMRON
  // NAR ROUTE : ACFT LDG KSFO--TRYSH AMAKR BGGLO KSFO
  // ACFT LDG KLAX--TRYSH ENI OAK BURGL IRNMN KLAX
  // TRACK 3.
  // FLEX ROUTE : LEPKI 40N160E 44N170E 47N180E 49N170W 48N160W 45N150W
  // 42N140W 39N130W DACEM
  // JAPAN ROUTE : AVBET OTR11 LEPKI
  // NAR ROUTE : ACFT LDG KLAX--DACEM PAINT PIRAT BURGL IRNMN KLAX
  // ACFT LDG KSFO--DACEM PAINT PIRAT OSI KSFO
  // RMK : ATM CENTER TEL:81-92-608-8870. 07 MAR 07:00 2020 UNTIL 07 MAR
  // 21:00 2020. CREATED: 06 MAR 18:43 2020
  // </PRE>

  atools::track::TrackVectorType temp;
  bool inRecord = false, inRemark = false;
  QString remark;
  QDateTime from, to;
  for(const QString& line : lines)
  {
    // Reading remark =======================================
    if(inRemark)
    {
      // End of remark - parse date =======================================
      if(line.contains("</PRE>", Qt::CaseInsensitive) || PACOTS_TRACK_REGEXP.match(line).hasMatch())
      {
        // ____________________________________ 1  2   3  4  5          6  7   8  9  10
        // RMK : ATM CENTER TEL:81-92-608-8870. 07 MAR 07:00 2020 UNTIL 07 MAR 21:00 2020. CREATED: 06 MAR 18:43 2020
        QRegularExpressionMatch match = PACOTS_DATE_REGEXP.match(remark);
        if(match.hasMatch())
        {
          from =
            QDateTime(QDate(match.captured(5).toInt(), monthFromStr(match.captured(2)), match.captured(1).toInt()),
                      QTime(match.captured(3).toInt(), match.captured(4).toInt()), Qt::UTC);
          to =
            QDateTime(QDate(match.captured(10).toInt(), monthFromStr(match.captured(7)), match.captured(6).toInt()),
                      QTime(match.captured(8).toInt(), match.captured(9).toInt()), Qt::UTC);

        }
        inRemark = false;
      }
      else
        // Accumulate remark text
        remark += " " + line;
    }

    // End of section - one track per PRE element. =======================================
    if(line.contains("</PRE>", Qt::CaseInsensitive))
    {
      // Add validity to all previously collected tracks
      for(Track& t : temp)
      {
        t.validFrom = from;
        t.validTo = to;
      }

      // Append list
      tracks.append(temp);

      temp.clear();
      remark.clear();
    }

    // Beginning of track - "TRACK 1." - more than one per PRE element =======================================
    QRegularExpressionMatch match = PACOTS_TRACK_REGEXP.match(line);
    if(match.hasMatch())
    {
      inRecord = true;
      Track track;
      track.name = match.captured(1);
      track.type = PACOTS;

      temp.append(track);
    }
    else if(inRecord)
    {
      if(line.startsWith("FLEX ROUTE :"))
        // Start of flex path =======================================
        temp.last().route.append(line.split(" ").mid(3));
      else
      {
        if(atools::strContains(line, INVALID_CHARS))
          // Any invalid characters for a route terminate
          inRecord = false;
        else
          // Append more of the path
          temp.last().route.append(line.split(" "));
      }
    }
    else if(line.startsWith("RMK :"))
    {
      // Start of remark section =======================================
      inRemark = true;
      remark = line;
    }
  }
}

void TrackReader::extractPacotTracks(const QStringList& lines)
{
  static const QRegularExpression PACOT_NAME_REGEXP("^\\(TDM TRK (\\S+)");
  extractTracks(lines, PACOT_NAME_REGEXP, PACOTS, false /* removeEmpty */);
}

void TrackReader::extractAusotTracks(const QStringList& lines)
{
  static const QRegularExpression AUSOT_NAME_REGEXP("^TDM TRK (\\S+)");
  extractTracks(lines, AUSOT_NAME_REGEXP, AUSOTS, true /* removeEmpty */);
}

void TrackReader::extractTracks(const QStringList& lines, const QRegularExpression& nameRegexp, TrackType type,
                                bool removeEmpty)
{
  // 2003070500 2003072100
  static const QRegularExpression DATE_REGEXP("(\\d\\d)(\\d\\d)(\\d\\d)(\\d\\d)(\\d\\d) "
                                              "(\\d\\d)(\\d\\d)(\\d\\d)(\\d\\d)(\\d\\d)");

  // One track for each element. Validity date in second line.
  // (TDM TRK K 200307050001
  // 2003070500 2003072100
  // AUDIA 36N130W 36N140W 36N150W 36N160W 37N170W 38N180E 39N170E
  // 40N160E EMRON
  // RTS/KLAX AUDIA
  // EMRON OTR9 AVBET
  // RMK/NO TRK ADVISORY FOR TRK K TONIGHT
  // ALTITUDE MAY BE RESTRICTED WHILE CROSSING ATS ROUTES
  // )

  int numAfterStart = 0;
  bool inRecord = false;
  atools::track::TrackVectorType temp;
  for(const QString& line : lines)
  {
    // Match and extract name =======================================
    QRegularExpressionMatch matchName = nameRegexp.match(line);
    if(matchName.hasMatch())
    {
      // (TDM TRK K 200307050001
      numAfterStart = 0;
      inRecord = true;
      Track track;
      track.name = matchName.captured(1);
      track.type = type;

      temp.append(track);
    }
    else if(inRecord)
    {
      // Second line - extract validity =======================================
      if(numAfterStart == 1)
      {
        // 2003070500 2003072100
        QRegularExpressionMatch matchDate = DATE_REGEXP.match(line);
        if(matchDate.hasMatch())
        {
          temp.last().validFrom = QDateTime(QDate(matchDate.captured(1).toInt() + 2000,
                                                  matchDate.captured(2).toInt(),
                                                  matchDate.captured(3).toInt()),
                                            QTime(matchDate.captured(4).toInt(),
                                                  matchDate.captured(5).toInt()), Qt::UTC);

          temp.last().validTo = QDateTime(QDate(matchDate.captured(6).toInt() + 2000,
                                                matchDate.captured(7).toInt(),
                                                matchDate.captured(8).toInt()),
                                          QTime(matchDate.captured(9).toInt(),
                                                matchDate.captured(10).toInt()), Qt::UTC);
        }
      }
      else if(numAfterStart >= 2)
      {
        // At and after third line - get route =======================================
        // AUDIA 36N130W 36N140W 36N150W 36N160W 37N170W 38N180E 39N170E
        // 40N160E EMRON
        // RTS/KLAX AUDIA

        if(atools::strContains(line, INVALID_CHARS))
          // Any invalid character terminates route
          inRecord = false;
        else
          // Add route elements
          temp.last().route.append(line.split(" "));
      }
    }
    numAfterStart++;
  }

  // Remove elements without waypoints =======================================
  if(removeEmpty)
  {
    atools::track::TrackVectorType::iterator it = std::remove_if(temp.begin(), temp.end(), [](const Track& t) -> bool
        {
          return t.route.isEmpty();
        });

    if(it != temp.end())
      temp.erase(it, temp.end());
  }

  tracks.append(temp);
}

void TrackReader::extractNatTracks(const QStringList& lines)
{
  // MAR 08/0100Z TO MAR 08/0800Z
  static const QRegularExpression NATS_DATE_REGEXP("^([A-Z]+) (\\d+)/(\\d\\d)(\\d\\d)Z TO "
                                                   "([A-Z]+) (\\d+)/(\\d\\d)(\\d\\d)Z");

  // More than one track for each element. Validity date at the beginning.
  // <pre>
  // <font color="#000099">
  // 061932 EGGXZOZX
  // (NAT-1/3 TRACKS FLS 310/390 INCLUSIVE
  // MAR 07/1130Z TO MAR 07/1900Z</font>
  // PART ONE OF THREE PARTS-
  // A SUNOT 58/20 59/30 59/40 58/50 DORYY
  // EAST LVLS NIL
  // WEST LVLS 310 320 330 340 350 360 370 380 390
  // EUR RTS WEST NIL
  // NAR NIL-
  // B PIKIL 57/20 58/30 58/40 57/50 HOIST
  // EAST LVLS NIL
  // WEST LVLS 310 320 330 340 350 360 370 380 390
  // EUR RTS WEST NIL
  // NAR -
  // END OF PART ONE OF THREE PARTS)

  QDateTime from, to;
  int year = QDateTime::currentDateTimeUtc().date().year();

  atools::track::TrackVectorType temp;
  for(const QString& line : lines)
  {
    // Get name and list of waypoints ============================================
    // C ETARI 5630/20 5730/30 5730/40 5630/50 IRLOK
    if(atools::charAt(line, 0).isUpper() && atools::latin1CharAt(line, 1) == ' ')
    {
      Track track;
      QStringList split = line.split(" ");
      track.name = split.takeFirst();

      // Convert coordinates to NAT waypoints
      track.route = toNatsWaypoints(split);

      track.type = NATS;
      track.validFrom = from;
      track.validTo = to;
      temp.append(track);
    }
    // Get levels ============================================
    else if(line.startsWith("EAST LVLS"))
    {
      // EAST LVLS NIL
      for(const QString& level : line.split(" ").mid(2))
      {
        if(level != "NIL")
          temp.last().eastLevels.append(level.toShort());
      }
    }
    else if(line.startsWith("WEST LVLS"))
    {
      // WEST LVLS 310 320 330 340 350 360 370 380 390
      for(const QString& level : line.split(" ").mid(2))
      {
        if(level != "NIL")
          temp.last().westLevels.append(level.toShort());
      }
    }
    // Read validity ============================================
    else
    {
      // MAR 08/0100Z TO MAR 08/0800Z
      QRegularExpressionMatch match = NATS_DATE_REGEXP.match(line);
      if(match.hasMatch())
      {
        QDateTime f = QDateTime(QDate(year, monthFromStr(match.captured(1)), match.captured(2).toInt()),
                                QTime(match.captured(3).toInt(), match.captured(4).toInt()), Qt::UTC);
        if(f.isValid())
          from = f;

        QDateTime t = QDateTime(QDate(year, monthFromStr(match.captured(5)), match.captured(6).toInt()),
                                QTime(match.captured(7).toInt(), match.captured(8).toInt()), Qt::UTC);
        if(t.isValid())
          to = t;
      }
    }
  }

  // Update direction based on levels ==========================
  for(Track& track : temp)
  {
    if(!track.westLevels.isEmpty() && !track.eastLevels.isEmpty())
      track.direction = BOTH;
    else if(!track.westLevels.isEmpty())
      track.direction = WEST;
    else if(!track.eastLevels.isEmpty())
      track.direction = EAST;
  }

  tracks.append(temp);
}

int TrackReader::monthFromStr(const QString& str)
{
  // Also allows full months
  if(str.startsWith("JAN"))
    return 1;
  else if(str.startsWith("FEB"))
    return 2;
  else if(str.startsWith("MAR"))
    return 3;
  else if(str.startsWith("APR"))
    return 4;
  else if(str.startsWith("MAY"))
    return 5;
  else if(str.startsWith("JUN"))
    return 6;
  else if(str.startsWith("JUL"))
    return 7;
  else if(str.startsWith("AUG"))
    return 8;
  else if(str.startsWith("SEP"))
    return 9;
  else if(str.startsWith("OCT"))
    return 10;
  else if(str.startsWith("NOV"))
    return 11;
  else if(str.startsWith("DEC"))
    return 12;
  else
    return -1;
}

QStringList TrackReader::extractSections(const QStringList& lines, const QString& begin, const QString& end)
{
  // Return lines between begin and end including begin and end
  QStringList sections;
  bool inSection = false, firstFound = false;

  for(const QString& line : lines)
  {
    int beginIndex = line.indexOf(begin, Qt::CaseInsensitive), endIndex = line.indexOf(end, Qt::CaseInsensitive);

    if(inSection)
    {
      if(beginIndex == -1)
      {
        if(endIndex >= 0)
        {
          sections.append(line.left(endIndex + end.size()));
          inSection = false;
        }
        else
          sections.append(line);
      }
      else
        throw new atools::Exception(tr("Error in file on line %1. Found begin marker inside of section."));
    }
    else
    {
      if(endIndex == -1)
      {
        if(beginIndex >= 0)
        {
          sections.append(line.mid(beginIndex));
          inSection = true;
          firstFound = true;
        }
      }
      else if(firstFound)
        throw new atools::Exception(tr("Error in file on line %1. Found end marker outside of section."));
    }
  }
  sections.removeAll("");
  return sections;
}

QStringList TrackReader::readLines(QTextStream& stream)
{
  QStringList sections;
  while(!stream.atEnd())
  {
    QString line = stream.readLine().simplified();
    if(!line.isEmpty())
      sections.append(line);
  }
  return sections;
}

QStringList TrackReader::toNatsWaypoints(const QStringList& str)
{
  // "58/20" to "5820N"
  static const QRegularExpression DEG("^(\\d\\d)/(\\d\\d)$");

  // "5530/20" to "H5530".
  static const QRegularExpression DEGH("^(\\d\\d)30/(\\d\\d)$");

  QStringList path(str);

  for(int i = 0; i < path.size(); i++)
  {
    QString& wp = path[i];
    QRegularExpressionMatch match = DEG.match(wp);
    if(match.hasMatch())
    {
      // Whole degree waypoint ========
      wp = QString("%1%2N").
           arg(match.captured(1).toInt(), 2, 10, QChar('0')).
           arg(match.captured(2).toInt(), 2, 10, QChar('0'));
    }
    else
    {
      match = DEGH.match(wp);
      if(match.hasMatch())
      {
        // Half degree waypoint ========
        wp = QString("H%1%2").
             arg(match.captured(1).toInt(), 2, 10, QChar('0')).
             arg(match.captured(2).toInt(), 2, 10, QChar('0'));
      }
    }
  }
  return path;
}

} // namespace track
} // namespace atools
