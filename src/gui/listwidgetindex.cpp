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

#include "listwidgetindex.h"

#include "zip/gzip.h"

#include <QAbstractButton>
#include <QComboBox>
#include <QFile>
#include <QGroupBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QStackedWidget>
#include <QTextDocumentFragment>
#include <QTextEdit>
#include <QStringBuilder>
#include <QApplication>
#include <QSplitter>

namespace atools {
namespace gui {

const static QString STOPWORDS_FILE(":/atools/resources/json/stopwords-iso.json.gz");
const static int MIN_STRING_SIZE = 2;

/* Index entry matching texts to widgets. Comparable and hashable. */
class ListWidgetIndexEntry
{
public:
  ListWidgetIndexEntry(int stackedWidgetIndexParam, QObject *objectParam)
    : stackedWidgetIndex(stackedWidgetIndexParam), object(objectParam)
  {
  }

  bool operator==(const ListWidgetIndexEntry& other) const
  {
    return stackedWidgetIndex == other.stackedWidgetIndex && object == other.object;
  }

  bool operator!=(const ListWidgetIndexEntry& other) const
  {
    return !operator==(other);
  }

  /* List/stack widget index for page */
  int getStackedWidgetIndex() const
  {
    return stackedWidgetIndex;
  }

  /* Frame, layout or widget */
  QObject *getObject() const
  {
    return object;
  }

private:
  int stackedWidgetIndex;
  QObject *object;
};

inline size_t qHash(const atools::gui::ListWidgetIndexEntry& entry, size_t seed)
{
  return qHashMulti(seed, entry.getStackedWidgetIndex(), entry.getObject());
}

struct ListWidgetIndexMap
{
  /* Internal index mapping texts to list indexes and widgets */
  QMultiMap<QString, ListWidgetIndexEntry> index;
};

// =======================================================================================
ListWidgetIndex::ListWidgetIndex(QSplitter *splitterWidgetParam, QListWidget *listWidgetParam, QStackedWidget *stackedWidgetParam)
  : listWidget(listWidgetParam), stackedWidget(stackedWidgetParam), splitterWidget(splitterWidgetParam), highlightColor(255, 255, 0, 200)
{
  map = new ListWidgetIndexMap;

  label = new QLabel();
  label->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);

  connect(listWidget, &QListWidget::currentRowChanged, this, &ListWidgetIndex::currentRowChanged);
}

ListWidgetIndex::~ListWidgetIndex()
{
  disconnect(listWidget, &QListWidget::currentRowChanged, this, &ListWidgetIndex::currentRowChanged);
  setLabel(QStringLiteral());
  delete label;
  delete map;
}

void ListWidgetIndex::resetView()
{
  setLabel(QStringLiteral());

  // Restore visibility
  for(int i = 0; i < listWidget->count(); i++)
    listWidget->item(i)->setHidden(false);

  // Restore saved stylesheets
  for(auto it = savedStyles.begin(); it != savedStyles.end(); ++it)
    it.key()->setStyleSheet(*it);
  savedStyles.clear();

  // Restore background color for list items
  for(int i = 0; i < listWidget->count(); i++)
    listWidget->item(i)->setBackground(QApplication::palette().color(QPalette::Normal, QPalette::Base));
}

void ListWidgetIndex::find(QString text)
{
  // Avoid updates while changing index
  changing = true;

  // Build index and load stopwords on demand
  buildIndex();

  // Clear all hightlights
  resetView();

  if(text.size() >= MIN_STRING_SIZE)
  {
    // Minimum size ok - do search
    searchActive = true;

    // List of page indexes
    QSet<int> foundStackedWidgetIndexes, foundStackedWidgetIndexesText;

    // Widgets matching texts for style change
    QSet<QWidget *> foundWidgets;
    text = text.toUpper();

    // Collect all widgets and page indexes =================================
    // Internal index mapping texts to list indexes and widgets
    for(auto it = map->index.begin(); it != map->index.end(); ++it)
    {
      if(it.key().startsWith(text))
      {
        foundStackedWidgetIndexes.insert(it->getStackedWidgetIndex());

        // Insert index of list for page text matches
        if(it->getObject() == listWidget)
          foundStackedWidgetIndexesText.insert(it->getStackedWidgetIndex());

        // Child widgets
        QWidget *widget = dynamic_cast<QWidget *>(it->getObject());
        if(widget != nullptr && widget != listWidget)
          foundWidgets.insert(widget);
      }
    }

    // Hide all pages not in the list =================================
    for(int i = 0; i < listWidget->count(); i++)
    {
      if(!foundStackedWidgetIndexes.contains(i))
        listWidget->item(i)->setHidden(true);
    }

    // Highlight all page items where text matches  =================================
    for(int i : foundStackedWidgetIndexesText)
      listWidget->item(i)->setBackground(highlightColor);

    // Highlight all widgets in the list =================================
    QString colorName = highlightColor.name(QColor::HexArgb);
    for(QWidget *foundWidget : foundWidgets)
    {
      savedStyles.insert(foundWidget, foundWidget->styleSheet());

      if(dynamic_cast<const QAbstractItemView *>(foundWidget) != nullptr)
        // Border around view
        foundWidget->setStyleSheet(QStringLiteral("border: 3px solid %1").arg(colorName));
      else if(dynamic_cast<const QGroupBox *>(foundWidget) != nullptr)
        // Only title for group boxes
        foundWidget->setStyleSheet(QStringLiteral("QGroupBox::title {background: %1}").arg(colorName));
      else
        // Background for other widgets
        foundWidget->setStyleSheet(QStringLiteral("background: %1").arg(colorName));
    }

    // Set current page to first in list and remember current page index =================================
    if(!foundStackedWidgetIndexes.isEmpty())
    {
      // Get a copy and sort it to get lowest visible index
      QList<int> indexes(foundStackedWidgetIndexes.constBegin(), foundStackedWidgetIndexes.constEnd());
      std::sort(indexes.begin(), indexes.end());

      // Update row to restore later
      if(lastCurrentRow == -1)
        lastCurrentRow = listWidget->currentRow();

      // Select first in list if current is hidden
      if(listWidget->currentItem()->isHidden())
        listWidget->setCurrentRow(indexes.constFirst());
    }
    else
    {
      // Nothing found - hide all items and show message
      for(int i = 0; i < listWidget->count(); i++)
        listWidget->item(i)->setHidden(true);

      setLabel(tr("<b>No options found for \"%1\".</b>").arg(text));
    }
  }
  else if(!text.isEmpty())
  {
    // Text too short - hide all items and show message
    searchActive = true;

    for(int i = 0; i < listWidget->count(); i++)
      listWidget->item(i)->setHidden(true);

    setLabel(tr("<b>Search text \"%1\" too short.</b>").arg(text));
  }
  else if(lastCurrentRow != -1)
  {
    // No text given - restore last page
    listWidget->setCurrentRow(lastCurrentRow);
    lastCurrentRow = -1;
    searchActive = false;
  }

  changing = false;
}

void ListWidgetIndex::setLabel(const QString& text)
{
  if(!text.isEmpty())
  {
    // Show message label if text is not empty
    label->setText(text);
    if(splitterWidget->widget(1) != label)
    {
      // The splitter takes ownership of widget and sets the parent of the replaced widget to null.
      splitterWidget->replaceWidget(1, label);
      splitterWidget->updateGeometry();
    }
  }
  else
  {
    // Hide mesage label and restore previous widget if text is empty
    if(splitterWidget->widget(1) != stackedWidget)
    {
      // The splitter takes ownership of widget and sets the parent of the replaced widget to null.
      splitterWidget->replaceWidget(1, stackedWidget);
      splitterWidget->updateGeometry();
    }
  }
}

void ListWidgetIndex::currentRowChanged(int currentRow)
{
  // Update current if user clicked into the list while searching
  if(!changing && searchActive)
    lastCurrentRow = currentRow;
}

void ListWidgetIndex::reset()
{
  resetView();
  stopwords.clear();
  currentStopwords = nullptr;
  processedObjects.clear();
  map->index.clear();
}

void ListWidgetIndex::buildIndex()
{
  if(map->index.isEmpty())
  {
    processedObjects.clear();

    loadStopwords();

    // Select stopword list based on UI language
    QString lang = QLocale().uiLanguages().value(0).section('-', 0, 0);

    if(!stopwords.contains(lang))
      // Fall back to English
      lang = QStringLiteral("en");
    currentStopwords = &stopwords[lang];

    // Search through app stacked widget pages
    for(int i = 0; i < stackedWidget->count(); i++)
    {
      // Add stacked widget to index
      QWidget *widget = stackedWidget->widget(i);
      insertPhrases(cleanTextList({listWidget->item(i)->text()}), i, listWidget);

      // Recurse to all page children
      const QObjectList& children = widget->children();
      for(QObject *object : children)
        processObjects(i, object);
    }

#ifdef DEBUG_INFORMATION_LISTINDEX

    qDebug() << Q_FUNC_INFO;
    bool dummy;
    for(auto it = map->index.begin(); it != map->index.end(); ++it)
    {
      qDebug() << "ListWidgetIndex" << it.key()
               << stackedWidget->widget(it->getStackedWidgetIndex())->objectName()
               << it->getObject()->objectName() << atools::elideTextShort(widgetText(it->getObject(), dummy), 20);
    }

#endif
  }
}

// . {
// .   "en": [
// .   "'ll",
// .   "'tis",
// .   "'twas",
void ListWidgetIndex::loadStopwords()
{
  if(stopwords.isEmpty())
  {
    QFile file(STOPWORDS_FILE);
    if(file.open(QIODevice::ReadOnly))
    {
      // Read probably gzipped file
      QJsonParseError error;
      QJsonDocument doc = QJsonDocument::fromJson(zip::gzipDecompressIf(file.readAll(), Q_FUNC_INFO), &error);

      if(error.error == QJsonParseError::NoError)
      {
        // Top level object
        QJsonObject object = doc.object();

        // Keys are languages
        const QStringList languages = object.keys();
        for(const QString& language : languages)
        {
          // Get words for one language
          QSet<QString> words;
          const QJsonArray array = object.value(language).toArray();
          for(const QJsonValue& value : array)
            words.insert(value.toString().toUpper());

          // Insert array for language
          stopwords.insert(language, words);
        }
      }
      else
        qWarning() << Q_FUNC_INFO << "Error reading" << STOPWORDS_FILE << error.errorString() << "at offset" << error.offset;

      file.close();
    }
    else
      qWarning() << Q_FUNC_INFO << "Cannot open stopwords file" << STOPWORDS_FILE;
  }
}

void ListWidgetIndex::processObjects(int pageIndex, QObject *object)
{
  if(object == nullptr)
    return;

  // Avoid endless recursion by tracking visited widgets
  if(processedObjects.contains(object))
    return;

  processedObjects.insert(object);

  QWidget *widget = dynamic_cast<QWidget *>(object);
  if(widget != nullptr)
  {
    bool recurse;
    // Insert texts for this widget - stop recursion for leaves in the tree
    insertPhrases(widgetText(object, recurse), pageIndex, widget);

    if(recurse)
    {
      const QObjectList& children = object->children();
      for(QObject *child : children)
        processObjects(pageIndex, child);
    }
  }
  else
  {
    // Recurse into layous which have no text
    const QLayout *layout = dynamic_cast<const QLayout *>(object);
    if(layout != nullptr)
    {
      for(int i = 0; i < layout->count(); ++i)
        processObjects(pageIndex, layout->itemAt(i)->widget());
    }
  }
}

QStringList ListWidgetIndex::widgetText(const QObject *object, bool& recurse)
{
  QStringList widgetTexts;
  recurse = false;

  if(const QAbstractButton *button = dynamic_cast<const QAbstractButton *>(object))
    widgetTexts.append(button->text());
  else if(const QLineEdit *lineEdit = dynamic_cast<const QLineEdit *>(object))
    widgetTexts.append(lineEdit->placeholderText());
  else if(const QLabel *label = dynamic_cast<const QLabel *>(object))
    widgetTexts.append(label->text());
  else if(const QTextEdit *textEdit = dynamic_cast<const QTextEdit *>(object))
    widgetTexts.append(textEdit->placeholderText());
  else if(const QComboBox *comboBox = dynamic_cast<const QComboBox *>(object))
  {
    // Get all texts from combo box
    for(int i = 0; i < comboBox->count(); i++)
      widgetTexts.append(comboBox->itemText(i));
  }
  else if(const QAbstractItemView *view = dynamic_cast<const QAbstractItemView *>(object))
  {
    // Get header and texts in first column from view
    const QAbstractItemModel *model = view->model();
    if(model->columnCount() > 0)
    {
      for(int col = 0; col < model->columnCount(); col++)
      {
        // Add header text
        widgetTexts.append(model->headerData(col, Qt::Horizontal).toString());

        // Add data
        for(int row = 0; row < model->rowCount(); row++)
          widgetTexts.append(model->data(model->index(row, col)).toString());
      }
    }
  }
  else if(const QGroupBox *groupBox = dynamic_cast<const QGroupBox *>(object))
  {
    // Get title but continue recursion
    recurse = true;
    widgetTexts.append(groupBox->title());
  }
  else if(const QTabBar *tabBar = dynamic_cast<const QTabBar *>(object))
  {
    // Tab texts
    for(int i = 0; i < tabBar->count(); i++)
      widgetTexts.append(tabBar->tabText(i));
  }
  else if(dynamic_cast<const QWidget *>(object) != nullptr)
    // No title or text but child widgets - dive deeper
    recurse = true;

  widgetTexts = cleanTextList(widgetTexts);

  return widgetTexts;
}

QStringList ListWidgetIndex::cleanTextList(const QStringList& texts)
{
  QStringList cleanTexts;
  if(!texts.isEmpty())
  {
    for(const QString& text : texts)
      cleanTexts.append(cleanText(text).split(' '));

    cleanTexts.removeAll(QStringLiteral());
  }
  return cleanTexts;
}

QString ListWidgetIndex::cleanText(const QString& text)
{
  return QTextDocumentFragment::fromHtml(text).toPlainText().
         remove('&').replace('\n', ' ').replace('.', ' ').replace(',', ' ').replace('/', ' ').replace('\"', ' ').
         replace('(', ' ').replace(')', ' ').replace("...", " ").replace("->", " ").replace(" - ", " ").
         simplified().toUpper();
}

void ListWidgetIndex::insertPhrases(const QStringList& texts, int pageIndex, QObject *object)
{
  if(!texts.isEmpty())
  {
    // Loop for phrases of single word until four words
    for(int numWords = 0; numWords <= 3; numWords++)
    {
      // Loop through text collecting up to four words per iteration
      for(int i = 0; i < texts.size() - numWords; i++)
      {
        bool minSizeOk = false, stopwordOk = false;
        QStringList phrase;

        // Check if at least one word is not in the stop word list and has required length
        for(int j = i; j <= i + numWords; j++)
        {
          const QString& str = texts.at(j);
          if(!minSizeOk && str.size() >= MIN_STRING_SIZE)
            minSizeOk = true;

          if(!stopwordOk && !currentStopwords->contains(str))
            stopwordOk = true;

          phrase.append(str);
        }

        if(minSizeOk && stopwordOk)
        {
          // Add phrase if key/value is not already present
          ListWidgetIndexEntry entry(pageIndex, object);
          QString phraseStr = phrase.join(' ');

          if(!map->index.contains(phraseStr, entry))
            map->index.insert(phraseStr, entry);
        }
      }
    }
  }
}

} // namespace gui
} // namespace atools

Q_DECLARE_TYPEINFO(atools::gui::ListWidgetIndexEntry, Q_PRIMITIVE_TYPE);
