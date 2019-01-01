/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_FS_STATUSTEXTPARSER_H
#define ATOOLS_FS_STATUSTEXTPARSER_H

#include <QStringList>

class QTextStream;

namespace atools {
namespace fs {
namespace online {

/*
 * Reads IVAO, VATSIM or custom status.txt file.
 */
class StatusTextParser
{
public:
  StatusTextParser();
  ~StatusTextParser();

  /* Read file content given in string */
  void read(QString file);

  /* Read file content given in stream */
  void read(QTextStream& stream);

  /* Reset internal state back */
  void reset();

  /* Get one URL pointing to a whazzup.txt from the file. This is key "url0".
   *  A round robin strategy is used to select the URLs from the list. */
  QString getRandomUrl(bool& gzipped);

  /* Get one URL pointing to a whazzup.txt from the file which contains server information. This is key "url1".
   *  A round robin strategy is used to select the URLs from the list. */
  QString getRandomVoiceUrl();

  /* Get the message which has to be shown if available */
  const QString& getMessage() const
  {
    return message;
  }

private:
  QStringList urlList, urlListGzip, urlListVoice;
  int curUrlIndex = 0, curGzipUrlIndex = 0, curVoiceUrlIndex = 0;
  QString message;
};

} // namespace online
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_STATUSTEXTPARSER_H
