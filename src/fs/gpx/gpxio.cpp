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

#include "fs/gpx/gpxio.h"

#include "exception.h"
#include "geo/pos.h"
#include "geo/calculations.h"
#include "atools.h"
#include "gpxtypes.h"
#include "fs/pln/flightplan.h"
#include "util/xmlstream.h"
#include "zip/gzip.h"

#include <QDateTime>
#include <QFile>
#include <QRegularExpression>
#include <QXmlStreamReader>

using atools::geo::Pos;
using atools::geo::PosD;
using atools::fs::pln::Flightplan;
using atools::fs::pln::FlightplanEntry;

namespace atools {
namespace fs {
namespace gpx {

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
using Qt::endl;
#endif

GpxIO::GpxIO()
{
  errorMsg = tr("Cannot open file %1. Reason: %2");
}

bool GpxIO::isGpxFile(const QString& file)
{
  // Get first 30 non empty lines - always returns a list of 30
  QStringList lines = probeFile(file, 30 /* numLinesRead */);

  if(lines.isEmpty())
    throw Exception(tr("Cannot open empty GPX file \"%1\".").arg(file));

  // Next or same line with "<gpx"
  return lines.constFirst().startsWith("<?xml", Qt::CaseInsensitive) &&
         (lines.at(0).startsWith("<gpx", Qt::CaseInsensitive) ||
          lines.at(1).startsWith("<gpx", Qt::CaseInsensitive) ||
          lines.at(2).startsWith("<gpx", Qt::CaseInsensitive));
}

void GpxIO::readPosGpx(atools::geo::PosD& pos, QString& name, atools::util::XmlStream& xmlStream, QDateTime *timestamp)
{
  bool lonOk, latOk;

  QXmlStreamReader& reader = xmlStream.getReader();
  double lon = reader.attributes().value("lon").toDouble(&lonOk);
  double lat = reader.attributes().value("lat").toDouble(&latOk);

  if(lonOk && latOk)
  {
    pos.setLonX(lon);
    pos.setLatY(lat);

    if(!pos.isValid())
    {
      qWarning() << Q_FUNC_INFO << "Invalid position in GPX. Ordinates out of range" << pos.asPos().toString();
      pos = atools::geo::EMPTY_POSD;
    }
    else if(!pos.isValidRange())
    {
      qWarning() << Q_FUNC_INFO << "Invalid position in GPX. Ordinates out of range" << pos.asPos().toString();
      pos = atools::geo::EMPTY_POSD;
    }
  }
  else
    throw Exception(tr("Invalid position in GPX file \"%1\".").arg(xmlStream.getFilename()));

  while(xmlStream.readNextStartElement())
  {
    if(reader.name() == QLatin1String("name"))
      name = reader.readElementText();
    else if(reader.name() == QLatin1String("time"))
    {
      if(timestamp != nullptr)
        // Reads with or without milliseconds and returns UTC without changed hour number
        *timestamp = QDateTime::fromString(reader.readElementText(), Qt::ISODate);
    }
    else if(reader.name() == QLatin1String("ele")) // Elevation
      pos.setAltitude(atools::geo::meterToFeet(reader.readElementText().toDouble()));
    else
      xmlStream.skipCurrentElement(false /* warn */);
  }
}

QString GpxIO::saveGpxStr(const atools::fs::gpx::GpxData& gpxData)
{
  QString gpxString;
  QXmlStreamWriter writer(&gpxString);
  saveGpxInternal(writer, gpxData);
  return gpxString;
}

QByteArray GpxIO::saveGpxGz(const atools::fs::gpx::GpxData& gpxData)
{
  QByteArray retval;
  atools::zip::gzipCompress(saveGpxStr(gpxData).toUtf8(), retval);
  return retval;
}

void GpxIO::saveGpx(const QString& filename, const atools::fs::gpx::GpxData& gpxData)
{
  QFile gpxFile(filename);
  if(gpxFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QXmlStreamWriter writer(&gpxFile);
    saveGpxInternal(writer, gpxData);
    gpxFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(gpxFile.errorString()));
}

void GpxIO::saveGpxInternal(QXmlStreamWriter& writer, const atools::fs::gpx::GpxData& gpxData)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  writer.setCodec("UTF-8");
#endif
  writer.setAutoFormatting(true);
  writer.setAutoFormattingIndent(2);

  // <?xml version="1.0" encoding="UTF-8"?>
  writer.writeStartDocument("1.0");

  // <gpx
  // xmlns="http://www.topografix.com/GPX/1/1"
  // version="1.1"
  // creator="Program"
  // xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  // xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd">

  writer.writeStartElement("gpx");

  writer.writeDefaultNamespace("http://www.topografix.com/GPX/1/1");
  writer.writeAttribute("version", "1.1");
  writer.writeAttribute("creator", "Little Navmap");
  writer.writeNamespace("http://www.w3.org/2001/XMLSchema-instance", "xsi");
  writer.writeAttribute("xsi:schemaLocation", "http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd");

  // writer.writeComment(programFileInfo());

  // <metadata>
  // <link href="http://www.garmin.com">
  // <text>Garmin International</text>
  // </link>
  // <time>2009-10-17T22:58:43Z</time>
  // </metadata>
  writer.writeStartElement("metadata");
  writer.writeStartElement("link");
  writer.writeAttribute("href", "https://www.littlenavmap.org");
  writer.writeTextElement("text", atools::programFileInfo());
  writer.writeEndElement(); // link
  writer.writeEndElement(); // metadata

  const Flightplan& flightplan = gpxData.getFlightplan();
  if(!flightplan.isEmpty())
  {
    writer.writeStartElement("rte");
    writer.writeTextElement("name", flightplan.getTitle() + tr(" - Flight Plan"));
    writer.writeTextElement("desc", flightplan.getDescription());

    // Write route ========================================================
    for(int i = 0; i < flightplan.size(); i++)
    {
      const FlightplanEntry& entry = flightplan.at(i);
      // <rtept lat="52.0" lon="13.5">
      // <ele>33.0</ele>
      // <time>2011-12-13T23:59:59Z</time>
      // <name>rtept 1</name>
      // </rtept>

      if(!entry.getPosition().isValidRange())
      {
        qWarning() << Q_FUNC_INFO << "Invalid position" << entry.getPosition();
        continue;
      }

      if(i > 0)
      {
        // Remove duplicates with same name and almost same position
        const FlightplanEntry& prev = flightplan.at(i - 1);
        if(entry.getIdent() == prev.getIdent() && entry.getRegion() == prev.getRegion() &&
           entry.getPosition().almostEqual(prev.getPosition(), Pos::POS_EPSILON_100M))
          continue;
      }

      // Write route point ===========
      writer.writeStartElement("rtept");
      writer.writeAttribute("lon", QString::number(entry.getPosition().getLonX(), 'f', 7));
      writer.writeAttribute("lat", QString::number(entry.getPosition().getLatY(), 'f', 7));
      writer.writeTextElement("ele", QString::number(atools::geo::feetToMeter(entry.getAltitude())));

      writer.writeTextElement("name", entry.getIdent());
      writer.writeTextElement("desc", entry.getWaypointTypeAsFsxString());

      writer.writeEndElement(); // rtept
    }

    writer.writeEndElement(); // rte
  }

  // Write track ========================================================
  if(gpxData.hasTrails())
  {
    writer.writeStartElement("trk");

    if(!flightplan.isEmpty())
      writer.writeTextElement("name", QCoreApplication::applicationName() + tr(" - Track"));

    for(const TrailPoints& track : gpxData.getTrails())
    {
      if(track.isEmpty())
        continue;

      writer.writeStartElement("trkseg");

      for(const TrailPoint& pos : track)
      {
        writer.writeStartElement("trkpt");

        writer.writeAttribute("lon", QString::number(pos.pos.getLonX(), 'f', 6));
        writer.writeAttribute("lat", QString::number(pos.pos.getLatY(), 'f', 6));
        writer.writeTextElement("ele", QString::number(atools::geo::feetToMeter(pos.pos.getAltitude())));

        if(pos.timestampMs > 0)
        {
          // (UTC/Zulu) in ISO 8601 format: "yyyy-mm-ddThh:mm:ssZ" or "yyyy-MM-ddTHH:mm:ss.zzzZ"
          // <time>2011-01-16T23:59:01Z</time>
          // Changes time number to local if Qt::UTC is omitted
          writer.writeTextElement("time", QDateTime::fromMSecsSinceEpoch(pos.timestampMs, Qt::UTC).toString(Qt::ISODateWithMs));
        }

        writer.writeEndElement(); // trkpt
      }
      writer.writeEndElement(); // trkseg
    }
    writer.writeEndElement(); // trk
  }

  writer.writeEndElement(); // gpx
  writer.writeEndDocument();
}

void GpxIO::loadGpxStr(atools::fs::gpx::GpxData& gpxData, const QString& string)
{
  if(!string.isEmpty())
  {
    atools::util::XmlStream xmlStream(string);
    loadGpxInternal(gpxData, xmlStream);
  }
}

void GpxIO::loadGpxGz(atools::fs::gpx::GpxData& gpxData, const QByteArray& bytes)
{
  if(!bytes.isEmpty())
    loadGpxStr(gpxData, atools::zip::gzipDecompress(bytes));
}

void GpxIO::loadGpx(atools::fs::gpx::GpxData& gpxData, const QString& filename)
{
  QFile gpxFile(filename);
  if(gpxFile.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    atools::util::XmlStream xmlStream(&gpxFile);
    loadGpxInternal(gpxData, xmlStream);
    gpxFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(gpxFile.errorString()));
}

void GpxIO::loadGpxInternal(atools::fs::gpx::GpxData& gpxData, atools::util::XmlStream& xmlStream)
{
  QXmlStreamReader& reader = xmlStream.getReader();
  xmlStream.readUntilElement("gpx");
  PosD pos;
  QString name;
  gpxData.clear();

  while(xmlStream.readNextStartElement())
  {
    // Read route elements ======================================================
    if(reader.name() == QLatin1String("rte"))
    {
      while(xmlStream.readNextStartElement())
      {
        if(reader.name() == QLatin1String("rtept"))
        {
          readPosGpx(pos, name, xmlStream);
          if(pos.isValidRange())
          {
            atools::fs::pln::FlightplanEntry entry;
            entry.setIdent(name);
            entry.setPosition(pos.asPos());
            gpxData.appendFlightplanEntry(entry);
          }
        }
        else
          xmlStream.skipCurrentElement(false /* warn */);
      }
    }
    // Read track elements if needed ======================================================
    else if(reader.name() == QLatin1String("trk"))
    {
      TrailPoints line;
      while(xmlStream.readNextStartElement())
      {
        if(reader.name() == QLatin1String("trkseg"))
        {
          line.clear();
          while(xmlStream.readNextStartElement())
          {
            if(reader.name() == QLatin1String("trkpt"))
            {
              QDateTime datetime;
              readPosGpx(pos, name, xmlStream, &datetime);
              if(pos.isValidRange())
                line.append(atools::fs::gpx::TrailPoint(pos, datetime.toMSecsSinceEpoch()));
            }
            else
              xmlStream.skipCurrentElement(false /* warn */);
          }

          gpxData.appendTrailPoints(line);
        }
        else
          xmlStream.skipCurrentElement(false /* warn */);
      }
    }
    else
      xmlStream.skipCurrentElement(false /* warn */);
  }

  gpxData.adjustDepartureAndDestinationFlightplan();
}

} // namespace gpx
} // namespace fs
} // namespace atools
