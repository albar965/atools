/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#include <QLocale>
#include <QCoreApplication>
#include <QColor>
#include <QSize>

namespace atools {
namespace util {

namespace html {
/* HTML formatting flags for text */
enum Flag
{
  NONE = 0x0000,
  BOLD = 0x0001,
  ITALIC = 0x0002,
  UNDERLINE = 0x0004,
  STRIKEOUT = 0x0008,
  SUBSCRIPT = 0x0010,
  SUPERSCRIPT = 0x0020,
  SMALL = 0x0040,
  BIG = 0x0080,
  NOBR = 0x0100,
  ALIGN_RIGHT = 0x1000 // Only for table data
};

Q_DECLARE_FLAGS(Flags, Flag);
Q_DECLARE_OPERATORS_FOR_FLAGS(html::Flags);
}

/*
 * Text base HTML builder class that does not use any XML frameworks and does no validation.
 * Useful for generating tootips or QTextEdit HTML text.
 */
class HtmlBuilder
{
  Q_DECLARE_TR_FUNCTIONS(HtmlBuilder)

public:
  /*
   * @param hasBackColor if true a alternating background color (gray/lightgray) is used for tables.
   */
  HtmlBuilder(bool hasBackColor);
  ~HtmlBuilder();

  const QString& getHtml() const
  {
    return htmlText;
  }

  /* Appends raw data without conversion */
  atools::util::HtmlBuilder& append(const atools::util::HtmlBuilder& other);

  /* Appends raw data without conversion */
  atools::util::HtmlBuilder& append(const QString& other);

  /* Add bold text */
  HtmlBuilder& b(const QString& str);

  /* Add italic text */
  HtmlBuilder& i(const QString& str);

  /* Add underlined text */
  HtmlBuilder& u(const QString& str);

  /* Add subscripted text */
  HtmlBuilder& sub(const QString& str);

  /* Add superscripted text */
  HtmlBuilder& sup(const QString& str);

  /* Add small text */
  HtmlBuilder& small(const QString& str);

  /* Add big text */
  HtmlBuilder& big(const QString& str);

  /* Add text with no break attribute */
  HtmlBuilder& nobr(const QString& str);

  /* Add horizontal ruler */
  HtmlBuilder& hr(int size = 1, int widthPercent = 100);

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
  HtmlBuilder& table();
  HtmlBuilder& tableWithAtts(const QHash<QString, QString>& attributes);
  HtmlBuilder& tableEnd();

  /* all row2 methods add two rows to a table.
   * The first one contains bold text (like a heading) the second one contains text according to attributes.
   * Text background may alternate depending on configuration */
  HtmlBuilder& row2(const QString& name, const QString& value = QString(),
                    html::Flags flags = html::BOLD, QColor color = QColor());
  HtmlBuilder& row2(const QString& name, float value, int precision = -1,
                    html::Flags flags = html::BOLD, QColor color = QColor());
  HtmlBuilder& row2(const QString& name, double value, int precision = -1,
                    html::Flags flags = html::BOLD, QColor color = QColor());
  HtmlBuilder& row2(const QString& name, int value,
                    html::Flags flags = html::BOLD, QColor color = QColor());
  HtmlBuilder& row2Var(const QString& name, const QVariant& value,
                       html::Flags flags = html::BOLD, QColor color = QColor());

  /* Add/end table row Text background may alternate depending on configuration */
  HtmlBuilder& tr(QColor backgroundColor = QColor());
  HtmlBuilder& trEnd();

  /* Add table data */
  HtmlBuilder& td();
  HtmlBuilder& td(int widthPercent);
  HtmlBuilder& tdEnd();
  HtmlBuilder& td(const QString& str, html::Flags flags = html::NONE, QColor color = QColor());
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

  bool isEmpty() const
  {
    return htmlText.isEmpty();
  }

  HtmlBuilder& clear();

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

  bool isTruncated() const
  {
    return truncated;
  }

  void setTruncated(bool value)
  {
    truncated = value;
  }

  /*
   * Create the href content with an base64 encoded image.
   * @return "data:image/png;base64, AKLDSAKLJKL"
   */
  static QString getEncodedImageHref(const QIcon& icon, QSize imageSize);

  static QString toEntities(const QString& src);

private:
  /* Select alternating entries based on the index from the string list */
  const QString& alt(const QStringList& list) const;
  QString asText(const QString& str, html::Flags flags, QColor color);

  QString rowBackColor, rowBackColorAlt, tableRowHeader;
  QStringList tableRow, tableRowAlignRight, tableRowBegin;

  int tableIndex = 0, defaultPrecision = 0, numLines = 0;
  QString htmlText;

  QLocale locale;
  QLocale::FormatType dateFormat = QLocale::ShortFormat;
  bool truncated = false;
};

} // namespace util
} // namespace atools

#endif // ATOOLS_UTIL_HTMLBUILDER_H
