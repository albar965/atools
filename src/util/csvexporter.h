/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#ifndef LNM_CSV_EXPORTER_H
#define LNM_CSV_EXPORTER_H

#include <QCoreApplication>

namespace atools {
namespace sql {
class SqlExport;
}
}

class QTableView;

namespace atools {
namespace util {

/*
 * Allows to export the table content or the selected table content from the
 * given QTableView into CSV files.
 *
 * CSV files are according to RFC: https://tools.ietf.org/html/rfc4180
 * Separator is always ';' and quotation is '"'.
 */
class CsvExporter
{
  Q_DECLARE_TR_FUNCTIONS(CsvExporter)

public:
  explicit CsvExporter(QTableView *viewParam)
    :view(viewParam)
  {
  }

  /* Copies selection in table as CSV. */
  void exportTableSelection();

  /* Copies full table content as CSV. */
  void exportTable();

  /* Get result */
  const QString& getCsv() const
  {
    return result;
  }

  /* true if result is not empty */
  bool hasCsv() const
  {
    return !result.isEmpty();
  }

  /* Number of rows exported to result */
  int getNumRowsExported() const
  {
    return numRowsExported;
  }

  /* Add header or not */
  void setHeader(bool headerParam)
  {
    header = headerParam;
  }

  /* Export full rows or only selected cells */
  void setRows(bool rowsParam)
  {
    rows = rowsParam;
  }

  /* Include rows which are hidden in the view */
  void setIncludeHidden(bool includeHiddenParam)
  {
    includeHidden = includeHiddenParam;
  }

  /* Include rows which are folded to minimum size */
  void setIncludeFolded(bool includeFoldedParam)
  {
    includeFolded = includeFoldedParam;
  }

  /* Use logical index to export as shown in the view */
  void setUseLogicalIndex(bool useLogicalIndexParam)
  {
    useLogicalIndex = useLogicalIndexParam;
  }

  /* Add additional headers and columns. This has to be used with the setAdditionalFieldFunction() */
  void setAdditionalHeaders(const QStringList& additionalHeadersParam)
  {
    additionalHeaders = additionalHeadersParam;
  }

  /* Return a string list for all additional columns ( setAdditionalHeaders() ) */
  void setAdditionalFieldFunction(const std::function<QStringList(int)>& additionalFieldsFunctionParam)
  {
    additionalFieldFunction = additionalFieldsFunctionParam;
  }

  /* Data function for user defined formatting */
  void setDataFunction(const std::function<QVariant(int, int)>& dataFunctionParam)
  {
    dataFunction = dataFunctionParam;
  }

private:
  QString buildHeader(const atools::sql::SqlExport& exporter);
  bool isExported(int logicalCol);
  int index(int columnIndex);

  bool header = true, rows = true, includeHidden = false, includeFolded = false, useLogicalIndex = true;
  QTableView *view = nullptr;
  QStringList additionalHeaders;

  QString result;
  int numRowsExported = 0;

  std::function<QStringList(int)> additionalFieldFunction = nullptr;
  std::function<QVariant(int, int)> dataFunction = nullptr;
};

} // namespace util
} // namespace atools

#endif // LNM_CSV_EXPORTER_H
