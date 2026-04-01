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

#include "gui/tools.h"

#include "atools.h"
#include "gui/dialog.h"

#include <QAction>
#include <QApplication>
#include <QFontDatabase>
#include <QItemSelection>
#include <QLabel>
#include <QLayout>
#include <QDebug>
#include <QTextDocumentFragment>
#include <QWindow>
#include <QScreen>
#include <QTabWidget>
#include <QCheckBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QScrollBar>
#include <QSpinBox>
#include <QComboBox>

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
  for(const QString& family : QStringList({"Andale Mono", "Consolas", "Lucida Console", "Inconsolata"}))
  {
    fixedFont = QFontDatabase::font(family, "Normal", QApplication::font().pointSize());

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

void updateAllFonts(const QFont& font, const QSet<QObject *>& exceptionList)
{
  for(QWindow *window: QGuiApplication::allWindows())
  {
#ifdef DEBUG_INFORMATION
    if(window != nullptr)
      qDebug() << Q_FUNC_INFO << window->objectName() << window->title();
#endif
    updateAllFonts(window, font, exceptionList);
  }
}

void updateAllFonts(QObject *object, const QFont& font, const QSet<QObject *>& exceptionList)
{

  if(object != nullptr)
  {
    QWidget *widget = dynamic_cast<QWidget *>(object);
    if(widget != nullptr)
    {
      if(widget->font() != font && !exceptionList.contains(object))
      {
        widget->setFont(font);
        widget->updateGeometry();
      }

      // Recurse for widget children
      for(QObject *obj : widget->children())
        updateAllFonts(obj, font, exceptionList);
    }
    else
    {
      QLayout *layout = dynamic_cast<QLayout *>(object);
      if(layout != nullptr)
      {
        // Recurse for layout children
        for(int i = 0; i < layout->count(); i++)
        {
          updateAllFonts(layout->itemAt(i)->widget(), font, exceptionList);
          updateAllFonts(layout->itemAt(i)->layout(), font, exceptionList);
        }
      }
    }
  }
}

void updateAllPalette(const QPalette& palette, const QSet<QObject *>& exceptionList)
{
  for(QWindow *window: QGuiApplication::allWindows())
  {
#ifdef DEBUG_INFORMATION
    if(window != nullptr)
      qDebug() << Q_FUNC_INFO << window->objectName() << window->title();
#endif
    updateAllPalette(window, palette, exceptionList);
  }
}

void updateAllPalette(QObject *object, const QPalette& palette, const QSet<QObject *>& exceptionList)
{
  if(object != nullptr)
  {
    QWidget *widget = dynamic_cast<QWidget *>(object);
    if(widget != nullptr)
    {
      if(widget->palette() != palette && !exceptionList.contains(object))
        widget->setPalette(palette);

      // Recurse for widget children
      for(QObject *obj : widget->children())
        updateAllPalette(obj, palette, exceptionList);
    }
    else
    {
      QLayout *layout = dynamic_cast<QLayout *>(object);
      if(layout != nullptr)
      {
        // Recurse for layout children
        for(int i = 0; i < layout->count(); i++)
        {
          updateAllPalette(layout->itemAt(i)->widget(), palette, exceptionList);
          updateAllPalette(layout->itemAt(i)->layout(), palette, exceptionList);
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
    widget->updateGeometry();
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

bool canTextEditUpdate(const QTextEdit *textEdit)
{
  // Do not update if scrollbar is clicked
  return !textEdit->verticalScrollBar()->isSliderDown() &&
         !textEdit->horizontalScrollBar()->isSliderDown();
}

void updateTextEdit(QTextEdit *textEdit, const QString& text, bool scrollToTop, bool keepSelection, bool clearSelection)
{
  // Remember cursor position
  QTextCursor cursor = textEdit->textCursor();
  int pos = cursor.position();
  int anchor = cursor.anchor();

  // Remember scrollbar position
  int vScrollPos = textEdit->verticalScrollBar()->value();
  int hScrollPos = textEdit->horizontalScrollBar()->value();
  textEdit->setText(text);

  if(clearSelection)
  {
    // Remove the selection which appears after clicking the linkg
    QTextCursor textCursor = textEdit->textCursor();
    textCursor.clearSelection();
    textEdit->setTextCursor(textCursor);
  }
  else if(anchor != pos && keepSelection)
  {
    // There is a selection - Reset cursor
    int maxPos = textEdit->document()->characterCount() - 1;

    // Probably the document changed its size
    anchor = std::min(maxPos, anchor);
    pos = std::min(maxPos, pos);

    // Create selection again
    cursor.setPosition(anchor, QTextCursor::MoveAnchor);
    cursor.setPosition(pos, QTextCursor::KeepAnchor);
    textEdit->setTextCursor(cursor);
  }

  if(!scrollToTop)
  {
    // Reset scroll bars
    textEdit->verticalScrollBar()->setValue(vScrollPos);
    textEdit->horizontalScrollBar()->setValue(hScrollPos);
  }
}

void showHideLayoutElements(const QList<QLayout *> layouts, const QList<QWidget *>& otherWidgets, bool visible, bool disable)
{
  for(QWidget *w : otherWidgets)
    w->setVisible(visible);

  for(QLayout *layout : layouts)
  {
    for(int i = 0; i < layout->count(); i++)
    {
      QLayoutItem *item = layout->itemAt(i);
      if(item->widget() != nullptr)
      {
        item->widget()->setVisible(visible);

        if(disable)
          // Do not only hide but also disable
          item->widget()->setEnabled(visible);
      }

      if(item->layout() != nullptr)
        // Recurse
        showHideLayoutElements({item->layout()}, otherWidgets, visible, disable);
    }
  }
}

bool anyWidgetChanged(const QList<const QObject *>& widgets)
{
  bool changed = false;
  for(const QObject *widget : widgets)
  {
    if(widget != nullptr)
    {
      if(const QLayout *layout = dynamic_cast<const QLayout *>(widget))
      {
        for(int i = 0; i < layout->count(); i++)
        {
          changed |= anyWidgetChanged({layout->itemAt(i)->widget()});
          changed |= anyWidgetChanged({layout->itemAt(i)->layout()});
        }
      }
      else if(const QCheckBox *cbx = dynamic_cast<const QCheckBox *>(widget))
        changed |= cbx->isTristate() ?
                   cbx->checkState() != Qt::PartiallyChecked :
                   cbx->checkState() == Qt::Checked;
      else if(const QAction *a = dynamic_cast<const QAction *>(widget))
        changed |= a->isCheckable() && !a->isChecked();
      else if(const QAbstractButton *b = dynamic_cast<const QAbstractButton *>(widget))
        changed |= b->isCheckable() && !b->isChecked();
      else if(const QLineEdit *le = dynamic_cast<const QLineEdit *>(widget))
        changed |= !le->text().isEmpty();
      else if(const QTextEdit *te = dynamic_cast<const QTextEdit *>(widget))
        changed |= !te->document()->isEmpty();
      else if(const QSpinBox *sb = dynamic_cast<const QSpinBox *>(widget))
        changed |= sb->value() != sb->maximum() && sb->value() != sb->minimum();
      else if(const QDoubleSpinBox *dsb = dynamic_cast<const QDoubleSpinBox *>(widget))
        changed |= dsb->value() < dsb->maximum() && dsb->value() > dsb->minimum();
      else if(const QComboBox *cb = dynamic_cast<const QComboBox *>(widget))
        changed |= cb->currentIndex() != 0;
    }
  }
  return changed;
}

bool allChecked(const QList<const QAction *>& actions)
{
  bool notChecked = false;
  for(const QAction *action : actions)
    notChecked |= action->isCheckable() && !action->isChecked();
  return !notChecked;
}

bool noneChecked(const QList<const QAction *>& actions)
{
  bool checked = false;
  for(const QAction *action : actions)
    checked |= action->isCheckable() && action->isChecked();
  return !checked;
}

void changeIndication(QAction *action, bool changed)
{
  QString changeText(QObject::tr(" (changed)", "Indication for search menu button items"));
  QString changeUnusedText(QObject::tr(" (changed, not used)", "Indication for search menu button items"));

  if(action->text().endsWith(changeText))
    action->setText(action->text().remove(changeText));

  if(action->text().endsWith(changeUnusedText))
    action->setText(action->text().remove(changeUnusedText));

  if(changed)
    action->setText(action->text() + (action->isChecked() ? changeText : changeUnusedText));
}

void changeTabBarSize(const QList<QTabWidget *> tabWidgets, double factor)
{
  for(QTabWidget *tabWidget : tabWidgets)
    changeTabBarSize(tabWidget, factor);
}

void changeTabBarSize(QTabWidget *tabWidget, double factor)
{
  tabWidget->setStyleSheet(QStringLiteral("QTabBar::tab { height: %1px; }").
                           arg(QString::number(atools::roundToInt(QFontMetricsF(QApplication::font()).height() * factor))));
  tabWidget->updateGeometry();
}

void changeWidgetColor(QWidget *widget, QColor backgroundColor)
{
#if !defined(Q_OS_MACOS)
  if(widget->isEnabled())
  {
    QColor foregroundColor(backgroundColor.value() < 180 ? Qt::white : Qt::black);
    widget->setStyleSheet(QStringLiteral("background-color: ") % backgroundColor.name() % QStringLiteral("; color: ") %
                          foregroundColor.name());
  }
  else
    widget->setStyleSheet(QStringLiteral());
#else
  Q_UNUSED(backgroundColor)
  Q_UNUSED(widget)
#endif
}

const QList<int> getSelectedIndexesInDeletionOrder(QItemSelectionModel *selectionModel)
{
  QList<int> indexes;
  if(selectionModel != nullptr)
  {
    // Create list in reverse order so that deleting can start at the bottom of the list
    const QModelIndexList localSelectedRows = selectionModel->selectedRows();
    for(const QModelIndex& index : localSelectedRows)
      indexes.append(index.row());

    std::sort(indexes.begin(), indexes.end());
    std::reverse(indexes.begin(), indexes.end());

  }
  return indexes;
}

void labelForcedUpdate(QLabel *label)
{
  QString text = label->text();
  label->clear();
  label->setText(text);
}

void centerWidgetOnScreen(QWidget *widget, const QSize& size)
{
  QSize widgetSize = size.isValid() ? size : widget->size();
  widget->resize(widgetSize);

  QRect availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
  widget->move(availableGeometry.x() + (availableGeometry.width() - widgetSize.width()) / 2,
               availableGeometry.y() + (availableGeometry.height() - widgetSize.height()) / 2);

}

void ensureVisibility(QWidget *mainWindow)
{
  // Has to be visible for 40 pixels at least on one screen
  static const int BUFFER_PIXELS = 40;
  bool visible = false;

  const QList<QScreen *> screens = QGuiApplication::screens();
  for(const QScreen *screen : screens)
  {
    QRect geometry = screen->availableGeometry();
    QRect intersected = geometry.intersected(mainWindow->frameGeometry());
    if(intersected.width() > BUFFER_PIXELS && intersected.height() > BUFFER_PIXELS)
    {
      visible = true;
      qDebug() << Q_FUNC_INFO << "Visibility" << visible << "on" << screen->name() << geometry
               << "window frame" << mainWindow->frameGeometry();
      break;
    }
  }

  if(!visible)
  {
    qDebug() << Q_FUNC_INFO << "Getting window back on screen" << QGuiApplication::primaryScreen()->name()
             << QGuiApplication::primaryScreen()->availableGeometry() << "window frame" << mainWindow->frameGeometry();
    centerWidgetOnScreen(mainWindow, QSize());
  }
}

void setWidgetAndIconSize(const QList<QWidget *>& widgets, const QSize& size, int iconSizePercent)
{
  for(QWidget *widget : widgets)
  {
    widget->setMinimumSize(size);

    QAbstractButton *button = dynamic_cast<QAbstractButton *>(widget);
    if(button != nullptr && !button->icon().isNull())
      button->setIconSize(size * iconSizePercent / 100);

    widget->updateGeometry();
  }
}

} // namespace gui
} // namespace atools
