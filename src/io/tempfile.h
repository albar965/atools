/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_TEMPFILE_H
#define ATOOLS_TEMPFILE_H

#include <QApplication>

namespace atools {
namespace io {

/*
 * Creates a temporary file either from another filename (e.g. resources)
 * or a byte array. File is closed after construction/copying and deleted in the destructor.
 *
 * File is created in the system temp folder and contains application name and a GUID.
 *
 * Throws atools::Exception if files cannot be created, read or written.
 */
class TempFile
{
  Q_DECLARE_TR_FUNCTIONS(TempFile)

public:
  /* Default suffix is ".temp" if empty */
  TempFile(const QString& filepathParam, const QString& suffix = QString());
  TempFile(const QByteArray& bytes, const QString& suffix = QString());
  ~TempFile();

  /* Get full path and filename */
  const QString& getFilePath() const;

  /* Methods to get path as UTF-8 string for C code */
  char *getFilePathData();
  const char *getFilePathConstData() const;

private:
  void init(const QByteArray& bytes, const QString& suffix);
  QString buildFilename(const QString& suffix);

  QString filepath;

  // Need a separate copy to avoid temporary objects when returning pointer to data
  QByteArray filepathData;
};

} // namespace io
} // namespace atools

#endif // ATOOLS_TEMPFILE_H
