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

#include "fs/gpx/gpxio.h"

#include "exception.h"
#include "geo/pos.h"
#include "geo/calculations.h"
#include "atools.h"
#include "gpxtypes.h"
#include "fs/pln/flightplan.h"
#include "util/xmlstreamreader.h"
#include "zip/gzip.h"

#include <QDateTime>
#include <QFile>
#include <QRegularExpression>
#include <QTimeZone>
#include <QXmlStreamReader>

using atools::geo::Pos;
using atools::geo::PosD;
using atools::fs::pln::Flightplan;
using atools::fs::pln::FlightplanEntry;

namespace atools {
namespace fs {
namespace gpx {

GpxIO::GpxIO()
{
  errorMsg = tr("Cannot open file %1. Reason: %2");
}

bool GpxIO::isGpxFile(const QString& file)
{
  // Get first 30 non empty lines - always returns a list of 30
  const QStringList probe = atools::probeFile(file, 30 /* numLinesRead */);

  if(probe.isEmpty())
    throw Exception(tr("Cannot open empty GPX file \"%1\".").arg(file));

  // Find gpx in the whole string in case it is not formatted
  return probe.constFirst().startsWith(QStringLiteral("<?xml"), Qt::CaseInsensitive) &&
         !probe.filter(QStringLiteral("<gpx")).isEmpty();
}

void GpxIO::readPosGpx(atools::geo::PosD& pos, QString& name, atools::util::XmlStreamReader& xmlStream, QDateTime *timestamp)
{
  double lon = xmlStream.readAttributeDouble(QStringLiteral("lon"), geo::INVALID_DOUBLE);
  double lat = xmlStream.readAttributeDouble(QStringLiteral("lat"), geo::INVALID_DOUBLE);

  if(lon < geo::INVALID_DOUBLE && lat < geo::INVALID_DOUBLE)
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
    if(xmlStream.name() == QLatin1String("name"))
      name = xmlStream.readElementTextStr();
    else if(xmlStream.name() == QLatin1String("time"))
    {
      if(timestamp != nullptr)
        // Reads with or without milliseconds and returns UTC without changed hour number
        *timestamp = QDateTime::fromString(xmlStream.readElementTextStr(), Qt::ISODate);
    }
    else if(xmlStream.name() == QLatin1String("ele")) // Elevation
      pos.setAltitude(atools::geo::meterToFeet(xmlStream.readElementTextStr().toDouble()));
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
  writer.setAutoFormatting(true);
  writer.setAutoFormattingIndent(2);

  // <?xml version="1.0" encoding="UTF-8"?>
  writer.writeStartDocument(QStringLiteral("1.0"));

  // <gpx
  // xmlns="http://www.topografix.com/GPX/1/1"
  // version="1.1"
  // creator="Program"
  // xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  // xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd">

  writer.writeStartElement(QStringLiteral("gpx"));

  writer.writeDefaultNamespace(QStringLiteral("http://www.topografix.com/GPX/1/1"));
  writer.writeAttribute(QStringLiteral("version"), QStringLiteral("1.1"));
  writer.writeAttribute(QStringLiteral("creator"), QStringLiteral("Little Navmap"));
  writer.writeNamespace(QStringLiteral("http://www.w3.org/2001/XMLSchema-instance"), QStringLiteral("xsi"));
  writer.writeAttribute(QStringLiteral("xsi:schemaLocation"),
                        QStringLiteral("http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd"));

  // writer.writeComment(programFileInfo());

  // <metadata>
  // <link href="http://www.garmin.com">
  // <text>Garmin International</text>
  // </link>
  // <time>2009-10-17T22:58:43Z</time>
  // </metadata>
  writer.writeStartElement(QStringLiteral("metadata"));
  writer.writeStartElement(QStringLiteral("link"));
  writer.writeAttribute(QStringLiteral("href"), QStringLiteral("https://www.littlenavmap.org"));
  writer.writeTextElement(QStringLiteral("text"), atools::programFileInfo());
  writer.writeEndElement(); // link
  writer.writeEndElement(); // metadata

  const Flightplan& flightplan = gpxData.getFlightplan();
  if(!flightplan.isEmpty())
  {
    writer.writeStartElement(QStringLiteral("rte"));
    writer.writeTextElement(QStringLiteral("name"), flightplan.getTitle() + tr(" - Flight Plan"));
    writer.writeTextElement(QStringLiteral("desc"), flightplan.getDescription());

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
      writer.writeStartElement(QStringLiteral("rtept"));
      writer.writeAttribute(QStringLiteral("lon"), QString::number(entry.getPosition().getLonX(), 'f', 7));
      writer.writeAttribute(QStringLiteral("lat"), QString::number(entry.getPosition().getLatY(), 'f', 7));
      writer.writeTextElement(QStringLiteral("ele"), QString::number(atools::geo::feetToMeter(entry.getAltitude())));

      writer.writeTextElement(QStringLiteral("name"), entry.getIdent());
      writer.writeTextElement(QStringLiteral("desc"), entry.getWaypointTypeAsFsxString());

      writer.writeEndElement(); // rtept
    }

    writer.writeEndElement(); // rte
  }

  // Write track ========================================================
  if(gpxData.hasTrails())
  {
    writer.writeStartElement(QStringLiteral("trk"));

    if(!flightplan.isEmpty())
      writer.writeTextElement(QStringLiteral("name"), QCoreApplication::applicationName() + tr(" - Track"));

    for(const TrailPoints& track : gpxData.getTrails())
    {
      if(track.isEmpty())
        continue;

      writer.writeStartElement(QStringLiteral("trkseg"));

      for(const TrailPoint& pos : track)
      {
        writer.writeStartElement(QStringLiteral("trkpt"));

        writer.writeAttribute(QStringLiteral("lon"), QString::number(pos.pos.getLonX(), 'f', 6));
        writer.writeAttribute(QStringLiteral("lat"), QString::number(pos.pos.getLatY(), 'f', 6));
        writer.writeTextElement(QStringLiteral("ele"), QString::number(atools::geo::feetToMeter(pos.pos.getAltitude())));

        if(pos.timestampMs > 0)
        {
          // (UTC/Zulu) in ISO 8601 format: "yyyy-mm-ddThh:mm:ssZ" or "yyyy-MM-ddTHH:mm:ss.zzzZ"
          // <time>2011-01-16T23:59:01Z</time>
          // Changes time number to local if Qt::UTC is omitted
          writer.writeTextElement(QStringLiteral("time"),
                                  QDateTime::fromMSecsSinceEpoch(pos.timestampMs, QTimeZone::UTC).toString(Qt::ISODateWithMs));
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
    atools::util::XmlStreamReader xmlStream(string);
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
    atools::util::XmlStreamReader xmlStream(&gpxFile);
    loadGpxInternal(gpxData, xmlStream);
    gpxFile.close();
  }
  else
    throw Exception(errorMsg.arg(filename).arg(gpxFile.errorString()));
}

void GpxIO::loadGpxInternal(atools::fs::gpx::GpxData& gpxData, atools::util::XmlStreamReader& xmlStream)
{
  xmlStream.readUntilElement(QStringLiteral("gpx"));
  PosD pos;
  QString name;
  gpxData.clear();

  while(xmlStream.readNextStartElement())
  {
    // Read route elements ======================================================
    if(xmlStream.name() == QLatin1String("rte"))
    {
      while(xmlStream.readNextStartElement())
      {
        if(xmlStream.name() == QLatin1String("rtept"))
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
    else if(xmlStream.name() == QLatin1String("trk"))
    {
      TrailPoints line;
      while(xmlStream.readNextStartElement())
      {
        if(xmlStream.name() == QLatin1String("trkseg"))
        {
          line.clear();
          while(xmlStream.readNextStartElement())
          {
            if(xmlStream.name() == QLatin1String("trkpt"))
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
