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

#include "util/htmlbuilder.h"

#include <QDebug>
#include <QApplication>
#include <QPalette>
#include <QDateTime>
#include <QBuffer>
#include <QIcon>
#include <QRegularExpression>

namespace atools {
namespace util {

// Matches "http://blah" and "https://www.example.com/blah" links
static const QRegularExpression LINK_REGEXP(
  "\\b((http[s]?|ftp|file)://[a-zA-Z0-9\\./:_\\?\\&=\\-\\$\\+\\!\\*'\\(\\),;%#\\[\\]@]+)\\b");

HtmlBuilder::HtmlBuilder(const QColor& rowColor, const QColor& rowColorAlt)
  : hasBackColor(true)
{
  initColors(rowColor, rowColorAlt);
}

HtmlBuilder::HtmlBuilder(bool backgroundColorUsed)
  : hasBackColor(backgroundColorUsed)
{
  if(hasBackColor)
    // Create darker colors dynamically from default palette
    initColors(QApplication::palette().color(QPalette::Active, QPalette::Base).darker(105),
               QApplication::palette().color(QPalette::Active, QPalette::AlternateBase).darker(105));
  else
    initColors(QColor(Qt::white), QColor(Qt::white).darker(120));
}

HtmlBuilder::HtmlBuilder(const atools::util::HtmlBuilder& other)
{
  this->operator=(other);

}

HtmlBuilder::~HtmlBuilder()
{

}

HtmlBuilder& HtmlBuilder::operator=(const atools::util::HtmlBuilder& other)
{
  rowBackColor = other.rowBackColor;
  rowBackColorAlt = other.rowBackColorAlt;
  tableRowHeader = other.tableRowHeader;
  tableRow = other.tableRow;
  tableRowAlignRight = other.tableRowAlignRight;
  tableRowBegin = other.tableRowBegin;
  tableIndex = other.tableIndex;
  defaultPrecision = other.defaultPrecision;
  numLines = other.numLines;
  htmlText = other.htmlText;
  locale = other.locale;
  dateFormat = other.dateFormat;
  hasBackColor = other.hasBackColor;

  return *this;
}

void HtmlBuilder::initColors(const QColor& rowColor, const QColor& rowColorAlt)
{
  // Create darker colors dynamically from default palette
  rowBackColor = rowColor.name(QColor::HexRgb);
  rowBackColorAlt = rowColorAlt.name(QColor::HexRgb);

  if(hasBackColor)
  {
    tableRow.append("<tr bgcolor=\"" + rowBackColor + "\"><td>%1</td><td>%2</td></tr>");
    tableRow.append("<tr bgcolor=\"" + rowBackColorAlt + "\"><td>%1</td><td>%2</td></tr>");

    tableRowAlignRight.append(
      "<tr bgcolor=\"" + rowBackColor + "\"><td>%1</td><td align=\"right\">%2</td></tr>");
    tableRowAlignRight.append(
      "<tr bgcolor=\"" + rowBackColorAlt + "\"><td>%1</td><td align=\"right\">%2</td></tr>");

    tableRowBegin.append("<tr bgcolor=\"" + rowBackColor + "\">");
    tableRowBegin.append("<tr bgcolor=\"" + rowBackColorAlt + "\">");
  }
  else
  {
    tableRow.append("<tr><td>%1</td><td>%2</td></tr>");
    tableRow.append("<tr><td>%1</td><td>%2</td></tr>");

    tableRowAlignRight.append("<tr><td>%1</td><td align=\"right\">%2</td></tr>");
    tableRowAlignRight.append("<tr><td>%1</td><td align=\"right\">%2</td></tr>");
  }
  tableRowHeader = "<tr><td>%1</td></tr>";
}

HtmlBuilder& HtmlBuilder::clear()
{
  htmlText.clear();
  numLines = 0;
  tableIndex = 0;
  return *this;
}

HtmlBuilder HtmlBuilder::cleared() const
{
  HtmlBuilder html(*this);
  html.clear();
  return html;
}

HtmlBuilder& HtmlBuilder::append(const HtmlBuilder& other)
{
  htmlText += other.getHtml();
  return *this;
}

HtmlBuilder& HtmlBuilder::append(const QString& other)
{
  htmlText += other;
  return *this;
}

HtmlBuilder& HtmlBuilder::error(const QString& str)
{
  htmlText += HtmlBuilder::errorMessage(str);
  return *this;
}

QString HtmlBuilder::errorMessage(const QString& str)
{
  if(!str.isEmpty())
    return QString("<span style=\"background-color: #ff0000; color: #ffffff; font-weight: bold;\">"
                     "%1"
                   "</span>").arg(str.toHtmlEscaped());

  return str;
}

HtmlBuilder& HtmlBuilder::warning(const QString& str)
{
  htmlText += HtmlBuilder::warningMessage(str);
  return *this;
}

QString HtmlBuilder::warningMessage(const QString& str)
{
  if(!str.isEmpty())
    return QString("<span style=\"color: #ff5000; font-weight: bold\">"
                     "%1"
                   "</span>").arg(str.toHtmlEscaped());

  return str;
}

HtmlBuilder& HtmlBuilder::row2Var(const QString& name, const QVariant& value, html::Flags flags,
                                  QColor color)
{
  QString valueStr;
  switch(value.type())
  {
    case QVariant::Invalid:
      valueStr = "Error: Invalid Variant";
      qWarning() << "Invalid Variant in HtmlBuilder. Name" << name;
      break;
    case QVariant::Bool:
      valueStr = value.toBool() ? tr("Yes") : tr("No");
      break;
    case QVariant::Int:
      valueStr = locale.toString(value.toInt());
      break;
    case QVariant::UInt:
      valueStr = locale.toString(value.toUInt());
      break;
    case QVariant::LongLong:
      valueStr = locale.toString(value.toLongLong());
      break;
    case QVariant::ULongLong:
      valueStr = locale.toString(value.toULongLong());
      break;
    case QVariant::Double:
      valueStr = locale.toString(value.toDouble(), 'f', defaultPrecision);
      break;
    case QVariant::Char:
    case QVariant::String:
      valueStr = value.toString();
      break;
    case QVariant::StringList:
      valueStr = value.toStringList().join(", ");
      break;
    case QVariant::Date:
      valueStr = locale.toString(value.toDate(), dateFormat);
      break;
    case QVariant::Time:
      valueStr = locale.toString(value.toTime(), dateFormat);
      break;
    case QVariant::DateTime:
      valueStr = locale.toString(value.toDateTime(), dateFormat);
      break;
    default:
      qWarning() << "Invalid variant type" << value.typeName() << "in HtmlBuilder. Name" << name;
      valueStr = QString("Error: Invalid variant type \"%1\"").arg(value.typeName());

  }
  htmlText += alt(flags & html::ALIGN_RIGHT ? tableRowAlignRight : tableRow).
              arg(asText(name, flags, color), value.toString());
  tableIndex++;
  numLines++;
  return *this;
}

HtmlBuilder& HtmlBuilder::row2If(const QString& name, const QString& value, html::Flags flags, QColor color)
{
  if(!value.isEmpty())
  {
    htmlText += alt(flags & html::ALIGN_RIGHT ? tableRowAlignRight : tableRow).
                arg(asText(name, flags | atools::util::html::BOLD, color)).
                arg(asText(value, flags, color));
    tableIndex++;
    numLines++;
  }
  return *this;
}

HtmlBuilder& HtmlBuilder::row2If(const QString& name, int value, html::Flags flags, QColor color)
{
  if(value > 0)
    row2(name, QString::number(value), flags, color);

  return *this;
}

HtmlBuilder& HtmlBuilder::row2IfVar(const QString& name, const QVariant& value, html::Flags flags, QColor color)
{
  if(!value.isNull() && value.isValid())
    row2(name, value.toString(), flags, color);

  return *this;
}

HtmlBuilder& HtmlBuilder::row2(const QString& name, const HtmlBuilder& value, html::Flags flags, QColor color)
{
  return row2(name, value.getHtml(), flags | html::NO_ENTITIES, color);
}

HtmlBuilder& HtmlBuilder::row2(const QString& name, const QString& value, html::Flags flags, QColor color)
{
  htmlText += alt(flags & html::ALIGN_RIGHT ? tableRowAlignRight : tableRow).
              arg(asText(name, flags | atools::util::html::BOLD, color)).
              arg(asText(value, flags, color));
  tableIndex++;
  numLines++;
  return *this;
}

HtmlBuilder& HtmlBuilder::row2(const QString& name, float value, int precision, html::Flags flags,
                               QColor color)
{
  return row2(name, locale.toString(value, 'f', precision != -1 ? precision : defaultPrecision),
              flags, color);
}

HtmlBuilder& HtmlBuilder::row2(const QString& name, double value, int precision, html::Flags flags,
                               QColor color)
{
  return row2(name, locale.toString(value, 'f', precision != -1 ? precision : defaultPrecision),
              flags, color);
}

HtmlBuilder& HtmlBuilder::row2(const QString& name, int value, html::Flags flags, QColor color)
{
  return row2(name, locale.toString(value), flags, color);
}

HtmlBuilder& HtmlBuilder::td(const QString& str, html::Flags flags, QColor color)
{
  htmlText += QString("<td") + (flags & html::ALIGN_RIGHT ? " style=\"text-align: right;\"" : "") + ">";
  text(str, flags, color);
  htmlText += "</td>\n";
  return *this;
}

HtmlBuilder& HtmlBuilder::tdF(html::Flags flags)
{
  htmlText += QString("<td") +
              (flags & html::ALIGN_RIGHT ? " style=\"text-align: right;\"" : "") + ">";
  return *this;
}

HtmlBuilder& HtmlBuilder::tdAtts(const QHash<QString, QString>& attributes)
{
  QString atts;
  for(const QString& name : attributes.keys())
    atts += QString(" %1=\"%2\" ").arg(name).arg(attributes.value(name));

  htmlText += "<td " + atts + ">";
  tableIndex = 0;
  return *this;
}

HtmlBuilder& HtmlBuilder::td()
{
  htmlText += QString("<td>");
  return *this;
}

HtmlBuilder& HtmlBuilder::tdW(int widthPercent)
{
  htmlText += QString("<td width=\"%1%\">").arg(widthPercent);
  return *this;
}

HtmlBuilder& HtmlBuilder::tdEnd()
{
  htmlText += QString("</td>\n");
  return *this;
}

HtmlBuilder& HtmlBuilder::th(const QString& str, html::Flags flags, QColor color)
{
  htmlText += QString("<th") + (flags & html::ALIGN_RIGHT ? " align=\"right\"" : "") + ">";
  text(str, flags, color);
  htmlText += "</th>\n";
  return *this;
}

HtmlBuilder& HtmlBuilder::tr(QColor backgroundColor)
{
  if(backgroundColor.isValid())
    htmlText += "<tr bgcolor=\"" + backgroundColor.name(QColor::HexRgb) + "\">\n";
  else
  {
    if(hasBackColor)
      htmlText += alt(tableRowBegin);
    else
      htmlText += "<tr>\n";
  }
  tableIndex++;
  numLines++;
  return *this;
}

HtmlBuilder& HtmlBuilder::tr()
{
  htmlText += "<tr>\n";
  tableIndex++;
  numLines++;
  return *this;
}

HtmlBuilder& HtmlBuilder::trEnd()
{
  htmlText += "</tr>\n";
  return *this;
}

HtmlBuilder& HtmlBuilder::table()
{
  htmlText += "<table border=\"0\" cellpadding=\"2\" cellspacing=\"0\">\n<tbody>\n";
  tableIndex = 0;
  return *this;
}

HtmlBuilder& HtmlBuilder::tableAtts(const QHash<QString, QString>& attributes)
{
  QString atts;
  for(const QString& name : attributes.keys())
    atts += QString(" %1=\"%2\" ").arg(name).arg(attributes.value(name));

  htmlText += "<table " + atts + ">\n<tbody>\n";
  tableIndex = 0;
  return *this;
}

HtmlBuilder& HtmlBuilder::tableEnd()
{
  htmlText += "</tbody>\n</table>\n";
  tableIndex = 0;
  return *this;
}

HtmlBuilder& HtmlBuilder::h(int level, const QString& str, html::Flags flags, QColor color,
                            const QString& id)
{
  QString num = QString::number(level);
  htmlText += "<h" + num + (id.isEmpty() ? QString() : " id=\"" + id + "\"") + ">" +
              asText(str, flags, color) + "</h" + num + ">\n";
  tableIndex = 0;
  numLines++;
  return *this;
}

HtmlBuilder& HtmlBuilder::h1(const QString& str, html::Flags flags, QColor color, const QString& id)
{
  h(1, str, flags, color, id);
  return *this;
}

HtmlBuilder& HtmlBuilder::h2(const QString& str, html::Flags flags, QColor color, const QString& id)
{
  h(2, str, flags, color, id);
  return *this;
}

HtmlBuilder& HtmlBuilder::h3(const QString& str, html::Flags flags, QColor color, const QString& id)
{
  h(3, str, flags, color, id);
  return *this;
}

HtmlBuilder& HtmlBuilder::h4(const QString& str, html::Flags flags, QColor color, const QString& id)
{
  h(4, str, flags, color, id);
  return *this;
}

HtmlBuilder& HtmlBuilder::h5(const QString& str, html::Flags flags, QColor color, const QString& id)
{
  h(4, str, flags, color, id);
  return *this;
}

HtmlBuilder& HtmlBuilder::b(const QString& str)
{
  text(str, html::BOLD);
  return *this;
}

HtmlBuilder& HtmlBuilder::b()
{
  htmlText += "<b>";
  return *this;
}

HtmlBuilder& HtmlBuilder::bEnd()
{
  htmlText += "</b>";
  return *this;
}

HtmlBuilder& HtmlBuilder::i()
{
  htmlText += "<i>";
  return *this;
}

HtmlBuilder& HtmlBuilder::iEnd()
{
  htmlText += "</i>";
  return *this;
}

HtmlBuilder& HtmlBuilder::nbsp()
{
  htmlText += "&nbsp;";
  return *this;
}

HtmlBuilder& HtmlBuilder::u(const QString& str)
{
  text(str, html::UNDERLINE);
  return *this;
}

HtmlBuilder& HtmlBuilder::u()
{
  htmlText += "<u>";
  return *this;
}

HtmlBuilder& HtmlBuilder::uEnd()
{
  htmlText += "</u>";
  return *this;
}

HtmlBuilder& HtmlBuilder::sub(const QString& str)
{
  text(str, html::SUBSCRIPT);
  return *this;
}

HtmlBuilder& HtmlBuilder::sub()
{
  htmlText += "<sub>";
  return *this;
}

HtmlBuilder& HtmlBuilder::subEnd()
{
  htmlText += "</sub>";
  return *this;
}

HtmlBuilder& HtmlBuilder::sup(const QString& str)
{
  text(str, html::SUPERSCRIPT);
  return *this;
}

HtmlBuilder& HtmlBuilder::sup()
{
  htmlText += "<sup>";
  return *this;
}

HtmlBuilder& HtmlBuilder::supEnd()
{
  htmlText += "</sup>";
  return *this;
}

HtmlBuilder& HtmlBuilder::small(const QString& str)
{
  text(str, html::SMALL);
  return *this;
}

HtmlBuilder& HtmlBuilder::small()
{
  htmlText += "<small>";
  return *this;
}

HtmlBuilder& HtmlBuilder::smallEnd()
{
  htmlText += "</small>";
  return *this;
}

HtmlBuilder& HtmlBuilder::big(const QString& str)
{
  text(str, html::BIG);
  return *this;
}

HtmlBuilder& HtmlBuilder::big()
{
  htmlText += "<big>";
  return *this;
}

HtmlBuilder& HtmlBuilder::bigEnd()
{
  htmlText += "</big>";
  return *this;
}

HtmlBuilder& HtmlBuilder::code(const QString& str)
{
  text(str, html::CODE);
  return *this;
}

HtmlBuilder& HtmlBuilder::code()
{
  htmlText += "<code>";
  return *this;
}

HtmlBuilder& HtmlBuilder::codeEnd()
{
  htmlText += "</code>";
  return *this;
}

HtmlBuilder& HtmlBuilder::nobr(const QString& str)
{
  text(str, html::NOBR);
  return *this;
}

HtmlBuilder& HtmlBuilder::br()
{
  htmlText += "<br/>";
  numLines++;
  return *this;
}

HtmlBuilder& HtmlBuilder::p()
{
  htmlText += "<p>";
  numLines++;
  return *this;
}

HtmlBuilder& HtmlBuilder::pEnd()
{
  htmlText += "</p>\n";
  return *this;
}

HtmlBuilder& HtmlBuilder::pre(const QString& str, html::Flags flags, QColor color)
{

  htmlText += "<pre>";
  text(str, flags, color);
  htmlText += "</pre>";
  return *this;
}

HtmlBuilder& HtmlBuilder::brText(const QString& str)
{
  br();
  htmlText += str.toHtmlEscaped();
  return *this;
}

HtmlBuilder& HtmlBuilder::textBr(const QString& str)
{
  htmlText += str.toHtmlEscaped();
  return br();
}

HtmlBuilder& HtmlBuilder::hr(int size, int widthPercent)
{
  htmlText += "<hr size=\"" + QString::number(size) + "\" width=\"" + QString::number(widthPercent) +
              "%\"/>\n";
  numLines++;
  return *this;
}

HtmlBuilder& HtmlBuilder::a(const QString& text, const QString& href, html::Flags flags, QColor color)
{
  QString style;
  if(flags & html::LINK_NO_UL)
    style = "style=\"text-decoration:none;\"";

  htmlText += "<a " + style + " " + (href.isEmpty() ? QString() : " href=\"" + href + "\"") + ">" +
              asText(text, flags, color) + "</a>";
  return *this;
}

HtmlBuilder& HtmlBuilder::img(const QIcon& icon, const QString& alt, const QString& style, QSize size)
{
  QByteArray data;
  QBuffer buffer(&data);
  icon.pixmap(size).save(&buffer, "PNG", 100);

  img(QString("data:image/png;base64, %0").arg(QString(data.toBase64())), alt, style, size);
  return *this;
}

HtmlBuilder& HtmlBuilder::img(const QString& src, const QString& alt, const QString& style, QSize size)
{
  htmlText += "<img src='" + src + "'" + (style.isEmpty() ? QString() : " style=\"" + style + "\"") +
              (alt.isEmpty() ? QString() : " alt=\"" + alt + "\"") +
              (size.isValid() ?
               QString(" width=\"") + QString::number(size.width()) + "\"" +
               " height=\"" + QString::number(size.height()) + "\"" : QString()) + "/>";

  return *this;
}

HtmlBuilder& HtmlBuilder::ol()
{
  htmlText += "<ol>";
  return *this;
}

HtmlBuilder& HtmlBuilder::olEnd()
{
  htmlText += "</ol>\n";
  return *this;
}

HtmlBuilder& HtmlBuilder::ul()
{
  htmlText += "<ul>";
  return *this;
}

HtmlBuilder& HtmlBuilder::ulEnd()
{
  htmlText += "</ul>\n";
  return *this;
}

HtmlBuilder& HtmlBuilder::li(const QString& str, html::Flags flags, QColor color)
{
  htmlText += "<li>" + asText(str, flags, color) + "</li>\n";
  numLines++;
  return *this;
}

QString HtmlBuilder::asText(QString str, html::Flags flags, QColor color)
{
  QString prefix, suffix;
  if(flags & html::BOLD)
  {
    prefix.append("<b>");
    suffix.prepend("</b>");
  }

  if(flags & html::ITALIC)
  {
    prefix.append("<i>");
    suffix.prepend("</i>");
  }

  if(flags & html::UNDERLINE)
  {
    prefix.append("<u>");
    suffix.prepend("</u>");
  }

  if(flags & html::STRIKEOUT)
  {
    prefix.append("<s>");
    suffix.prepend("</s>");
  }

  if(flags & html::SUBSCRIPT)
  {
    prefix.append("<sub>");
    suffix.prepend("</sub>");
  }

  if(flags & html::SUPERSCRIPT)
  {
    prefix.append("<sup>");
    suffix.prepend("</sup>");
  }

  if(flags & html::SMALL)
  {
    prefix.append("<small>");
    suffix.prepend("</small>");
  }

  if(flags & html::BIG)
  {
    prefix.append("<big>");
    suffix.prepend("</big>");
  }

  if(flags & html::CODE)
  {
    prefix.append("<code>");
    suffix.prepend("</code>");
  }

  if(flags & html::NOBR)
  {
    prefix.append("<nobr>");
    suffix.prepend("</nobr>");
  }

  if(color.isValid())
  {
    prefix.append("<span style=\"color:" + color.name(QColor::HexRgb) + "\">");
    suffix.prepend("</span>");
  }

  if(!(flags & html::NO_ENTITIES))
    str = toEntities(str.toHtmlEscaped()).replace("\n", "<br/>");

  if(flags & html::REPLACE_CRLF)
  {
    str = str.replace("\r\n", "<br/>");
    str = str.replace("\n", "<br/>");
    str = str.replace("\r", "<br/>");
  }

  if(flags & html::AUTOLINK)
    str.replace(LINK_REGEXP, "<a href=\"\\1\">\\1</a>");

  return prefix + str + suffix;
}

bool HtmlBuilder::checklength(int maxLines, const QString& msg)
{
  QString dotText(QString("<b>%1</b>").arg(msg));
  if(numLines > maxLines)
  {
    if(!htmlText.endsWith(dotText))
      hr().b(msg);
    return true;
  }
  return false;
}

HtmlBuilder& HtmlBuilder::textBar(int lenght, html::Flags flags, QColor color)
{
  QString str;
  str.resize(lenght, QChar(L'—'));
  text(str, flags, color).br();
  return *this;
}

HtmlBuilder& HtmlBuilder::text(const QString& str, html::Flags flags, QColor color)
{
  htmlText += asText(str, flags, color);
  return *this;
}

HtmlBuilder& HtmlBuilder::textHtml(const HtmlBuilder& other)
{
  text(other.getHtml(), html::NO_ENTITIES);
  return *this;
}

HtmlBuilder& HtmlBuilder::p(const QString& str, html::Flags flags, QColor color)
{
  htmlText += "<p>";
  text(str, flags, color);
  htmlText += "</p>\n";
  tableIndex = 0;
  numLines++;
  return *this;
}

HtmlBuilder& HtmlBuilder::doc(const QString& title, const QString& css)
{
  htmlText +=
    "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
      "<html>\n"
        "<head>\n";

  if(!css.isEmpty())
    htmlText += QString("<style type=\"text/css\" xml:space=\"preserve\">\n%1</style>\n").arg(css);

  if(!title.isEmpty())
    htmlText += QString("<title>%1</title>\n").arg(title);

  // <link rel="stylesheet" href="css/style.css" type="text/css" />
  htmlText += "</head>\n";
  htmlText += "<body style=\"font-family:'sans'; font-size:8pt; font-weight:400; font-style:normal;\">\n";

  tableIndex = 0;
  return *this;
}

HtmlBuilder& HtmlBuilder::docEnd()
{
  htmlText += "</body>\n</html>\n";
  return *this;
}

const QString& HtmlBuilder::alt(const QStringList& list) const
{
  return list.at(tableIndex % list.size());
}

QString HtmlBuilder::getEncodedImageHref(const QIcon& icon, QSize imageSize)
{
  QByteArray data;
  QBuffer buffer(&data);
  icon.pixmap(imageSize).save(&buffer, "PNG", 100);

  return QString("data:image/png;base64, %0").arg(QString(data.toBase64()));
}

QString HtmlBuilder::toEntities(const QString& src)
{
  QString tmp(src);
  int i = 0, len = tmp.length();
  while(i < len)
  {
    if(tmp.at(i).unicode() > 128)
    {
      QString rp = "&#" + QString::number(tmp[i].unicode()) + ";";
      tmp.replace(i, 1, rp);
      len += rp.length() - 1;
      i += rp.length();
    }
    else
      i++;
  }
  return tmp;
}

} // namespace util
} // namespace atools
