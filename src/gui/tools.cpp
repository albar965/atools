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

#include "gui/tools.h"
#include "gui/dialog.h"

#include <QAction>
#include <QApplication>
#include <QFontDatabase>
#include <QItemSelection>
#include <QLabel>
#include <QLayout>
#include <QDebug>
#include <QTextDocumentFragment>

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

namespace atools {
namespace gui {

void fontDescription(const QFont& font, QLabel *label)
{
  label->setFont(font);
  label->setText(fontDescription(font));
}

QString fontDescription(const QFont& font)
{
  QStringList fontText;

  fontText.append(font.family());
  if(font.pointSizeF() > 0.)
    fontText.append(QObject::tr("%1 pt").arg(font.pointSizeF()));
  else if(font.pixelSize() > 0)
    fontText.append(QObject::tr("%1 px").arg(font.pixelSize()));

  int weight = font.weight();
  if(weight == QFont::Thin)
    fontText.append(QObject::tr("thin"));
  else if(weight <= QFont::ExtraLight)
    fontText.append(QObject::tr("extra light"));
  else if(weight <= QFont::Light)
    fontText.append(QObject::tr("light"));
  else if(weight <= QFont::Normal)
    fontText.append(QObject::tr("normal"));
  else if(weight <= QFont::Medium)
    fontText.append(QObject::tr("medium"));
  else if(weight <= QFont::DemiBold)
    fontText.append(QObject::tr("demi bold"));
  else if(weight <= QFont::Bold)
    fontText.append(QObject::tr("bold"));
  else if(weight <= QFont::ExtraBold)
    fontText.append(QObject::tr("extra bold"));
  else if(weight >= QFont::Black)
    fontText.append(QObject::tr("black"));

  if(font.italic())
    fontText.append(QObject::tr("italic"));
  if(font.overline())
    fontText.append(QObject::tr("overline"));
  if(font.underline())
    fontText.append(QObject::tr("underline"));
  if(font.strikeOut())
    fontText.append(QObject::tr("strike out"));

  if(font.fixedPitch())
    fontText.append(QObject::tr("fixed pitch"));

  QString prefix;
  if(font == QFontDatabase::systemFont(QFontDatabase::GeneralFont))
    prefix = QObject::tr("System font: %1");
  else
    prefix = QObject::tr("User selected font: %1");

  return prefix.arg(fontText.join(QObject::tr(", ")));
}

QList<int> selectedRows(QItemSelectionModel *model, bool reverse)
{
  QList<int> rows;

  if(model != nullptr)
  {
    const QItemSelection sm = model->selection();
    for(const QItemSelectionRange& rng : sm)
    {
      for(int row = rng.top(); row <= rng.bottom(); row++)
        rows.append(row);
    }

    if(!rows.isEmpty())
    {
      // Sort columns
      std::sort(rows.begin(), rows.end());
      if(reverse)
        std::reverse(rows.begin(), rows.end());
    }

    // Remove duplicates
    rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
  }

  return rows;
}

bool checked(const QAction *action)
{
  return action->isEnabled() && action->isChecked();
}

QFont getBestFixedFont()
{
  QFont fixedFont;

#if defined(Q_OS_WINDOWS)
  bool found = false;
  QFontDatabase fontDb;
  for(const QString& family : QStringList({"Andale Mono", "Consolas", "Lucida Console", "Inconsolata"}))
  {
    fixedFont = fontDb.font(family, "Normal", QApplication::font().pointSize());

    // Get information about the actually used font
    QFontInfo info(fixedFont);
    if(info.family() == family && info.fixedPitch() && info.style() == QFont::StyleNormal)
    {
      found = true;
      break;
    }
  }

  if(!found)
    // Fall back to default (Courier on Windows)
    fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
#else
  // Get system defined fixed font
  fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
  // Need to enlarge a bit
  fixedFont.setPointSizeF(fixedFont.pointSizeF() * 1.2);
#endif

  return fixedFont;
}

void adjustSelectionColors(QWidget *widget)
{
#if defined(Q_OS_WIN32)
  QPalette palette = widget->palette();
  palette.setColor(QPalette::Inactive, QPalette::Highlight, palette.color(QPalette::Active, QPalette::Highlight));
  palette.setColor(QPalette::Inactive, QPalette::HighlightedText, palette.color(QPalette::Active, QPalette::HighlightedText));
  widget->setPalette(palette);
#else
  Q_UNUSED(widget)
#endif
}

void updateAllFonts(QObject *object, const QFont& font)
{
  if(object != nullptr)
  {
    QWidget *widget = dynamic_cast<QWidget *>(object);
    if(widget != nullptr)
    {
      if(widget->font() != font)
        widget->setFont(font);

      // Recurse for widget children
      for(QObject *obj : widget->children())
        updateAllFonts(obj, font);
    }
    else
    {
      QLayout *layout = dynamic_cast<QLayout *>(object);
      if(layout != nullptr)
      {
        // Recurse for layout children
        for(int i = 0; i < layout->count(); i++)
        {
          updateAllFonts(layout->itemAt(i)->widget(), font);
          updateAllFonts(layout->itemAt(i)->layout(), font);
        }
      }
    }
  }
}

void setWidgetFontSize(QWidget *widget, int percent)
{
  QFont font = QApplication::font();
  double size = font.pointSizeF() * percent / 100.;
  if(size > 0.5 && size < 90.)
  {
    font.setPointSizeF(size);
    widget->setFont(font);
  }
}

void logMessageBox(QWidget *parent, QMessageBox::Icon icon, const QString& text)
{
  QString parentName = parent != nullptr ? parent->objectName() : "no name";
  QString plainText = QTextDocumentFragment::fromHtml(text).toPlainText();

  switch(icon)
  {
    case QMessageBox::NoIcon:
    case QMessageBox::Information:
    case QMessageBox::Question:
      qInfo().noquote().nospace() << Q_FUNC_INFO << " parent " << parentName << " message \"" << plainText << "\"";
      break;

    case QMessageBox::Warning:
      qWarning().noquote().nospace() << Q_FUNC_INFO << " parent " << parentName << " message \"" << plainText << "\"";
      break;

    case QMessageBox::Critical:
      qCritical().noquote().nospace() << Q_FUNC_INFO << " parent " << parentName << " message \"" << plainText << "\"";
      break;
  }
}

} // namespace gui
} // namespace atools
