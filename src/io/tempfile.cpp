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

#include "io/tempfile.h"

#include "exception.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QUuid>

namespace atools {
namespace io {

TempFile::TempFile(const QString& filepathParam, const QString& suffix, bool deleteOnExitParam)
  : deleteOnExit(deleteOnExitParam)
{
  QFile src(filepathParam);

  if(src.open(QIODevice::ReadOnly))
  {
    // Read data from source file
    QByteArray bytes = src.readAll();

    if(bytes.isEmpty() && src.error() != QFileDevice::NoError)
      throw atools::Exception(tr("Cannot read from \"%1\". Error: %2").arg(filepathParam).arg(src.errorString()));

    if(bytes.isEmpty())
      qWarning() << Q_FUNC_INFO << "Empty file" << src.fileName();

    // Write to temp file
    init(bytes, suffix);
    src.close();
  }
  else
    throw atools::Exception(tr("Cannot open \"%1\" for reading. Error: %2").arg(filepathParam).arg(src.errorString()));
}

TempFile::TempFile(const QByteArray& bytes, const QString& suffix, bool deleteOnExitParam)
  : deleteOnExit(deleteOnExitParam)
{
  init(bytes, suffix);
}

TempFile::~TempFile()
{
  if(deleteOnExit)
  {
    if(!filepath.isEmpty() && !QFile::remove(filepath))
      qWarning() << Q_FUNC_INFO << "Cannot remove temporary file" << filepath;
  }
}

QString TempFile::getTempFilename(const QString& suffix)
{
  // little_navmap_ef85eb54-a5b8-4a6a-890f-ee61a58f1ef9"suffix"
  return QDir::tempPath() + QStringLiteral("/%1-%2-%3%4").
         arg(QCoreApplication::organizationName().replace(' ', '_').toLower()).
         arg(QCoreApplication::applicationName().replace(' ', '_').toLower()).
         arg(QUuid::createUuid().toString(QUuid::Id128)).
         arg(suffix.isEmpty() ? ".temp" : suffix);
}

const QString& TempFile::getFilePath() const
{
  return filepath;
}

char *TempFile::getFilePathData()
{
  return filepathData.data();
}

const char *TempFile::getFilePathConstData() const
{
  return filepathData.data();
}

void TempFile::init(const QByteArray& bytes, const QString& suffix)
{
  QFile tempFile(getTempFilename(suffix));
  if(tempFile.open(QIODevice::WriteOnly))
  {
    tempFile.write(bytes);
    filepath = tempFile.fileName();
    filepathData = filepath.toUtf8();
    tempFile.close();
  }
  else
    throw atools::Exception(tr("Cannot open \"%1\" for writing. Error: %2").arg(tempFile.fileName()).arg(tempFile.errorString()));
}

} // namespace io
} // namespace atools
