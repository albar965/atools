/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#include <QColor>
#include <QCoreApplication>
#include <QLocale>
#include <QSize>

namespace atools {
namespace util {

namespace html {
/* HTML formatting flags for text */
enum Flag
{
  NONE = 0,

  /* HTML formatting attributes */
  BOLD = 1 << 0,
  ITALIC = 1 << 1,
  UNDERLINE = 1 << 2,
  STRIKEOUT = 1 << 3,
  SUBSCRIPT = 1 << 4,
  SUPERSCRIPT = 1 << 5,
  SMALL = 1 << 6,
  BIG = 1 << 7,
  CODE = 1 << 8,

  NOBR = 1 << 9, /* HTML no break */

  LINK_NO_UL = 1 << 10, /* Do not underline links */
  NO_ENTITIES = 1 << 11, /* Do not convert entities */
  ALIGN_RIGHT = 1 << 12, /* Only for table data */
  AUTOLINK = 1 << 13, /* Automatically create links from http:// and https:// in text */
  REPLACE_CRLF = 1 << 14 /* Replace carriage return and linefeed with <br/> */
};

Q_DECLARE_FLAGS(Flags, Flag);
Q_DECLARE_OPERATORS_FOR_FLAGS(html::Flags);
}

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
  ~HtmlBuilder();

  const QString& getHtml() const
  {
    return htmlText;
  }

  HtmlBuilder& operator=(const atools::util::HtmlBuilder& other);

  /* Clears this instance except settings */
  HtmlBuilder& clear();

  /* Returns a clean copy of this instance */
  HtmlBuilder cleared() const;

  /* Appends raw data without conversion */
  atools::util::HtmlBuilder& append(const atools::util::HtmlBuilder& other);

  /* Appends raw data without conversion */
  atools::util::HtmlBuilder& append(const QString& other);

  /* Error message. Bold white text on red background. */
  HtmlBuilder& error(const QString& str);
  static QString errorMessage(const QString& str);

  /* Warning message. Orange bold text. */
  HtmlBuilder& warning(const QString& str);
  static QString warningMessage(const QString& str);

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
  HtmlBuilder& textBar(int lenght = 10, html::Flags flags = html::NONE, QColor color = QColor());

  /* Add link (anchor/href) */
  HtmlBuilder& a(const QString& text, const QString& href,
                 html::Flags flags = html::NONE, QColor color = QColor());

  /* Add image */
  HtmlBuilder& img(const QString& src, const QString& alt = QString(),
                   const QString& style = QString(), QSize size = QSize());

  /* Add inline base64 encoded image */
  HtmlBuilder& img(const QIcon& icon, const QString& alt = QString(),
                   const QString& style = QString(), QSize size = QSize());

  /* List functions */
  HtmlBuilder& ol();
  HtmlBuilder& olEnd();
  HtmlBuilder& ul();
  HtmlBuilder& ulEnd();
  HtmlBuilder& li(const QString& str, html::Flags flags = html::NONE, QColor color = QColor());

  /* Add text with color and attributes */
  HtmlBuilder& text(const QString& str, html::Flags flags = html::NONE, QColor color = QColor());

  /* Add other`s HTML not escaping entities */
  HtmlBuilder& textHtml(const atools::util::HtmlBuilder& other);

  /* Add string enclosed in a paragraph */
  HtmlBuilder& p(const QString& str, html::Flags flags = html::NONE, QColor color = QColor());

  /* Begin paragraph */
  HtmlBuilder& p();

  /* End paragraph */
  HtmlBuilder& pEnd();

  /* Add preformatted text */
  HtmlBuilder& pre(const QString& str, html::Flags flags = html::NONE, QColor color = QColor());

  /* Add break */
  HtmlBuilder& br();

  /* Add text and break */
  HtmlBuilder& textBr(const QString& str);

  /* Add break and text */
  HtmlBuilder& brText(const QString& str);

  /* Add non breaking space */
  HtmlBuilder& nbsp();

  /* Add HTML header */
  HtmlBuilder& h(int level, const QString& str, html::Flags flags = html::NONE,
                 QColor color = QColor(), const QString& id = QString());
  HtmlBuilder& h1(const QString& str, html::Flags flags = html::NONE,
                  QColor color = QColor(), const QString& id = QString());
  HtmlBuilder& h2(const QString& str, html::Flags flags = html::NONE,
                  QColor color = QColor(), const QString& id = QString());
  HtmlBuilder& h3(const QString& str, html::Flags flags = html::NONE,
                  QColor color = QColor(), const QString& id = QString());
  HtmlBuilder& h4(const QString& str, html::Flags flags = html::NONE,
                  QColor color = QColor(), const QString& id = QString());
  HtmlBuilder& h5(const QString& str, html::Flags flags = html::NONE,
                  QColor color = QColor(), const QString& id = QString());

  /* Add table and table body */
  HtmlBuilder& table(int border = 0, int padding = 2, int spacing = 0, int widthPercent = 0, QColor bgcolor = QColor());
  HtmlBuilder& tableAtts(const QHash<QString, QString>& attributes);
  HtmlBuilder& tableEnd();

  /* all row2 methods add two rows to a table.
   * The first one contains bold text (like a heading) the second one contains text according to attributes.
   * Text background may alternate depending on configuration */
  HtmlBuilder& row2(const QString& name, const atools::util::HtmlBuilder& value,
                    html::Flags flags = html::NONE, QColor color = QColor());
  HtmlBuilder& row2(const QString& name, const QString& value = QString(),
                    html::Flags flags = html::NONE, QColor color = QColor());
  HtmlBuilder& row2(const QString& name, float value, int precision = -1,
                    html::Flags flags = html::NONE, QColor color = QColor());
  HtmlBuilder& row2(const QString& name, double value, int precision = -1,
                    html::Flags flags = html::NONE, QColor color = QColor());
  HtmlBuilder& row2(const QString& name, int value,
                    html::Flags flags = html::NONE, QColor color = QColor());
  HtmlBuilder& row2Var(const QString& name, const QVariant& value,
                       html::Flags flags = html::NONE, QColor color = QColor());

  /* Adds row if string is not empty*/
  HtmlBuilder& row2If(const QString& name, const QString& value, html::Flags flags = html::NONE,
                      QColor color = QColor());

  /* Adds row if value > 0 */
  HtmlBuilder& row2If(const QString& name, int value, html::Flags flags = html::NONE, QColor color = QColor());

  /* Adds row if value is valid and not null */
  HtmlBuilder& row2IfVar(const QString& name, const QVariant& value, html::Flags flags = html::NONE,
                         QColor color = QColor());

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

  HtmlBuilder& th(const QString& str, html::Flags flags = html::NONE, QColor color = QColor());

  /* Document begin and end */
  HtmlBuilder& doc(const QString& title = QString(), const QString& css = QString());
  HtmlBuilder& docEnd();

  /*
   * Check length of text and add a string (i.e. "...") if too long.
   * @param maxLines maximum lines
   * @param msg text to add if too long
   * @return true if text is too long
   */
  bool checklength(int maxLines, const QString& msg);
  bool checklengthTextBar(int maxLines, const QString& msg, int lenght);

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

private:
  /* Select alternating entries based on the index from the string list */
  const QString& alt(const QStringList& list) const;
  QString asText(QString str, html::Flags flags, QColor color);
  void initColors(const QColor& rowColor, const QColor& rowColorAlt);

  QString rowBackColorStr, rowBackColorAltStr, tableRowHeader;
  QColor rowBackColor, rowBackColorAlt;
  QStringList tableRow, tableRowAlignRight, tableRowBegin;

  int tableIndex = 0, defaultPrecision = 0, numLines = 0;
  QString htmlText;

  QLocale locale;
  QLocale::FormatType dateFormat = QLocale::ShortFormat;
  bool hasBackColor = false;
};

} // namespace util
} // namespace atools

#endif // ATOOLS_UTIL_HTMLBUILDER_H
