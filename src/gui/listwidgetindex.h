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

#ifndef ATOOLS_LISTWIDGETINDEX_H
#define ATOOLS_LISTWIDGETINDEX_H

#include <QBrush>
#include <QMultiMap>
#include <QSet>

class QListWidget;
class QStackedWidget;
class QObject;
class QWidget;

class QAbstractItemModel;
namespace atools {
namespace gui {

struct ListWidgetIndexMap;

/*
 * Manages search for phrases and words in a dialog window consisting of a stacked widget, related list and all child widgets.
 * Not matching list widget elements are hidden when searching. Widgets are highlighted using the set color including alpha channnel.
 *
 * Automatically builds a text search index from single to four consecutive words.
 * Loads a stopword list to filter out not relevant words.
 *
 * Stopwords are loaded and index is built on first use of find().
 */
class ListWidgetIndex
{
public:
  /* Widgets are required and have to be initialized before first use.
   * Pointers to all child widgets are stored which must not be deleted, therefore. */
  explicit ListWidgetIndex(QListWidget *listWidgetParam, QStackedWidget *stackedWidgetParam);
  ~ListWidgetIndex();

  /* Finds a phrase or word and handles all widgets.
   * Not matching list widget elements are hidden when searching and child widgets are highlighted using
   * the set color including alpha channnel.
   * Word size has to be two or more characters. */
  void find(QString text);

  /* Reset all widgets, hightlights and clears index to be reloaded on next use of find() */
  void reset();

  /* Set color including alpha channed to highlight widgets. */
  void setHighlightColor(const QColor& value)
  {
    highlightColor = value;
  }

private:
  /* Recurses through all child widgets of the stacked widget and collects text in index */
  void processObject(int pageIndex, QObject *object);

  /* Extracts relevant widget texts and returns an upper case cleaned up list. */
  QStringList widgetText(const QObject *object, bool& recurse);

  /* Remove all special characters, simplifies text and returns an upper case copy. Also strips off all HTML. */
  QStringList cleanText(const QStringList& text);

  /* Adds from single up to four consecutive words to the index from the list */
  void insertPhrases(const QStringList& texts, int pageIndex, QObject *object);

  /* Build full index */
  void buildIndex();

  /* Load stopword list from JSON file */
  void loadStopwords();

  /* Resets list and highlights back to previous values */
  void resetView();

  /* List and related stacked widget */
  QListWidget *listWidget;
  QStackedWidget *stackedWidget;

  /* Stopwords. Key is two letter language code having a value of stopwords */
  QHash<QString, QSet<QString> > stopwords;

  /* Stopword list depending on the current UI language */
  QSet<QString> *currentStopwords = nullptr;
  QSet<const QObject *> processedObjects;

  /* Internal text index */
  ListWidgetIndexMap *map;

  /* Saved style sheets of highlighted widgets */
  QHash<QWidget *, QString> savedStyles;

  /* Color used as frame or background for highlighted widgets */
  QColor highlightColor;

  /* Last user selected row / page index in index widget before searching */
  int lastCurrentRow = -1;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_LISTWIDGETINDEX_H
