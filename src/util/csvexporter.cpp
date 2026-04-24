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

bool CsvExporter::isExported(int columnIndex)
{
  return (includeHidden || !view->isColumnHidden(columnIndex)) &&
         (includeFolded || view->columnWidth(columnIndex) > view->horizontalHeader()->minimumSectionSize());
}

int CsvExporter::index(int columnIndex)
{
  return useLogicalIndex ? view->horizontalHeader()->logicalIndex(columnIndex) : columnIndex;
}

void CsvExporter::exportTableSelection()
{
  numRowsExported = 0;
  atools::sql::SqlExport exporter;
  exporter.setSeparatorChar(';');
  exporter.setEndline(false);

  QAbstractItemModel *model = view->model();

  QItemSelectionModel *selection = view->selectionModel();
  if(selection != nullptr && selection->hasSelection())
  {
    if(rows)
    {
      // Copy full selected rows ==========================================
      QTextStream stream(&result, QIODevice::WriteOnly);
      if(header)
        stream << buildHeader(exporter) << Qt::endl;

      for(const QItemSelectionRange& rng : selection->selection())
      {
        // Add data
        for(int row = rng.top(); row <= rng.bottom(); ++row)
        {
          QVariantList variantList;
          for(int viewCol = 0; viewCol < model->columnCount(); viewCol++)
          {
            // Convert view position to model position - needed to keep order
            int columnIndex = index(viewCol);

            if(columnIndex == -1)
              continue;

            if(isExported(columnIndex))
            {
              if(dataFunction)
                variantList.append(dataFunction(row, columnIndex));
              else
                variantList.append(model->data(model->index(row, columnIndex)));
            }
          }

          stream << exporter.getResultSetRow(variantList) %
            (additionalHeaders.isEmpty() ||
             !additionalFieldFunction ? QStringLiteral() : ';' % additionalFieldFunction(row).join(';')) << Qt::endl;

          numRowsExported++;
        }
      }
      stream.flush();
    }
    else
    {
      // Copy selected fields - not a real CSV ==========================================
      QStringList resultList;
      for(const QModelIndex& index : selection->selectedIndexes())
      {
        if(dataFunction)
          resultList.append(dataFunction(index.row(), index.column()).toString());
        else
          resultList.append(model->data(index).toString());
        numRowsExported++;
      }
      result = resultList.join(';');
    }
  }
}

void CsvExporter::exportTable()
{
  atools::sql::SqlExport exporter;
  exporter.setSeparatorChar(';');
  exporter.setEndline(false);

  QAbstractItemModel *model = view->model();

  // Copy full rows
  QTextStream stream(&result, QIODevice::WriteOnly);
  if(header)
    stream << buildHeader(exporter) << Qt::endl;

  for(int row = 0; row < model->rowCount(); row++)
  {
    QVariantList variantList;
    for(int viewCol = 0; viewCol < model->columnCount(); viewCol++)
    {
      // Convert view position to model position - needed to keep order
      int columnIndex = index(viewCol);

      if(columnIndex == -1)
        continue;

      if(isExported(columnIndex))
      {
        if(dataFunction)
          variantList.append(dataFunction(row, columnIndex));
        else
          variantList.append(model->data(model->index(row, columnIndex)));
      }
    }

    stream << exporter.getResultSetRow(variantList) %
      (additionalHeaders.isEmpty() ||
       !additionalFieldFunction ? QStringLiteral() : ';' % additionalFieldFunction(row).join(';')) << Qt::endl;

    numRowsExported++;
  }
  stream.flush();
}

QString CsvExporter::buildHeader(const atools::sql::SqlExport& exporter)
{
  QAbstractItemModel *model = view->model();
  QStringList headers;
  for(int viewCol = 0; viewCol < model->columnCount(); viewCol++)
  {
    // Convert view position to model position - needed to keep order
    int columnIndex = index(viewCol);

    if(columnIndex == -1)
      continue;

    if(isExported(columnIndex))
      headers.append(model->headerData(columnIndex, Qt::Horizontal).toString().
                     replace(QStringLiteral("-\n"), QStringLiteral()).replace('\n', ' '));
  }

  return exporter.getResultSetHeader(headers) %
         (additionalHeaders.isEmpty() ||
          !additionalFieldFunction ? QStringLiteral() : ';' % additionalHeaders.join(';'));
}

} // namespace util
} // namespace atools
