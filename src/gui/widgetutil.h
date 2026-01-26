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

#ifndef ATOOLS_WIDGETTOOLS_H
#define ATOOLS_WIDGETTOOLS_H

#include <QList>

class QLayout;
class QWidget;
class QAction;
class QObject;
class QTextEdit;
class QColor;
class QMenu;
class QLabel;
class QItemSelectionModel;
class QSize;

namespace atools {
namespace gui {
namespace util {

/* Returns indexes in reverse order so that items can be deleted from the end of the list. */
const QList<int> getSelectedIndexesInDeletionOrder(QItemSelectionModel *selectionModel);

/* Changes the background color of the widget using stylesheets and adapts text color for readability */
void changeWidgetColor(QWidget *widget, QColor backgroundColor);

/* @return true if no scrollbar is pressed in the text edit */
bool canTextEditUpdate(const QTextEdit *textEdit);

/* Update text edit and keep selection and scrollbar position */
void updateTextEdit(QTextEdit *textEdit, const QString& text, bool scrollToTop, bool keepSelection, bool clearSelection = false);

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

} // namespace util
} // namespace gui
} // namespace atools

#endif // ATOOLS_WIDGETTOOLS_H
