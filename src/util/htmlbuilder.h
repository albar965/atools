/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_UTIL_HTMLBUILDER_H
#define ATOOLS_UTIL_HTMLBUILDER_H

#include "util/htmlbuilderflags.h"

#include <QBitArray>
#include <QColor>
#include <QCoreApplication>
#include <QLocale>
#include <QSize>
#include <QDebug>

namespace atools {
namespace util {

/*
 * Text base HTML builder class that does not use any XML frameworks and does no validation.
 * Useful for generating tootips or QTextEdit HTML text.
 *
 * Has functions for alternating row colors in tables.
 */
class HtmlBuilder
{
  Q_DECLARE_TR_FUNCTIONS(HtmlBuilder)

public:
  /*
   * @param hasBackColor if true a alternating background color taken from the
   * system palette (gray/lightgray) is used for tables.
   */
  HtmlBuilder(bool backgroundColorUsed = false);

  /* Pass alternating table row colors in */
  HtmlBuilder(const QColor& rowColor, const QColor& rowColorAlt);

  HtmlBuilder(const atools::util::HtmlBuilder& other);

  const QString& getHtml() const
  {
    return htmlText;
  }

  HtmlBuilder& operator=(const atools::util::HtmlBuilder& other);

  /* Joins the list of builders using <br> or <p> */
  static QString joinBr(std::initializer_list<atools::util::HtmlBuilder> builders);
  static QString joinP(std::initializer_list<atools::util::HtmlBuilder> builders);
  static QString joinBr(QStringList strings);
  static QString joinP(QStringList strings);

  /* Clears this instance except settings */
  HtmlBuilder& clear();

  /* Returns a clean copy of this instance */
  HtmlBuilder cleared() const;

  /* Sets a marked position in the stream. -1 is unset. */
  HtmlBuilder& mark(int markParam);

  /* Mark current position (i.e. end of HTML string for rewind() or tableEndIf(). */
  HtmlBuilder& mark();

  /* Reset mark */
  HtmlBuilder& clearMark();

  /* Deletes all back to the marked position. Does nothing if mark is not set. */
  HtmlBuilder& rewind();

  /* Returns mark index in string or -1 if unset */
  int getMark() const;

  /* Appends raw data without conversion */
  atools::util::HtmlBuilder& append(const atools::util::HtmlBuilder& other);

  /* Appends raw data without conversion */
  atools::util::HtmlBuilder& append(const QString& other);
  atools::util::HtmlBuilder& append(const char *other);

  /* Default flags for error, warning and note messages. Combine these with any other flags if needed. */
  const static html::Flags MSG_FLAGS; /* html::BOLD | html::NO_ENTITIES */

  /* Error message. Bold white text on red background. */
  HtmlBuilder& error(const QString& str, html::Flags flags = MSG_FLAGS);
  static QString errorMessage(const QString& str, html::Flags flags = MSG_FLAGS);
  static QString errorMessage(const QStringList& stringList, const QString& separator = "<br/>", html::Flags flags = MSG_FLAGS);

  /* Warning message. Orange bold text. */
  HtmlBuilder& warning(const QString& str, html::Flags flags = MSG_FLAGS);
  static QString warningMessage(const QString& str, html::Flags flags = MSG_FLAGS);
  static QString warningMessage(const QStringList& stringList, const QString& separator = "<br/>", html::Flags flags = MSG_FLAGS);

  /* Hint message. Dark green bold text. */
  HtmlBuilder& note(const QString& str, html::Flags flags = MSG_FLAGS);
  static QString noteMessage(const QString& str, html::Flags flags = MSG_FLAGS);
  static QString noteMessage(const QStringList& stringList, const QString& separator = "<br/>", html::Flags flags = MSG_FLAGS);

  /* Message.  */
  HtmlBuilder& message(const QString& str, html::Flags flags = html::NONE,
                       QColor foreground = QColor(), QColor background = QColor());
  static QString textMessage(const QString& str, html::Flags flags = html::NONE,
                             QColor foreground = QColor(), QColor background = QColor());

  static QString textMessage(const QStringList& stringList, html::Flags flags = html::NONE,
                             QColor foreground = QColor(), QColor background = QColor(),
                             const QString& separator = "<br/>");

  /* Add bold text */
  HtmlBuilder& b(const QString& str);
  HtmlBuilder& b();
  HtmlBuilder& bEnd();

  /* Add italic text */
  HtmlBuilder& i(const QString& str);
  HtmlBuilder& i();
  HtmlBuilder& iEnd();

  /* Add underlined text */
  HtmlBuilder& u(const QString& str);
  HtmlBuilder& u();
  HtmlBuilder& uEnd();

  /* Add subscripted text */
  HtmlBuilder& sub(const QString& str);
  HtmlBuilder& sub();
  HtmlBuilder& subEnd();

  /* Add superscripted text */
  HtmlBuilder& sup(const QString& str);
  HtmlBuilder& sup();
  HtmlBuilder& supEnd();

  /* Add small text */
  HtmlBuilder& small(const QString& str);
  HtmlBuilder& small();
  HtmlBuilder& smallEnd();

  /* Add big text */
  HtmlBuilder& big(const QString& str);
  HtmlBuilder& big();
  HtmlBuilder& bigEnd();

  /* Add code / monospace */
  HtmlBuilder& code(const QString& str);
  HtmlBuilder& code();
  HtmlBuilder& codeEnd();

  /* Add text with no break attribute */
  HtmlBuilder& nobr(const QString& str);

  /* Add horizontal ruler */
  HtmlBuilder& hr(int size = 1, int widthPercent = 100);

  /* Ruler made of a number of dashes and br */
  HtmlBuilder& textBar(int length = 10, html::Flags flags = html::NONE, QColor color = QColor());

  /* Add link (anchor/href). Only allowed style is LINK_NO_UL */
  HtmlBuilder& a(const QString& text, const QString& href, html::Flags flags = html::NONE, QColor color = QColor());

  /* Returns anchor with unchanged URL and elided text. Creates a "<div>" if a style NOBR_WHITESPACE or LINK_NO_UL is given. */
  static QString aUrl(const QString& text, const QString& href, html::Flags flags = html::NONE,
                      QColor color = QColor(), int elideText = 60);

  /* Returns anchor with file absolute path URL in href and elided text with absolute native file path.
   * Creates a "<div>" if a style NOBR_WHITESPACE or LINK_NO_UL is given. */
  static QString aFilePath(const QString& filepath, html::Flags flags = html::NONE, QColor color = QColor(), int elideText = 60);

  /* Returns anchor with file absolute path URL in href and elided text with absolute native file name.
   * Creates a "<div>" if a style NOBR_WHITESPACE or LINK_NO_UL is given. */
  static QString aFileName(const QString& filepath, html::Flags flags = html::NONE, QColor color = QColor(), int elideText = 60);

  /* Add image */
  HtmlBuilder& img(const QString& src, const QString& alt = QString(), const QString& style = QString(), QSize size = QSize());

  /* Add inline base64 encoded image */
  HtmlBuilder& img(const QIcon& icon, const QString& alt = QString(), const QString& style = QString(), QSize size = QSize());

  /* List functions */
  HtmlBuilder& ol();
  HtmlBuilder& olEnd();
  HtmlBuilder& ul();
  HtmlBuilder& ulEnd();
  HtmlBuilder& li(const QString& str, html::Flags flags = html::NONE, QColor color = QColor());

  /* Add text with color and attributes */
  HtmlBuilder& text(const QString& str, html::Flags flags = html::NONE, QColor color = QColor());

  /* Builds a text string (not paragraph) using the given flags */
  static QString textStr(const QString& str, html::Flags flags = html::NONE, QColor color = QColor());

  /* Add other`s HTML not escaping entities */
  HtmlBuilder& textHtml(const atools::util::HtmlBuilder& other);

  /* Add string enclosed in a paragraph */
  HtmlBuilder& p(const QString& str, html::Flags flags = html::NONE, QColor color = QColor());

  /* Begin paragraph */
  HtmlBuilder& p(html::Flags flags = html::NONE);

  /* End paragraph */
  HtmlBuilder& pEnd();

  /* Add preformatted text */
  HtmlBuilder& pre(const QString& str, html::Flags flags = html::NONE, QColor color = QColor());
  HtmlBuilder& pre();
  HtmlBuilder& preEnd();

  /* Add break */
  HtmlBuilder& br();

  /* Add text and break */
  HtmlBuilder& textBr(const QString& str, html::Flags flags = html::NONE, QColor color = QColor());

  /* Add break and text */
  HtmlBuilder& brText(const QString& str, html::Flags flags = html::NONE, QColor color = QColor());

  /* Add non breaking space */
  HtmlBuilder& nbsp();

  /* Add HTML header. Does NOT add closing tag. */
  HtmlBuilder& h(int level, const QString& str, html::Flags flags = html::NONE, QColor color = QColor(), const QString& id = QString());
  HtmlBuilder& hEnd(int level);

  /* Add HTML header. Adds closing tag. */
  HtmlBuilder& h1(const QString& str, html::Flags flags = html::NONE, QColor color = QColor(), const QString& id = QString());
  HtmlBuilder& h2(const QString& str, html::Flags flags = html::NONE, QColor color = QColor(), const QString& id = QString());
  HtmlBuilder& h3(const QString& str, html::Flags flags = html::NONE, QColor color = QColor(), const QString& id = QString());
  HtmlBuilder& h4(const QString& str, html::Flags flags = html::NONE, QColor color = QColor(), const QString& id = QString());
  HtmlBuilder& h5(const QString& str, html::Flags flags = html::NONE, QColor color = QColor(), const QString& id = QString());

  /* Add table and table body */
  HtmlBuilder& table(int border = 0, int padding = 2, int spacing = 0, int widthPercent = 0, QColor bgcolor = QColor(),
                     QColor bordercolor = QColor());

  /* Sets a stream mark and opens a table for use wit htableEndIf(). */
  HtmlBuilder& tableIf(int border = 0, int padding = 2, int spacing = 0, int widthPercent = 0, QColor bgcolor = QColor(),
                       QColor bordercolor = QColor());
  HtmlBuilder& tableAtts(const QHash<QString, QString>& attributes);

  HtmlBuilder& tableEnd();

  /* Remove all content to the last set mark if table is has no rows. Mark can be set with tableIf() or mark() */
  HtmlBuilder& tableEndIf();

  /* Number increased for each row in a table an reset if table is closed. Reset when creating a new table. */
  int getTableRows() const
  {
    return tableRowsCur;
  }

  /* true if table has no rows not including header */
  bool isTableEmpty() const
  {
    return tableRowsCur == 0;
  }

  /* all row2 methods add two rows to a table.
   * The first one contains bold text (like a heading) the second one contains text according to attributes.
   * Text background may alternate depending on configuration */
  HtmlBuilder& row2(const QString& name, const atools::util::HtmlBuilder& value, html::Flags flags = html::NONE, QColor color = QColor());
  HtmlBuilder& row2(const QString& name, const QString& value = QString(), html::Flags flags = html::NONE, QColor color = QColor());
  HtmlBuilder& row2(const QString& name, float value, int precision = -1, html::Flags flags = html::NONE, QColor color = QColor());
  HtmlBuilder& row2(const QString& name, double value, int precision = -1, html::Flags flags = html::NONE, QColor color = QColor());
  HtmlBuilder& row2(const QString& name, int value, html::Flags flags = html::NONE, QColor color = QColor());
  HtmlBuilder& row2Var(const QString& name, const QVariant& value, html::Flags flags = html::NONE, QColor color = QColor());

  /* Set alignment globally for the value colums for all row2 methods */
  HtmlBuilder& row2AlignRight(bool alignRight = true);

  /* Adds row if string is not empty*/
  HtmlBuilder& row2If(const QString& name, const QString& value, html::Flags flags = html::NONE, QColor color = QColor());

  /* Adds row if value > 0 */
  HtmlBuilder& row2If(const QString& name, int value, html::Flags flags = html::NONE, QColor color = QColor());

  /* Adds row if value is valid and not null */
  HtmlBuilder& row2IfVar(const QString& name, const QVariant& value, html::Flags flags = html::NONE, QColor color = QColor());

  /* Display two column row with value as warning or error */
  HtmlBuilder& row2Warning(const QString& name, const QString& value, html::Flags flags = html::NONE);
  HtmlBuilder& row2Error(const QString& name, const QString& value, html::Flags flags = html::NONE);

  /* Add/end table row Text background may alternate depending on configuration */
  HtmlBuilder& tr(QColor backgroundColor);

  /* Table row without alternating background color */
  HtmlBuilder& tr();
  HtmlBuilder& trEnd();

  /* Add table data */
  HtmlBuilder& td();
  HtmlBuilder& tdW(int widthPercent);
  HtmlBuilder& td(const QString& str, html::Flags flags = html::NONE, QColor color = QColor());
  HtmlBuilder& tdF(html::Flags flags);
  HtmlBuilder& tdAtts(const QHash<QString, QString>& attributes);
  HtmlBuilder& tdEnd();

  HtmlBuilder& th(const QString& str, html::Flags flags = html::NONE, QColor color = QColor(), int colspan = -1);

  /* Document begin and end */
  HtmlBuilder& doc(const QString& title = QString(), const QString& css = QString(),
                   const QString& bodyStyle = QString(), const QStringList& headerLines = QStringList());
  HtmlBuilder& docEnd();

  /*
   * Check length of text and add a string (i.e. "...") if too long.
   * @param maxLines maximum lines
   * @param msg text to add if too long
   * @return true if text is too long
   */
  bool checklength(int maxLines, const QString& msg);
  bool checklengthTextBar(int maxLines, const QString& msg, int length);

  bool isEmpty() const
  {
    return htmlText.isEmpty();
  }

  /* Date format for row2Var */
  QLocale::FormatType getDateFormat() const
  {
    return dateFormat;
  }

  void setDateFormat(QLocale::FormatType value)
  {
    dateFormat = value;
  }

  /* Numeric precision for row2Var */
  int getPrecision() const
  {
    return defaultPrecision;
  }

  void setPrecision(int value)
  {
    defaultPrecision = value;
  }

  /*
   * @return estimated number of lines in the document
   */
  int getNumLines() const
  {
    return numLines;
  }

  /*
   * Create the href content with an base64 encoded image.
   * @return "data:image/png;base64, AKLDSAKLJKL"
   */
  static QString getEncodedImageHref(const QIcon& icon, QSize imageSize);

  static QString toEntities(const QString& src);

  bool hasBackgroundColor() const
  {
    return hasBackColor;
  }

  const QColor getRowBackColor() const
  {
    return rowBackColor;
  }

  const QColor& getRowBackColorAlt() const
  {
    return rowBackColorAlt;
  }

  /* Set the current id flag for the stream. All row2 output is skipped if the id flag is not included in setIds().
   * The enum does not need to use bit flags. */
  template<typename TYPE>
  HtmlBuilder& id(TYPE idEnum)
  {
    int num = static_cast<int>(idEnum);
    if(num <= MAX_ID)
      currentId = num;
    else
      qWarning() << Q_FUNC_INFO << "id" << num << "too large";

    return *this;
  }

  /* true if id is set for the given enum value */
  template<typename TYPE>
  bool isIdSet(TYPE idEnum)
  {
    int num = static_cast<int>(idEnum);
    if(num <= MAX_ID)
      return idBits.at(num);
    else
      qWarning() << Q_FUNC_INFO << "id" << num << "too large";
    return false;
  }

  /* Clears the currently set id value */
  HtmlBuilder& clearId()
  {
    currentId = -1;
    return *this;
  }

  /* Sets a list of enums which decide if row2 table output is skipped after id() or not. Maximum enum value if MAX_ID. */
  template<typename TYPE>
  void setIds(const QVector<TYPE>& idEnums);

  const static int MAX_ID = 512;

  /* Set bitfield directly without using setIds() */
  void setIdBits(const QBitArray& value);

  /* Colors for error and warning messages */
  const static QColor COLOR_FOREGROUND_ERROR;
  const static QColor COLOR_BACKGROUND_ERROR;
  const static QColor COLOR_FOREGROUND_WARNING;
  const static QColor COLOR_BACKGROUND_WARNING;

private:
  /* Select alternating entries based on the index from the string list */
  const QString& alt(const QStringList& list) const;
  static QString asText(QString str, html::Flags flags, QColor foreground, QColor background = QColor());

  void initColors(const QColor& rowColor, const QColor& rowColorAlt);

  bool isId() const
  {
    return currentId == -1 || idBits.testBit(currentId);
  }

  QString rowBackColorStr, rowBackColorAltStr, tableRowHeader;
  QColor rowBackColor, rowBackColorAlt;
  QStringList tableRow, tableRowAlignRight, tableRowBegin;

  int defaultPrecision = 0, numLines = 0;
  QString htmlText;

  QLocale locale;
  QLocale::FormatType dateFormat = QLocale::ShortFormat;
  bool hasBackColor = false, row2AlignRightFlag = false;
  int markIndex = -1, /* Last marked stream position */
      tableRowsCur = 0; /* Number of rows in currently opened table */

  QBitArray idBits;
  int currentId = -1;
};

// =================================================
template<typename TYPE>
void HtmlBuilder::setIds(const QVector<TYPE>& idEnums)
{
  idBits.fill(false);
  for(TYPE id : idEnums)
  {
    int num = static_cast<int>(id);
    if(num <= MAX_ID)
      idBits.setBit(num, true);
    else
      qWarning() << Q_FUNC_INFO << "id" << num << "too large";
  }
}

} // namespace util
} // namespace atools

#endif // ATOOLS_UTIL_HTMLBUILDER_H
