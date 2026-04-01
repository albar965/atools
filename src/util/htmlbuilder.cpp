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

#include "util/htmlbuilder.h"

#include "atools.h"

#include <QApplication>
#include <QPalette>
#include <QDateTime>
#include <QBuffer>
#include <QIcon>
#include <QRegularExpression>
#include <QStringBuilder>
#include <QFileInfo>
#include <QUrl>

namespace atools {
namespace util {

const QColor HtmlBuilder::COLOR_FOREGROUND_ERROR("#ffffff");
const QColor HtmlBuilder::COLOR_BACKGROUND_ERROR("#ff0000");
const QColor HtmlBuilder::COLOR_FOREGROUND_WARNING("#ff2000");
const QColor HtmlBuilder::COLOR_BACKGROUND_WARNING(Qt::transparent);

const html::Flags HtmlBuilder::MSG_FLAGS = html::BOLD | html::NO_ENTITIES;

// Matches "http://blah" and "https://www.example.com/blah" links
static const QRegularExpression LINK_REGEXP("\\b((http[s]?|ftp|file)://[a-zA-Z0-9\\./:_\\?\\&=\\-\\$\\+\\!\\*'\\(\\),;%#\\[\\]@]+)\\b");

HtmlBuilder::HtmlBuilder(const QColor& rowColor, const QColor& rowColorAlt)
  : hasBackColor(true)
{
  initColors(rowColor, rowColorAlt);

  idBits.fill(false, MAX_ID + 1);
}

void HtmlBuilder::init()
{
  if(hasBackColor)
    // Create darker colors dynamically from default palette
    initColors(QApplication::palette().color(QPalette::Active, QPalette::Base).darker(105),
               QApplication::palette().color(QPalette::Active, QPalette::AlternateBase).darker(105));
  else
    initColors(QColor(Qt::white), QColor(Qt::white).darker(120));

  idBits.fill(false, MAX_ID + 1);
}

HtmlBuilder& HtmlBuilder::operator=(const QString& other)
{
  htmlText = other;
  return *this;
}

HtmlBuilder& HtmlBuilder::operator=(const atools::util::HtmlBuilder& other)
{
  rowBackColorStr = other.rowBackColorStr;
  rowBackColorAltStr = other.rowBackColorAltStr;
  rowBackColor = other.rowBackColor;
  rowBackColorAlt = other.rowBackColorAlt;
  tableRowHeader = other.tableRowHeader;
  tableRow = other.tableRow;
  tableRowAlignRight = other.tableRowAlignRight;
  tableRowBegin = other.tableRowBegin;
  defaultPrecision = other.defaultPrecision;
  numLines = other.numLines;
  htmlText = other.htmlText;
  locale = other.locale;
  dateFormat = other.dateFormat;
  hasBackColor = other.hasBackColor;
  markIndex = other.markIndex;
  tableRowsCur = other.tableRowsCur;
  currentId = other.currentId;
  idBits = other.idBits;

  return *this;
}

QString HtmlBuilder::joinBr(std::initializer_list<HtmlBuilder> builders)
{
  QStringList texts;
  for(const HtmlBuilder& builder : builders)
    texts.append(builder.getHtml());
  return joinBr(texts);
}

QString HtmlBuilder::joinP(std::initializer_list<HtmlBuilder> builders)
{
  QStringList texts;
  for(const HtmlBuilder& builder : builders)
    texts.append(builder.getHtml());
  return joinP(texts);
}

QString HtmlBuilder::joinHr(std::initializer_list<HtmlBuilder> builders)
{
  QStringList texts;
  for(const HtmlBuilder& builder : builders)
    texts.append(builder.getHtml());
  return joinHr(texts);
}

QString HtmlBuilder::joinBr(QStringList strings)
{
  return atools::strJoin(strings, QStringLiteral("<br/>"));
}

QString HtmlBuilder::joinHr(QStringList strings)
{
  return atools::strJoin(strings, QStringLiteral("<small><hr/></small>"));
}

QString HtmlBuilder::joinP(QStringList strings)
{
  return atools::strJoin(QStringLiteral("<p>"), strings, QStringLiteral("<p/><p>"), QStringLiteral("<p/><p>"), QStringLiteral("</p>"));
}

void HtmlBuilder::initColors(const QColor& rowColor, const QColor& rowColorAlt)
{
  // Create darker colors dynamically from default palette
  rowBackColor = rowColor;
  rowBackColorAlt = rowColorAlt;

  rowBackColorStr = rowBackColor.name(QColor::HexRgb);
  rowBackColorAltStr = rowBackColorAlt.name(QColor::HexRgb);

  if(hasBackColor)
  {
    tableRow.append(QStringLiteral("<tr bgcolor=\"") % rowBackColorStr % QStringLiteral("\"><td>%1</td><td>%2</td></tr>"));
    tableRow.append(QStringLiteral("<tr bgcolor=\"") % rowBackColorAltStr % QStringLiteral("\"><td>%1</td><td>%2</td></tr>"));

    tableRowAlignRight.append(
      QStringLiteral("<tr bgcolor=\"") % rowBackColorStr % QStringLiteral("\"><td>%1</td><td align=\"right\">%2</td></tr>"));
    tableRowAlignRight.append(
      QStringLiteral("<tr bgcolor=\"") % rowBackColorAltStr % QStringLiteral("\"><td>%1</td><td align=\"right\">%2</td></tr>"));

    tableRowBegin.append(QStringLiteral("<tr bgcolor=\"") % rowBackColorStr % QStringLiteral("\">"));
    tableRowBegin.append(QStringLiteral("<tr bgcolor=\"") % rowBackColorAltStr % QStringLiteral("\">"));
  }
  else
  {
    tableRow.append(QStringLiteral("<tr><td>%1</td><td>%2</td></tr>"));
    tableRow.append(QStringLiteral("<tr><td>%1</td><td>%2</td></tr>"));

    tableRowAlignRight.append(QStringLiteral("<tr><td>%1</td><td align=\"right\">%2</td></tr>"));
    tableRowAlignRight.append(QStringLiteral("<tr><td>%1</td><td align=\"right\">%2</td></tr>"));
  }
  tableRowHeader = QStringLiteral("<tr><td>%1</td></tr>");
}

HtmlBuilder& HtmlBuilder::clear()
{
  htmlText.clear();
  idBits.fill(false, MAX_ID + 1);
  numLines = 0;
  tableRowsCur = 0;
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
  htmlText.append(other.getHtml());
  return *this;
}

HtmlBuilder& HtmlBuilder::append(const QString& other)
{
  htmlText.append(other);
  return *this;
}

HtmlBuilder& HtmlBuilder::append(const char *other)
{
  htmlText.append(QString(other));
  return *this;
}

HtmlBuilder& HtmlBuilder::mark(int markParam)
{
  markIndex = markParam;
  return *this;
}

HtmlBuilder& HtmlBuilder::mark()
{
  markIndex = htmlText.size();
  return *this;
}

HtmlBuilder& HtmlBuilder::clearMark()
{
  mark(-1);
  return *this;
}

HtmlBuilder& HtmlBuilder::rewind()
{
  if(markIndex != -1)
    htmlText.truncate(markIndex);
  return *this;
}

int HtmlBuilder::getMark() const
{
  return markIndex;
}

HtmlBuilder& HtmlBuilder::error(const QString& str, html::Flags flags)
{
  htmlText.append(HtmlBuilder::errorMessage(str, flags));
  return *this;
}

QString HtmlBuilder::errorMessage(const QStringList& stringList, const QString& separator, html::Flags flags)
{
  QStringList errList;
  for(const QString& str : stringList)
    errList.append(errorMessage(str, flags));
  return errList.join(separator);
}

QString HtmlBuilder::errorMessage(const QString& str, html::Flags flags)
{
  if(!str.isEmpty())
    return textMessage(str, flags, COLOR_FOREGROUND_ERROR, COLOR_BACKGROUND_ERROR);

  return str;
}

HtmlBuilder& HtmlBuilder::warning(const QString& str, html::Flags flags)
{
  htmlText.append(HtmlBuilder::warningMessage(str, flags));
  return *this;
}

QString HtmlBuilder::warningMessage(const QString& str, html::Flags flags)
{
  if(!str.isEmpty())
    return textMessage(str, flags, COLOR_FOREGROUND_WARNING, COLOR_BACKGROUND_WARNING);

  return str;
}

QString HtmlBuilder::warningMessage(const QStringList& stringList, const QString& separator, html::Flags flags)
{
  QStringList warnList;
  for(const QString& str : stringList)
    warnList.append(warningMessage(str, flags));
  return warnList.join(separator);
}

HtmlBuilder& HtmlBuilder::note(const QString& str, html::Flags flags)
{
  htmlText.append(HtmlBuilder::noteMessage(str, flags));
  return *this;
}

QString HtmlBuilder::noteMessage(const QString& str, html::Flags flags)
{
  if(!str.isEmpty())
    return textMessage(str, flags, QColor(QStringLiteral("#00aa00")));

  return str;
}

QString HtmlBuilder::noteMessage(const QStringList& stringList, const QString& separator, html::Flags flags)
{
  QStringList warnList;
  for(const QString& str : stringList)
    warnList.append(noteMessage(str, flags));
  return warnList.join(separator);
}

HtmlBuilder& HtmlBuilder::message(const QString& str, html::Flags flags, QColor foreground, QColor background)
{
  htmlText.append(textMessage(str, flags, foreground, background));
  numLines++;
  return *this;
}

QString HtmlBuilder::textMessage(const QString& str, html::Flags flags, QColor foreground, QColor background)
{
  if(!str.isEmpty())
    return asText(str, flags, foreground, background);

  return str;
}

QString HtmlBuilder::textMessage(const QStringList& stringList, html::Flags flags, QColor foreground, QColor background,
                                 const QString& separator)
{
  QStringList warnList;
  for(const QString& str : stringList)
    warnList.append(textMessage(str, flags, foreground, background));
  return warnList.join(separator);
}

HtmlBuilder& HtmlBuilder::row2AlignRight(bool alignRight)
{
  row2AlignRightFlag = alignRight;
  return *this;
}

HtmlBuilder& HtmlBuilder::row2Var(const QString& name, const QVariant& value, html::Flags flags, QColor color)
{
  if(isId())
  {
    flags |= row2AlignRightFlag ? html::ALIGN_RIGHT : html::NONE;

    QString valueStr;
    switch(value.metaType().id())
    {
      case QMetaType::Bool:
        valueStr = value.toBool() ? tr("Yes") : tr("No");
        break;

      case QMetaType::Type::Int:
        valueStr = locale.toString(value.toInt());
        break;

      case QMetaType::Type::UInt:
        valueStr = locale.toString(value.toUInt());
        break;

      case QMetaType::Type::LongLong:
        valueStr = locale.toString(value.toLongLong());
        break;

      case QMetaType::Type::ULongLong:
        valueStr = locale.toString(value.toULongLong());
        break;

      case QMetaType::Type::Double:
        valueStr = locale.toString(value.toDouble(), 'f', defaultPrecision);
        break;

      case QMetaType::Type::Char:
      case QMetaType::Type::QString:
        valueStr = value.toString();
        break;

      case QMetaType::Type::QStringList:
        valueStr = value.toStringList().join(QStringLiteral(", "));
        break;

      case QMetaType::Type::QDate:
        valueStr = locale.toString(value.toDate(), dateFormat);
        break;

      case QMetaType::Type::QTime:
        valueStr = locale.toString(value.toTime(), dateFormat);
        break;

      case QMetaType::Type::QDateTime:
        valueStr = locale.toString(value.toDateTime(), dateFormat);
        break;

      default:
        qWarning() << "Invalid variant type" << value.typeName() << "in HtmlBuilder. Name" << name;
        valueStr = QStringLiteral("Error: Invalid variant type \"%1\"").arg(value.typeName());

    }
    htmlText.append(alt(flags.testFlag(html::ALIGN_RIGHT) ? tableRowAlignRight : tableRow).arg(asText(name, flags, color),
                                                                                               value.toString()));
    tableRowsCur++;
    numLines++;
  }
  return *this;
}

HtmlBuilder& HtmlBuilder::row2If(const QString& name, const QString& value, html::Flags flags, QColor color)
{
  if(isId())
  {
    flags |= row2AlignRightFlag ? html::ALIGN_RIGHT : html::NONE;
    if(!value.isEmpty())
    {
      htmlText.append(alt(flags.testFlag(html::ALIGN_RIGHT) ? tableRowAlignRight : tableRow).
                      arg(asText(name, flags | atools::util::html::BOLD, color)).
                      arg(asText(value, flags, color)));
      tableRowsCur++;
      numLines++;
    }
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
  if(!atools::isVariantNull(value))
    row2(name, value.toString(), flags, color);

  return *this;
}

HtmlBuilder& HtmlBuilder::row2(const QString& name, const HtmlBuilder& value, html::Flags flags, QColor color)
{
  return row2(name, value.getHtml(), flags | html::NO_ENTITIES, color);
}

HtmlBuilder& HtmlBuilder::row2Warning(const QString& name, const QString& value, html::Flags flags)
{
  return row2(name, warningMessage(value), flags | html::NO_ENTITIES);
}

HtmlBuilder& HtmlBuilder::row2Error(const QString& name, const QString& value, html::Flags flags)
{
  return row2(name, errorMessage(value), flags | html::NO_ENTITIES);
}

HtmlBuilder& HtmlBuilder::row2(const QString& name, const QString& value, html::Flags flags, QColor color)
{
  if(isId())
  {
    flags |= row2AlignRightFlag ? html::ALIGN_RIGHT : html::NONE;
    htmlText.append(alt(flags.testFlag(html::ALIGN_RIGHT) ? tableRowAlignRight : tableRow).
                    arg(asText(name, flags | html::BOLD, color)).
                    // Add space to avoid formatting issues with table
                    arg(value.isEmpty() ? QStringLiteral("&nbsp;") : asText(value, flags, color)));
    tableRowsCur++;
    numLines++;
  }
  return *this;
}

HtmlBuilder& HtmlBuilder::row2(const QString& name, float value, int precision, html::Flags flags, QColor color)
{
  return row2(name, locale.toString(value, 'f', precision != -1 ? precision : defaultPrecision), flags, color);
}

HtmlBuilder& HtmlBuilder::row2(const QString& name, double value, int precision, html::Flags flags, QColor color)
{
  return row2(name, locale.toString(value, 'f', precision != -1 ? precision : defaultPrecision), flags, color);
}

HtmlBuilder& HtmlBuilder::row2(const QString& name, int value, html::Flags flags, QColor color)
{
  return row2(name, locale.toString(value), flags, color);
}

HtmlBuilder& HtmlBuilder::td(const QString& str, html::Flags flags, QColor color)
{
  htmlText.append(QStringLiteral("<td") %
                  (flags.testFlag(html::ALIGN_RIGHT) ? QStringLiteral(" align=\"right\"") : QStringLiteral()) %
                  (flags.testFlag(html::ALIGN_LEFT) ? QStringLiteral(" align=\"left\"") : QStringLiteral()) %
                  (flags.testFlag(html::ALIGN_CENTER) ? QStringLiteral(" align=\"center\"") : QStringLiteral()) %
                  QStringLiteral(">"));
  text(str, flags, color);
  htmlText.append(QStringLiteral("</td>\n"));
  return *this;
}

HtmlBuilder& HtmlBuilder::tdF(html::Flags flags)
{
  htmlText.append(QStringLiteral("<td") %
                  (flags.testFlag(html::ALIGN_RIGHT) ? QStringLiteral(" align=\"right\"") : QStringLiteral()) %
                  (flags.testFlag(html::ALIGN_LEFT) ? QStringLiteral(" align=\"left\"") : QStringLiteral()) %
                  (flags.testFlag(html::ALIGN_CENTER) ? QStringLiteral(" align=\"center\"") : QStringLiteral()) %
                  QStringLiteral(">"));
  return *this;
}

HtmlBuilder& HtmlBuilder::tdAtts(const QHash<QString, QString>& attributes)
{
  QString atts;
  for(auto it = attributes.constBegin(); it != attributes.constEnd(); ++it)
    atts.append(QStringLiteral(" %1=\"%2\" ").arg(it.key()).arg(it.value()));

  htmlText.append(QStringLiteral("<td ") % atts % QStringLiteral(">"));
  return *this;
}

HtmlBuilder& HtmlBuilder::td()
{
  htmlText.append(QStringLiteral("<td>"));
  return *this;
}

HtmlBuilder& HtmlBuilder::tdW(int widthPercent)
{
  htmlText.append(QStringLiteral("<td width=\"%1%\">").arg(widthPercent));
  return *this;
}

HtmlBuilder& HtmlBuilder::tdEnd()
{
  htmlText.append(QStringLiteral("</td>\n"));
  return *this;
}

HtmlBuilder& HtmlBuilder::th(const QString& str, html::Flags flags, QColor color, int colspan)
{
  htmlText.append(QStringLiteral("<th") %
                  (flags.testFlag(html::ALIGN_RIGHT) ? QStringLiteral(" align=\"right\"") : QStringLiteral()) %
                  (flags.testFlag(html::ALIGN_LEFT) ? QStringLiteral(" align=\"left\"") : QStringLiteral()) %
                  (flags.testFlag(html::ALIGN_CENTER) ? QStringLiteral(" align=\"center\"") : QStringLiteral()) %
                  (colspan != -1 ? QStringLiteral(" colspan=\"") % QString::number(colspan) % QStringLiteral("\"") : QStringLiteral()) %
                  QStringLiteral(">"));
  text(str, flags, color);
  htmlText.append(QStringLiteral("</th>\n"));
  return *this;
}

HtmlBuilder& HtmlBuilder::tr(QColor backgroundColor)
{
  if(backgroundColor.isValid())
    htmlText.append(QStringLiteral("<tr bgcolor=\"") % backgroundColor.name(QColor::HexRgb) % QStringLiteral("\">\n"));
  else
  {
    if(hasBackColor)
      htmlText.append(alt(tableRowBegin));
    else
      htmlText.append(QStringLiteral("<tr>\n"));
  }
  tableRowsCur++;
  numLines++;
  return *this;
}

HtmlBuilder& HtmlBuilder::tr()
{
  htmlText.append(QStringLiteral("<tr>\n"));
  tableRowsCur++;
  numLines++;
  return *this;
}

HtmlBuilder& HtmlBuilder::trEnd()
{
  htmlText.append(QStringLiteral("</tr>\n"));
  return *this;
}

HtmlBuilder& HtmlBuilder::table(int border, int padding, int spacing, int widthPercent, QColor bgcolor, QColor bordercolor)
{
  htmlText.append(QStringLiteral("<table border=\"") % QString::number(border) % QStringLiteral("\" cellpadding=\"") %
                  QString::number(padding) % QStringLiteral("\" cellspacing=\"") % QString::number(spacing) % QStringLiteral("\"") %
                  (bgcolor.isValid() ? QStringLiteral(" bgcolor=\"") % bgcolor.name(QColor::HexRgb) %
                   QStringLiteral("\"") : QStringLiteral()) %
                  (bordercolor.isValid() ? QStringLiteral(" border-color=\"") % bordercolor.name(QColor::HexRgb) %
                   QStringLiteral("\"") : QStringLiteral()) %
                  (widthPercent >
                   0 ? QStringLiteral(" width=\"") % QString::number(widthPercent) % QStringLiteral("%\"") : QStringLiteral()) %
                  QStringLiteral(">\n<tbody>\n"));
  tableRowsCur = 0;
  return *this;
}

HtmlBuilder& HtmlBuilder::tableIf(int border, int padding, int spacing, int widthPercent, QColor bgcolor, QColor bordercolor)
{
  mark();
  return table(border, padding, spacing, widthPercent, bgcolor, bordercolor);
}

HtmlBuilder& HtmlBuilder::tableAtts(const QHash<QString, QString>& attributes)
{
  QString atts;
  for(auto it = attributes.constBegin(); it != attributes.constEnd(); ++it)
    atts.append(QStringLiteral(" %1=\"%2\" ").arg(it.key()).arg(it.value()));

  htmlText.append(QStringLiteral("<table ") % atts % QStringLiteral(">\n<tbody>\n"));
  tableRowsCur = 0;
  return *this;
}

HtmlBuilder& HtmlBuilder::tableEnd()
{
  htmlText.append(QStringLiteral("</tbody>\n</table>\n"));
  tableRowsCur = 0;
  return *this;
}

HtmlBuilder& HtmlBuilder::tableEndIf()
{
  if(isTableEmpty())
    rewind();
  else
    tableEnd();
  tableRowsCur = 0;
  return *this;
}

HtmlBuilder& HtmlBuilder::h(int level, const QString& str, html::Flags flags, QColor color, const QString& id)
{
  QString num = QString::number(level);
  htmlText.append(QStringLiteral("<h") % num % (id.isEmpty() ? QStringLiteral() : QStringLiteral(" id=\"") %
                                                id % QStringLiteral("\"")) % QStringLiteral(">") % asText(str, flags, color));
  return *this;
}

HtmlBuilder& HtmlBuilder::hEnd(int level)
{
  QString num = QString::number(level);
  htmlText.append(QStringLiteral("</h") % num % QStringLiteral(">\n"));
  numLines++;
  return *this;
}

HtmlBuilder& HtmlBuilder::h1(const QString& str, html::Flags flags, QColor color, const QString& id)
{
  h(1, str, flags, color, id).hEnd(1);
  return *this;
}

HtmlBuilder& HtmlBuilder::h2(const QString& str, html::Flags flags, QColor color, const QString& id)
{
  h(2, str, flags, color, id).hEnd(2);
  return *this;
}

HtmlBuilder& HtmlBuilder::h3(const QString& str, html::Flags flags, QColor color, const QString& id)
{
  h(3, str, flags, color, id).hEnd(3);
  return *this;
}

HtmlBuilder& HtmlBuilder::h4(const QString& str, html::Flags flags, QColor color, const QString& id)
{
  h(4, str, flags, color, id).hEnd(4);
  return *this;
}

HtmlBuilder& HtmlBuilder::h5(const QString& str, html::Flags flags, QColor color, const QString& id)
{
  h(5, str, flags, color, id).hEnd(5);
  return *this;
}

HtmlBuilder& HtmlBuilder::b(const QString& str)
{
  text(str, html::BOLD);
  return *this;
}

HtmlBuilder& HtmlBuilder::b()
{
  htmlText.append(QStringLiteral("<b>"));
  return *this;
}

HtmlBuilder& HtmlBuilder::bEnd()
{
  htmlText.append(QStringLiteral("</b>"));
  return *this;
}

HtmlBuilder& HtmlBuilder::i()
{
  htmlText.append(QStringLiteral("<i>"));
  return *this;
}

HtmlBuilder& HtmlBuilder::iEnd()
{
  htmlText.append(QStringLiteral("</i>"));
  return *this;
}

HtmlBuilder& HtmlBuilder::nbsp()
{
  htmlText.append(QStringLiteral("&nbsp;"));
  return *this;
}

HtmlBuilder& HtmlBuilder::u(const QString& str)
{
  text(str, html::UNDERLINE);
  return *this;
}

HtmlBuilder& HtmlBuilder::u()
{
  htmlText.append(QStringLiteral("<u>"));
  return *this;
}

HtmlBuilder& HtmlBuilder::uEnd()
{
  htmlText.append(QStringLiteral("</u>"));
  return *this;
}

HtmlBuilder& HtmlBuilder::sub(const QString& str)
{
  text(str, html::SUBSCRIPT);
  return *this;
}

HtmlBuilder& HtmlBuilder::sub()
{
  htmlText.append(QStringLiteral("<sub>"));
  return *this;
}

HtmlBuilder& HtmlBuilder::subEnd()
{
  htmlText.append(QStringLiteral("</sub>"));
  return *this;
}

HtmlBuilder& HtmlBuilder::sup(const QString& str)
{
  text(str, html::SUPERSCRIPT);
  return *this;
}

HtmlBuilder& HtmlBuilder::sup()
{
  htmlText.append(QStringLiteral("<sup>"));
  return *this;
}

HtmlBuilder& HtmlBuilder::supEnd()
{
  htmlText.append(QStringLiteral("</sup>"));
  return *this;
}

HtmlBuilder& HtmlBuilder::small(const QString& str)
{
  text(str, html::SMALL);
  return *this;
}

HtmlBuilder& HtmlBuilder::small()
{
  htmlText.append(QStringLiteral("<small>"));
  return *this;
}

HtmlBuilder& HtmlBuilder::smallEnd()
{
  htmlText.append(QStringLiteral("</small>"));
  return *this;
}

HtmlBuilder& HtmlBuilder::big(const QString& str)
{
  text(str, html::BIG);
  return *this;
}

HtmlBuilder& HtmlBuilder::big()
{
  htmlText.append(QStringLiteral("<big>"));
  return *this;
}

HtmlBuilder& HtmlBuilder::bigEnd()
{
  htmlText.append(QStringLiteral("</big>"));
  return *this;
}

HtmlBuilder& HtmlBuilder::code(const QString& str)
{
  text(str, html::CODE);
  return *this;
}

HtmlBuilder& HtmlBuilder::code()
{
  htmlText.append(QStringLiteral("<code>"));
  return *this;
}

HtmlBuilder& HtmlBuilder::codeEnd()
{
  htmlText.append(QStringLiteral("</code>"));
  return *this;
}

HtmlBuilder& HtmlBuilder::nobr(const QString& str)
{
  text(str, html::NOBR);
  return *this;
}

HtmlBuilder& HtmlBuilder::br()
{
  htmlText.append(QStringLiteral("<br/>"));
  numLines++;
  return *this;
}

HtmlBuilder& HtmlBuilder::p(const QString& str, html::Flags flags, QColor color)
{
  if(flags.testFlag(html::NOBR_WHITESPACE))
    htmlText.append(QStringLiteral("<p style=\"white-space:pre\">"));
  else
    htmlText.append(QStringLiteral("<p>"));
  text(str, flags, color);
  htmlText.append(QStringLiteral("</p>\n"));
  numLines++;
  return *this;
}

HtmlBuilder& HtmlBuilder::p(html::Flags flags)
{
  if(flags.testFlag(html::NOBR_WHITESPACE))
    htmlText.append(QStringLiteral("<p style=\"white-space:pre\">"));
  else
    htmlText.append(QStringLiteral("<p>"));
  numLines++;
  return *this;
}

HtmlBuilder& HtmlBuilder::pEnd()
{
  htmlText.append(QStringLiteral("</p>\n"));
  return *this;
}

HtmlBuilder& HtmlBuilder::pre()
{
  htmlText.append(QStringLiteral("<pre>"));
  numLines++;
  return *this;
}

HtmlBuilder& HtmlBuilder::preEnd()
{
  htmlText.append(QStringLiteral("</pre>\n"));
  return *this;
}

HtmlBuilder& HtmlBuilder::pre(const QString& str, html::Flags flags, QColor color)
{

  htmlText.append(QStringLiteral("<pre>"));
  text(str, flags, color);
  htmlText.append(QStringLiteral("</pre>"));
  return *this;
}

HtmlBuilder& HtmlBuilder::brText(const QString& str, html::Flags flags, QColor color)
{
  br();
  text(str, flags, color);
  return *this;
}

HtmlBuilder& HtmlBuilder::textBr(const QString& str, html::Flags flags, QColor color)
{
  text(str, flags, color);
  return br();
}

HtmlBuilder& HtmlBuilder::hr(int size, int widthPercent)
{
  htmlText.append(QStringLiteral("<hr size=\"") % QString::number(size) % QStringLiteral("\" width=\"") %
                  QString::number(widthPercent) % QStringLiteral("%\"/>\n"));
  numLines++;
  return *this;
}

HtmlBuilder& HtmlBuilder::a(const QString& text, const QString& href, html::Flags flags, QColor color)
{
  QString styleTxt;

  if(flags.testFlag(html::LINK_NO_UL))
    styleTxt = "style=\"text-decoration:none;\"";

  htmlText.append(QStringLiteral("<a ") % styleTxt % QStringLiteral(" ") %
                  (href.isEmpty() ? QStringLiteral() : QStringLiteral(" href=\"") % href % QStringLiteral("\"")) %
                  QStringLiteral(">") %
                  asText(text, flags, color) % QStringLiteral("</a>"));

  return *this;
}

QString HtmlBuilder::aUrl(const QString& text, const QString& href, html::Flags flags, QColor color, int elideText)
{
  QStringList styles;
  if(flags.testFlag(html::LINK_NO_UL))
    styles.append(QStringLiteral("text-decoration: none;"));

  if(flags.testFlag(html::NOBR_WHITESPACE))
    styles.append(QStringLiteral("white-space: pre;"));

  QString divStart, divEnd;
  if(!styles.isEmpty())
  {
    divStart = QStringLiteral("<div style=\"") % styles.join(' ') % QStringLiteral("\">");
    divEnd = QStringLiteral("</div>");
  }

  return QStringLiteral("<a") % (href.isEmpty() ? QStringLiteral() : QStringLiteral(" href=\"") %
                                 href % QStringLiteral("\"")) % QStringLiteral(">") % divStart %
         asText(atools::elideTextShortMiddle(text, elideText), flags, color) % divEnd % QStringLiteral("</a>");
}

QString HtmlBuilder::aFilePath(const QString& filepath, html::Flags flags, QColor color, int elideText)
{
  QFileInfo info(filepath);
  return aUrl(atools::nativeCleanPath(info.absoluteFilePath()),
              QUrl::fromLocalFile(info.absoluteFilePath()).toString(), flags, color, elideText);
}

QString HtmlBuilder::aFileName(const QString& filepath, html::Flags flags, QColor color, int elideText)
{
  QFileInfo info(filepath);
  return aUrl(info.fileName(), QUrl::fromLocalFile(info.absoluteFilePath()).toString(), flags, color, elideText);
}

HtmlBuilder& HtmlBuilder::img(const QIcon& icon, const QString& alt, const QString& style, QSize size)
{
  // Square size if one dimension is zero
  QSize pixmapSize(size);
  if(pixmapSize.width() == 0)
    pixmapSize.setWidth(pixmapSize.height());
  if(pixmapSize.height() == 0)
    pixmapSize.setHeight(pixmapSize.width());

  // Resulting pixmap might be smaller
  QByteArray data;
  QBuffer buffer(&data);
  icon.pixmap(pixmapSize).save(&buffer, "PNG", 100);

  img(QStringLiteral("data:image/png;base64, %1").arg(QString(data.toBase64())), alt, style, size);
  return *this;
}

HtmlBuilder& HtmlBuilder::img(const QString& src, const QString& alt, const QString& style, QSize size)
{
  QString widthAtt = size.width() >
                     0 ? QStringLiteral(" width=\"") % QString::number(size.width()) % QStringLiteral("\"") : QStringLiteral();
  QString heightAtt = size.height() >
                      0 ? QStringLiteral(" height=\"") % QString::number(size.height()) % QStringLiteral("\"") : QStringLiteral();
  QString altAtt = alt.isEmpty() ? QStringLiteral() : QStringLiteral(" alt=\"") % alt % QStringLiteral("\"");
  QString styleAtt = style.isEmpty() ? QStringLiteral() : QStringLiteral(" style=\"") % style % QStringLiteral("\"");

  htmlText.append(QStringLiteral("<img src='") % src %
                  QStringLiteral("'") % styleAtt % altAtt % widthAtt % heightAtt % QStringLiteral("/>"));

  return *this;
}

HtmlBuilder& HtmlBuilder::ol()
{
  htmlText.append(QStringLiteral("<ol>"));
  return *this;
}

HtmlBuilder& HtmlBuilder::olEnd()
{
  htmlText.append(QStringLiteral("</ol>\n"));
  return *this;
}

HtmlBuilder& HtmlBuilder::ul()
{
  htmlText.append(QStringLiteral("<ul>"));
  return *this;
}

HtmlBuilder& HtmlBuilder::ulEnd()
{
  htmlText.append(QStringLiteral("</ul>\n"));
  return *this;
}

HtmlBuilder& HtmlBuilder::li(const QString& str, html::Flags flags, QColor color)
{
  htmlText.append(QStringLiteral("<li>") % asText(str, flags, color) % QStringLiteral("</li>\n"));
  numLines++;
  return *this;
}

QString HtmlBuilder::asText(QString str, html::Flags flags, QColor foreground, QColor background)
{
  QString prefix, suffix;
  if(flags.testFlag(html::BOLD))
  {
    prefix.append(QStringLiteral("<b>"));
    suffix.prepend(QStringLiteral("</b>"));
  }

  if(flags.testFlag(html::ITALIC))
  {
    prefix.append(QStringLiteral("<i>"));
    suffix.prepend(QStringLiteral("</i>"));
  }

  if(flags.testFlag(html::UNDERLINE))
  {
    prefix.append(QStringLiteral("<u>"));
    suffix.prepend(QStringLiteral("</u>"));
  }

  if(flags.testFlag(html::STRIKEOUT))
  {
    prefix.append(QStringLiteral("<s>"));
    suffix.prepend(QStringLiteral("</s>"));
  }

  if(flags.testFlag(html::SUBSCRIPT))
  {
    prefix.append(QStringLiteral("<sub>"));
    suffix.prepend(QStringLiteral("</sub>"));
  }

  if(flags.testFlag(html::SUPERSCRIPT))
  {
    prefix.append(QStringLiteral("<sup>"));
    suffix.prepend(QStringLiteral("</sup>"));
  }

  if(flags.testFlag(html::SMALL))
  {
    prefix.append(QStringLiteral("<small>"));
    suffix.prepend(QStringLiteral("</small>"));
  }

  if(flags.testFlag(html::BIG))
  {
    prefix.append(QStringLiteral("<big>"));
    suffix.prepend(QStringLiteral("</big>"));
  }

  if(flags.testFlag(html::CODE))
  {
    prefix.append(QStringLiteral("<code>"));
    suffix.prepend(QStringLiteral("</code>"));
  }

  if(flags.testFlag(html::PRE))
  {
    prefix.append(QStringLiteral("<pre>"));
    suffix.prepend(QStringLiteral("</pre>"));
  }

  if(flags.testFlag(html::NOBR))
  {
    prefix.append(QStringLiteral("<nobr>"));
    suffix.prepend(QStringLiteral("</nobr>"));
  }

  if(foreground.isValid() || (background.isValid() && background != Qt::transparent))
  {
    prefix.append(QStringLiteral("<span style=\""));

    if(foreground.isValid())
      prefix.append(QStringLiteral("color:") % foreground.name(QColor::HexRgb));

    if(background.isValid() && background != Qt::transparent)
    {
      if(foreground.isValid())
        prefix.append(QStringLiteral("; "));
      prefix.append(QStringLiteral("background-color:") % background.name(QColor::HexRgb));
    }

    prefix.append(QStringLiteral("\">"));
    suffix.prepend(QStringLiteral("</span>"));
  }

  if(!(flags.testFlag(html::NO_ENTITIES)))
    str = toEntities(str.toHtmlEscaped()).replace(QStringLiteral("\n"), QStringLiteral("<br/>"));

  if(flags.testFlag(html::REPLACE_CRLF))
  {
    str = str.replace(QStringLiteral("\r\n"), QStringLiteral("<br/>"));
    str = str.replace(QStringLiteral("\n"), QStringLiteral("<br/>"));
    str = str.replace(QStringLiteral("\r"), QStringLiteral("<br/>"));
  }

  if(flags & html::AUTOLINK)
    str.replace(LINK_REGEXP, QStringLiteral("<a href=\"\\1\">\\1</a>"));

  return prefix % str % suffix;
}

bool HtmlBuilder::checklength(int maxLines, const QString& msg)
{
  QString dotText(QStringLiteral("<b>%1</b>").arg(msg));
  if(numLines > maxLines)
  {
    if(!htmlText.endsWith(dotText))
      hr().b(msg);
    return true;
  }
  return false;
}

bool HtmlBuilder::checklengthTextBar(int maxLines, const QString& msg, int length)
{
  QString dotText(QStringLiteral("<b>%1</b>").arg(msg));
  if(numLines > maxLines)
  {
    if(!htmlText.endsWith(dotText))
      textBar(length).b(msg);
    return true;
  }
  return false;
}

HtmlBuilder& HtmlBuilder::textBar(int length, html::Flags flags, QColor color)
{
  QString str;
  str.resize(length, QChar(L'—'));
  text(str, flags, color).br();
  return *this;
}

HtmlBuilder& HtmlBuilder::text(const QString& str, html::Flags flags, QColor color)
{
  htmlText.append(asText(str, flags, color));
  return *this;
}

QString HtmlBuilder::textStr(const QString& str, html::Flags flags, QColor color)
{
  return asText(str, flags, color);
}

HtmlBuilder& HtmlBuilder::textHtml(const HtmlBuilder& other)
{
  text(other.getHtml(), html::NO_ENTITIES);
  return *this;
}

HtmlBuilder& HtmlBuilder::doc(const QString& title, const QString& css, const QString& bodyStyle,
                              const QStringList& headerLines)
{
  htmlText.append(
    "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
      "<html>\n"
        "<head>\n");

  if(!css.isEmpty())
    htmlText.append(QStringLiteral("<style type=\"text/css\" xml:space=\"preserve\">\n%1</style>\n").arg(css));

  if(!title.isEmpty())
    htmlText.append(QStringLiteral("<title>%1</title>\n").arg(title));

  // Other header lines like "meta"
  for(const QString& line : headerLines)
    htmlText.append(line);

  // <link rel="stylesheet" href="css/style.css" type="text/css" />
  htmlText.append(QStringLiteral("</head>\n"));

  if(!bodyStyle.isEmpty())
    htmlText.append(QStringLiteral("<body style=\"") % bodyStyle % QStringLiteral("\">\n"));
  else
    htmlText.append(QStringLiteral("<body>\n"));

  tableRowsCur = 0;
  markIndex = -1;
  return *this;
}

HtmlBuilder& HtmlBuilder::docEnd()
{
  htmlText.append(QStringLiteral("</body>\n</html>\n"));
  return *this;
}

const QString& HtmlBuilder::alt(const QStringList& list) const
{
  return list.at(tableRowsCur % list.size());
}

QString HtmlBuilder::getEncodedImageHref(const QIcon& icon, QSize imageSize)
{
  QByteArray data;
  QBuffer buffer(&data);
  icon.pixmap(imageSize).save(&buffer, "PNG", 100);

  return QStringLiteral("data:image/png;base64, %1").arg(QString(data.toBase64()));
}

QString HtmlBuilder::toEntities(const QString& src)
{
  QString tmp(src);
  int i = 0, len = tmp.length();
  while(i < len)
  {
    if(tmp.at(i).unicode() > 128)
    {
      QString rp = QStringLiteral("&#") + QString::number(tmp[i].unicode()) + QStringLiteral(";");
      tmp.replace(i, 1, rp);
      len += rp.length() - 1;
      i += rp.length();
    }
    else
      i++;
  }
  return tmp;
}

void HtmlBuilder::setIdBits(const QBitArray& value)
{
  idBits.fill(false, MAX_ID + 1);
  idBits |= value;
}

} // namespace util
} // namespace atools
