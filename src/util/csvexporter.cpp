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

#include "util/csvexporter.h"

#include "sql/sqlexport.h"

#include <QIODevice>
#include <QHeaderView>
#include <QTableView>

using atools::sql::SqlExport;

namespace atools {
namespace util {

bool CsvExporter::isExported(int columnIndex) const
{
  return (includeHidden || !view->isColumnHidden(columnIndex)) &&
         (includeFolded || view->columnWidth(columnIndex) > view->horizontalHeader()->minimumSectionSize());
}

int CsvExporter::logicalIndex(int visualIndex) const
{
  // Returns the logicalIndex for the section at the given visualIndex position
  // visualIndex is not affected by hidden sections
  return useLogicalIndex ? view->horizontalHeader()->logicalIndex(visualIndex) : visualIndex;
}

CsvExporter::CsvExporter(QTableView *viewParam)
  :view(viewParam)
{
  exporter = new atools::sql::SqlExport;
  exporter->setSeparatorChar(';');
  exporter->setEndline(false);
}

CsvExporter::~CsvExporter()
{
  delete exporter;
}

void CsvExporter::exportColumns(QTextStream& stream, int row)
{
  QVariantList variantList;
  QAbstractItemModel *model = view->model();
  for(int viewCol = 0; viewCol < model->columnCount(); viewCol++)
  {
    // Convert view position to model position - needed to keep order
    int columnLogicalIndex = logicalIndex(viewCol);

    if(columnLogicalIndex == -1 || skipColumIndexes.contains(columnLogicalIndex))
      continue;

    if(isExported(columnLogicalIndex))
    {
      if(dataFunction)
        variantList.append(dataFunction(row, columnLogicalIndex));
      else
        variantList.append(model->data(model->index(row, columnLogicalIndex)));
    }
  }

  stream << exporter->getResultSetRow(variantList) %
    (additionalHeaders.isEmpty() ||
     !additionalFieldFunction ? QStringLiteral() : ';' % additionalFieldFunction(row).join(';')) << Qt::endl;

  numRowsExported++;
}

QString CsvExporter::exportTableSelection()
{
  QString result;
  QTextStream stream(&result, QIODevice::WriteOnly);
  exportTableSelection(stream);
  return result;
}

void CsvExporter::exportTableSelection(QTextStream& stream)
{
  numRowsExported = 0;
  QItemSelectionModel *selection = view->selectionModel();

  // Copy full selected rows ==========================================
  if(header)
    stream << buildHeader() << Qt::endl;

  if(selection != nullptr && selection->hasSelection())
  {
    for(const QItemSelectionRange& rng : selection->selection())
    {
      // Add data
      for(int row = rng.top(); row <= rng.bottom(); ++row)
        exportColumns(stream, row);
    }
    stream.flush();
  }
}

QString CsvExporter::exportTable()
{
  QString result;
  QTextStream stream(&result, QIODevice::WriteOnly);
  exportTable(stream);
  return result;
}

void CsvExporter::exportTable(QTextStream& stream)
{
  numRowsExported = 0;
  atools::sql::SqlExport exporter;
  exporter.setSeparatorChar(';');
  exporter.setEndline(false);

  QAbstractItemModel *model = view->model();

  // Copy full rows
  if(header)
    stream << buildHeader() << Qt::endl;

  for(int row = 0; row < model->rowCount(); row++)
    exportColumns(stream, row);

  stream.flush();
}

QString CsvExporter::buildHeader() const
{
  QAbstractItemModel *model = view->model();
  QStringList headers;
  for(int viewCol = 0; viewCol < model->columnCount(); viewCol++)
  {
    // Convert view position to model position - needed to keep order
    int columnLogicalIndex = logicalIndex(viewCol);

    if(columnLogicalIndex == -1 || skipColumIndexes.contains(columnLogicalIndex))
      continue;

    if(isExported(columnLogicalIndex))
      headers.append(model->headerData(columnLogicalIndex, Qt::Horizontal).toString().
                     replace(QStringLiteral("-\n"), QStringLiteral()).replace('\n', ' '));
  }

  return exporter->getResultSetHeader(headers) %
         (additionalHeaders.isEmpty() ||
          !additionalFieldFunction ? QStringLiteral() : ';' % additionalHeaders.join(';'));
}

} // namespace util
} // namespace atools
