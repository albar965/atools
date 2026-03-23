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

#ifndef ATOOLS_GUI_TOOLS_H
#define ATOOLS_GUI_TOOLS_H

#include <QList>
#include <QMessageBox>

class QString;
class QWidget;
class QUrl;
class QFont;
class QLabel;
class QItemSelectionModel;
class QAction;
class QObject;
class QTextEdit;
class QTabWidget;

namespace atools {
namespace gui {

void logMessageBox(QWidget *parent, QMessageBox::Icon icon, const QString& text);

/* Get best fixed system font avoiding the unreadable courier on Windows */
QFont getBestFixedFont();

/* Make selection color of inactive table elements the same as active on Windows */
void adjustSelectionColors(QWidget *widget);

/* Return a user readable text for the given font */
QString fontDescription(const QFont& font);

/* Sets label text and label font */
void fontDescription(const QFont& font, QLabel *label);

/* Updates all fonts recursively for objects and their children.
 * Workaround for Qt limitation which does not adapt all widgets and no floating windows with parent null. */
void updateAllFonts(const QFont& font, const QSet<QObject *>& exceptionList = QSet<QObject *>());
void updateAllFonts(QObject *object, const QFont& font, const QSet<QObject *>& exceptionList = QSet<QObject *>());

/* Updates all palette recursively for objects and their children.
 * Workaround for Qt limitation which does not adapt all widgets and no floating windows with parent null. */
void updateAllPalette(const QPalette& palette, const QSet<QObject *>& exceptionList = QSet<QObject *>());
void updateAllPalette(QObject *object, const QPalette& palette, const QSet<QObject *>& exceptionList = QSet<QObject *>());

/* Set font size in widgets and layouts recursively based on percent application font size. */
void setWidgetFontSize(QWidget *widget, int percent);

/* Get a list of all selected rows in given order ignoring any selection ranges */
QList<int> selectedRows(QItemSelectionModel *model, bool reverse);

/* true if action is enabled AND checked */
bool checked(const QAction *action);

/* Returns indexes in reverse order so that items can be deleted from the end of the list. */
const QList<int> getSelectedIndexesInDeletionOrder(QItemSelectionModel *selectionModel);

/* Changes the background color of the widget using stylesheets and adapts text color for readability */
void changeWidgetColor(QWidget *widget, QColor backgroundColor);

/* @return true if no scrollbar is pressed in the text edit */
bool canTextEditUpdate(const QTextEdit *textEdit);

/* Update text edit and keep selection and scrollbar position */
void updateTextEdit(QTextEdit *textEdit, const QString& text, bool scrollToTop, bool keepSelection, bool clearSelection = false);

/* Change tab height using given factor on font size */
void changeTabBarSize(QTabWidget *tabWidget, double factor = 1.6);
void changeTabBarSize(QList<QTabWidget *> tabWidgets, double factor = 1.6);

/*
 * Shows or hides all widgets in a list of layouts.
 * @param layouts all widgets in these layouts will have their visibility changed
 * @param visible hide or show widgets
 * @param disable disable hidden widgets if true. Enable if unhidden
 * @param otherWidgets other widgets not part of the layout that will have their visibility changed
 */
void showHideLayoutElements(const QList<QLayout *> layouts,
                            const QList<QWidget *>& otherWidgets, bool visible, bool disable);

/*
 * Check is a list of widgets has their state at default (i.e. a checkbox is
 * unchecked or a combo box is at index 0)
 * @param widgets that will be checked
 * @return true if any widget in the list does not have its default state
 */
bool anyWidgetChanged(const QList<const QObject *>& widgets);

/* Centers the widget on the primary screen (not its parent) */
void centerWidgetOnScreen(QWidget *widget, const QSize& size);

/* Moves widget to be visible on main screen */
void ensureVisibility(QWidget *widget);

/*
 * @return true if all actions that are checkable are checked
 */
bool allChecked(const QList<const QAction *>& actions);

/*
 * @return true if all actions that are checkable are not checked
 */
bool noneChecked(const QList<const QAction *>& actions);

/* Add a " (changed)"/" (changed, unused)" to an action text or removes it. Latter text if action is not checked. */
void changeIndication(QAction *action, bool changed);

/* Remember text, clear label and set text again to force update after style changes */
void labelForcedUpdate(QLabel *label);

} // namespace gui
} // namespace atools

#endif // ATOOLS_GUI_TOOLS_H
