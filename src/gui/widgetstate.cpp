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

#include "gui/widgetstate.h"

#include "logging/loggingdefs.h"
#include "settings/settings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListView>
#include <QMainWindow>
#include <QMenu>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTableView>
#include <QTextEdit>

namespace atools {
namespace gui {

using atools::settings::Settings;

WidgetState::WidgetState(const QString& settingsKeyPrefix, bool saveVisibility)
  : keyPrefix(settingsKeyPrefix), visibility(saveVisibility)
{

}

void WidgetState::save(const QObject *widget)
{
  if(widget != nullptr)
  {
    Settings& s = Settings::instance();

    if(const QLayout * layout = dynamic_cast<const QLayout *>(widget))
      for(int i = 0; i < layout->count(); i++)
        save(layout->itemAt(i)->widget());
    else if(const QLineEdit * le = dynamic_cast<const QLineEdit *>(widget))
    {
      saveWidget(s, le, le->text());
      saveWidgetVisible(s, le);
    }
    else if(const QSpinBox * sb = dynamic_cast<const QSpinBox *>(widget))
    {
      saveWidget(s, sb, sb->value());
      saveWidgetVisible(s, sb);
    }
    else if(const QDoubleSpinBox * dsb = dynamic_cast<const QDoubleSpinBox *>(widget))
    {
      saveWidget(s, dsb, dsb->value());
      saveWidgetVisible(s, dsb);
    }
    else if(const QComboBox * cb = dynamic_cast<const QComboBox *>(widget))
    {
      saveWidget(s, cb, cb->currentIndex());
      saveWidgetVisible(s, cb);
    }
    else if(const QAbstractSlider * sl = dynamic_cast<const QAbstractSlider *>(widget))
    {
      saveWidget(s, sl, sl->value());
      saveWidgetVisible(s, sl);
    }
    else if(const QTabWidget * tw = dynamic_cast<const QTabWidget *>(widget))
      saveWidget(s, tw, tw->currentIndex());
    else if(const QTabBar * tb = dynamic_cast<const QTabBar *>(widget))
      saveWidget(s, tb, tb->currentIndex());
    else if(const QAction * a = dynamic_cast<const QAction *>(widget))
    {
      if(a->isCheckable())
        saveWidget(s, a, a->isChecked());
    }
    else if(const QHeaderView * hv = dynamic_cast<const QHeaderView *>(widget))
      saveWidget(s, hv, hv->saveState());
    else if(const QTableView * tv = dynamic_cast<const QTableView *>(widget))
      saveWidget(s, tv, tv->horizontalHeader()->saveState());
    else if(const QFileDialog * fd = dynamic_cast<const QFileDialog *>(widget))
      saveWidget(s, fd, fd->saveState());
    else if(const QMainWindow * mw = dynamic_cast<const QMainWindow *>(widget))
    {
      s->setValue(keyPrefix + "_" + mw->objectName() + "_size", mw->size());
      saveWidget(s, mw, mw->saveState());
    }
    else if(const QSplitter * sp = dynamic_cast<const QSplitter *>(widget))
      saveWidget(s, sp, sp->saveState());
    else if(const QStatusBar * stb = dynamic_cast<const QStatusBar *>(widget))
      saveWidget(s, stb, !stb->isHidden());
    else if(const QCheckBox * cbx = dynamic_cast<const QCheckBox *>(widget))
    {
      saveWidget(s, cbx, cbx->checkState());
      saveWidgetVisible(s, cbx);
    }
    else if(const QAbstractButton * b = dynamic_cast<const QAbstractButton *>(widget))
    {
      if(b->isCheckable())
        saveWidget(s, b, b->isChecked());
      saveWidgetVisible(s, b);
    }
    else if(const QFrame * f = dynamic_cast<const QFrame *>(widget))
      saveWidgetVisible(s, f);
    else
      qWarning() << "Found unsupported widet type in save" << widget->metaObject()->className();
  }
  else
    qWarning() << "Found null widget in save";
}

void WidgetState::restore(QObject *widget)
{
  if(widget != nullptr)
  {
    Settings& s = Settings::instance();

    if(const QLayout * layout = dynamic_cast<const QLayout *>(widget))
      for(int i = 0; i < layout->count(); i++)
        restore(layout->itemAt(i)->widget());
    else if(QLineEdit * le = dynamic_cast<QLineEdit *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        le->setText(v.toString());
      loadWidgetVisible(s, le);
    }
    else if(QSpinBox * sb = dynamic_cast<QSpinBox *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        sb->setValue(v.toInt());
      loadWidgetVisible(s, sb);
    }
    else if(QDoubleSpinBox * dsb = dynamic_cast<QDoubleSpinBox *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        dsb->setValue(v.toDouble());
      loadWidgetVisible(s, dsb);
    }
    else if(QComboBox * cb = dynamic_cast<QComboBox *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        cb->setCurrentIndex(v.toInt());
      loadWidgetVisible(s, cb);
    }
    else if(QAbstractSlider * sl = dynamic_cast<QAbstractSlider *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        sl->setValue(v.toInt());
      loadWidgetVisible(s, sl);
    }
    else if(QTabWidget * tw = dynamic_cast<QTabWidget *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        tw->setCurrentIndex(v.toInt());
    }
    else if(QTabBar * tb = dynamic_cast<QTabBar *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        tb->setCurrentIndex(v.toInt());
    }
    else if(QAction * a = dynamic_cast<QAction *>(widget))
    {
      if(a->isCheckable())
      {
        QVariant v = loadWidget(s, widget);
        if(v.isValid())
          a->setChecked(v.toBool());
      }
    }
    else if(QHeaderView * hv = dynamic_cast<QHeaderView *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        hv->restoreState(v.toByteArray());
    }
    else if(QTableView * tv = dynamic_cast<QTableView *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        tv->horizontalHeader()->restoreState(v.toByteArray());
    }
    else if(QFileDialog * fd = dynamic_cast<QFileDialog *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        fd->restoreState(v.toByteArray());
    }
    else if(QMainWindow * mw = dynamic_cast<QMainWindow *>(widget))
    {
      QVariant v = loadWidget(s, widget);

      if(v.isValid())
      {
        mw->restoreState(v.toByteArray());
        mw->resize(s->value(keyPrefix + "_" + mw->objectName() + "_size", mw->sizeHint()).toSize());
      }
    }
    else if(QSplitter * sp = dynamic_cast<QSplitter *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        sp->restoreState(v.toByteArray());
    }
    else if(QStatusBar * stb = dynamic_cast<QStatusBar *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        stb->setHidden(!v.toBool());
    }
    else if(QCheckBox * cbx = dynamic_cast<QCheckBox *>(widget))
    {
      QVariant v = loadWidget(s, widget);

      if(v.isValid())
        cbx->setCheckState(static_cast<Qt::CheckState>(v.toInt()));

      loadWidgetVisible(s, cbx);
    }
    else if(QAbstractButton * b = dynamic_cast<QAbstractButton *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(b->isCheckable() && v.isValid())
        b->setChecked(v.toBool());

      loadWidgetVisible(s, b);
    }
    else if(QFrame * f = dynamic_cast<QFrame *>(widget))
      loadWidgetVisible(s, f);
    else
      qWarning() << "Found unsupported widet type in load" << widget->metaObject()->className();
  }
  else
    qWarning() << "Found null widget in load";
}

void WidgetState::syncSettings()
{
  Settings::instance().syncSettings();
}

void WidgetState::save(const QList<QObject *>& widgets)
{
  for(const QObject *w : widgets)
    save(w);
}

void WidgetState::restore(const QList<QObject *>& widgets)
{
  for(QObject *w : widgets)
    restore(w);
}

void WidgetState::saveWidgetVisible(Settings& settings, const QWidget *w)
{
  if(visibility)
  {
    if(!w->objectName().isEmpty())
    {
      if(!w->isVisible())
      {
        qDebug() << "saving visibility" << w->objectName();
        settings->setValue(keyPrefix + "_visible_" + w->objectName(), w->isVisible());
      }
    }
    else
      qWarning() << "Found widget with empty name";
  }
}

void WidgetState::saveWidget(Settings& settings, const QObject *w, const QVariant& value)
{
  if(!w->objectName().isEmpty())
  {
    qDebug() << "saving" << w->objectName() << "value" << value;
    settings->setValue(keyPrefix + "_" + w->objectName(), value);
  }
  else
    qWarning() << "Found widget with empty name";
}

QVariant WidgetState::loadWidget(Settings& settings, QObject *w)
{
  if(!w->objectName().isEmpty())
  {
    QString name = keyPrefix + "_" + w->objectName();
    if(settings->contains(name))
    {
      QVariant v = settings->value(name);
      qDebug() << "loading" << w->objectName() << "value" << v;
      return v;
    }
  }
  else
    qWarning() << "Found widget with empty name";
  return QVariant();
}

void WidgetState::loadWidgetVisible(Settings& settings, QWidget *w)
{
  if(visibility)
  {
    if(!w->objectName().isEmpty())
    {
      QString name = keyPrefix + "_" + "_visible_" + w->objectName();
      if(settings->contains(name))
      {
        bool visible = settings->value(name).toBool();
        if(!visible)
        {
          qDebug() << "loading visibility" << w->objectName();
          w->setVisible(visible);
        }
      }
    }
    else
      qWarning() << "Found widget with empty name";
  }
}

} // namespace gui
} // namespace atools
