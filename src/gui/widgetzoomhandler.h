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

#ifndef ATOOLS_WIDGETZOOMHANDLER_H
#define ATOOLS_WIDGETZOOMHANDLER_H

#include <QMutex>
#include <QObject>
#include <QSet>

class QAction;
class QWidget;

namespace atools {
namespace gui {

/*
 * Maintains font size/zoom functionality for table or tree widgets. Supports save and restore.
 * Contains special handling for tables and trees. Does not support tables and trees using different font sizes.
 *
 * Collects all managed widgets in contructor and unregisteres in destructor. Thread safe.
 */
class WidgetZoomHandler :
  public QObject
{
  Q_OBJECT

public:
  /* Will not support any zooming but set the initial cell and font size
   * if constructed with tableview parameter only. Otherwise font size can be changed using the given
   * actions.*/
  WidgetZoomHandler(QWidget *widgetParam, QAction *zoomInAction, QAction *zoomOutAction, QAction *zoomDefaultAction,
                    QString settingsKeyStr, double marginParm)
  {
    init(widgetParam, zoomInAction, zoomOutAction, zoomDefaultAction, settingsKeyStr, marginParm);
  }

  WidgetZoomHandler(QWidget *widgetParam, double marginParam)
  {
    init(widgetParam, nullptr, nullptr, nullptr, QString(), marginParam);
  }

  WidgetZoomHandler(QWidget *widgetParam)
  {
    init(widgetParam, nullptr, nullptr, nullptr, QString(), 0.);
  }

  virtual ~WidgetZoomHandler() override;

  /* Do not allow copying */
  WidgetZoomHandler(const WidgetZoomHandler& other) = delete;
  WidgetZoomHandler& operator=(const WidgetZoomHandler& other) = delete;

  /* Use zoom methods for direct changes instead actions */
  void zoomTableView(int value);

  void zoomIn()
  {
    zoomTableView(1);
  }

  void zoomOut()
  {
    zoomTableView(-1);
  }

  void zoomDefault()
  {
    zoomTableView(0);
  }

  /* Bypasses all in/out/default methods and increases/decreases the size. */
  void zoomPercent(int percent = 100);

  /* Section height will be font height plus this value */
  float getSectionToFontSize() const
  {
    return static_cast<float>(sectionToFontSize);
  }

  void setSectionToFontSize(int value)
  {
    sectionToFontSize = value;
  }

  /* Minimum font size in pixel */
  float getMinFontSize() const
  {
    return static_cast<float>(minFontSize);
  }

  void setMinFontSize(int value)
  {
    minFontSize = value;
  }

  /* Maximum font size in pixel */
  float getMaxFontSize() const
  {
    return static_cast<float>(maxFontSize);
  }

  void setMaxFontSize(int value)
  {
    maxFontSize = value;
  }

  /* Get widget as passed into the constructor */
  QWidget *getWidget() const
  {
    return widget;
  }

  /* Get a copy of the list with all managed widgets */
  static const QSet<QObject *> getRegisteredWidgets();

private:
  void init(QWidget *widgetParam, QAction *zoomInAction, QAction *zoomOutAction,
            QAction *zoomDefaultAction, QString settingsKeyStr, double marginParm);

  /* Change font size and store in settings */

  /* Update action status (enabled/disabled) */
  void enableDisableZoomActions();

  /* Init table row height according to font size (smaller than default) */
  void initTableViewZoom();

  /* Change font size and adjust row height accordingly */
  void setTableViewFontSize(double pointSize);

  double sectionToFontSize = 2.;
  double minFontSize = 7.;
  double maxFontSize = 16.;
  double margin = 0.;

  QWidget *widget;
  QAction *actionZoomIn, *actionZoomOut, *actionZoomDefault;
  QString settingsKey;

  /* Collects all managed widgets in contructor and unregisteres in destructor. Thread safe. */
  static QSet<QObject *> registeredWidgets;
  static QMutex registeredWidgetsMutex;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_WIDGETZOOMHANDLER_H
