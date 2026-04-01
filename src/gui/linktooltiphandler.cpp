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

#include "linktooltiphandler.h"

#include <QMouseEvent>
#include <QLabel>
#include <QTextBrowser>
#include <QToolTip>
#include <QUrlQuery>

namespace atools {
namespace gui {

const static QStringList URL_SCHEMES_WEB({QStringLiteral("http"), QStringLiteral("https"), QStringLiteral("ftp")});
const static QStringList URL_SCHEMES_FILE({QStringLiteral("file")});

void LinkTooltipHandler::clear()
{
  // Disconnect labels
  for(QLabel *label : std::as_const(labels))
    disconnect(label, &QLabel::linkHovered, this, &LinkTooltipHandler::linkHovered);

  // Remove event hanlder from text browsers
  for(QTextBrowser *textBrowser : std::as_const(textBrowsers))
    textBrowser->removeEventFilter(this);

  labels.clear();
  textBrowsers.clear();
}

void LinkTooltipHandler::addWidgets(const QList<QWidget *>& widgets)
{
  for(QWidget *widget : widgets)
  {
    if(widget != nullptr)
    {
      QLabel *label = dynamic_cast<QLabel *>(widget);
      if(label != nullptr)
      {
        // Use signal for label
        connect(label, &QLabel::linkHovered, this, &LinkTooltipHandler::linkHovered);
        labels.append(label);
      }
      else
      {
        QTextBrowser *textBrowser = dynamic_cast<QTextBrowser *>(widget);
        if(textBrowser != nullptr)
        {
          // Use event filter for text browser
          textBrowser->installEventFilter(this);
          textBrowsers.append(textBrowser);
        }
        else
          qWarning() << Q_FUNC_INFO << "Unknown type" << widget->metaObject()->className() << widget->objectName();
      }
    }
    else
      qWarning() << Q_FUNC_INFO << "Widget is null";
  }
}

bool LinkTooltipHandler::eventFilter(QObject *object, QEvent *event)
{
  // Use tooltip event
  if(showToolTips && event->type() == QEvent::ToolTip)
  {
    QHelpEvent *helpEvent = dynamic_cast<QHelpEvent *>(event);
    QTextBrowser *textBrowser = dynamic_cast<QTextBrowser *>(object);

    if(helpEvent != nullptr && textBrowser != nullptr)
      linkHovered(textBrowser->anchorAt(helpEvent->pos()));

    // Event consumed
    return true;
  }

  return QObject::eventFilter(object, event);
}

void LinkTooltipHandler::linkHovered(const QString& link)
{
  if(showToolTips)
  {
    const QUrl url(link);
    QString toolTip;

    if(URL_SCHEMES_WEB.contains(url.scheme()))
      toolTip = webUrlToolTip;
    else if(URL_SCHEMES_FILE.contains(url.scheme()))
      toolTip = fileUrlToolTip;
    else if(url.scheme() == QStringLiteral("lnm"))
    {
      const QString key = QUrlQuery(url).queryItemValue(QStringLiteral("tooltip"));
      if(!key.isEmpty())
        // Use value tooltip "showairport" in "lnm://show?id=%1&type=%2&tooltip=showairport"
        toolTip = urlKeyToolTipHash.value(key);
      else
        // "showairport" in "lnm://showairport"
        toolTip = urlKeyToolTipHash.value(url.host());
    }

    if(!toolTip.isEmpty())
      QToolTip::showText(QCursor::pos(), toolTip);
    else
      QToolTip::hideText();
  }
}

} // namespace gui
} // namespace atools
