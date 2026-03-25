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

#ifndef ATOOLS_GUI_LINKTOOLTIPHANDLER_H
#define ATOOLS_GUI_LINKTOOLTIPHANDLER_H

#include <QHash>
#include <QObject>

class QLabel;
class QTextBrowser;

namespace atools {
namespace gui {

typedef  QHash<QString, QString> LinkToolTipHashType;

/*
 * Shows tooltips for links in QLabel or QTextBrowser widgets. The tooltips are extracted by key found in the link.
 * Either a connection or an event filter is used to show the tooltips.
 *
 * Example key "showairport": "lnm://show?id=%1&type=%2&tooltip=showairport"
 */
class LinkTooltipHandler :
  public QObject
{
  Q_OBJECT

public:
  LinkTooltipHandler(QObject *parent)
    :QObject(parent)
  {
  }

  virtual ~LinkTooltipHandler() override
  {
    clear();
  }

  /* Do not allow copying */
  LinkTooltipHandler(const LinkTooltipHandler& other) = delete;
  LinkTooltipHandler& operator=(const LinkTooltipHandler& other) = delete;

  /* Add widgets where tooltips should be shown for links */
  void addWidgets(const QList<QWidget *>& widgets);

  void addWidget(QWidget *widget)
  {
    addWidgets({widget});
  }

  /* Add key and tooltip text value.
   * Example key "showairport": "lnm://show?id=%1&type=%2&tooltip=showairport"
   * Call addUrlTooltip("infoairport", tr("Click to show the airport information"));
   */
  void addUrlTooltip(const QString& key, const QString& tooltip)
  {
    urlKeyToolTipHash.insert(key, tooltip);
  }

  /* Remove tooltip key/value pairs */
  void clearUrlTooltips()
  {
    urlKeyToolTipHash.clear();
  }

  /* Clear internal lists, disconnect signals and remove event handlers for all widgets */
  void clear();

  /* Disable or enable tooltips */
  void setShowTooltips(bool showToolTipsParam)
  {
    showToolTips = showToolTipsParam;
  }

  /* Tooltip for all https, http and ftp URLs */
  void setWebUrlToolTip(const QString& newWebUrlToolTip)
  {
    webUrlToolTip = newWebUrlToolTip;
  }

  /* Tooltip for all file URLs */
  void setFileUrlToolTip(const QString& newFileUrlToolTip)
  {
    fileUrlToolTip = newFileUrlToolTip;
  }

private:
  virtual bool eventFilter(QObject *object, QEvent *event) override;
  void linkHovered(const QString& link);

  QList<QLabel *> labels;
  QList<QTextBrowser *> textBrowsers;

  atools::gui::LinkToolTipHashType urlKeyToolTipHash;
  QString webUrlToolTip, fileUrlToolTip;
  bool showToolTips = true;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_GUI_LINKTOOLTIPHANDLER_H
