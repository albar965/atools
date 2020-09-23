/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#include "settings/settings.h"

#include <QDebug>
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
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTableView>
#include <QTreeView>
#include <QTextEdit>
#include <QActionGroup>
#include <QTableWidget>
#include <QTreeWidget>
#include <QListWidget>

namespace atools {
namespace gui {

using atools::settings::Settings;

WidgetState::WidgetState(const QString& settingsKeyPrefix, bool saveVisibility, bool blockSignals)
  : keyPrefix(settingsKeyPrefix), visibility(saveVisibility), block(blockSignals)
{

}

void WidgetState::save(const QObject *widget) const
{
  if(widget != nullptr)
  {
    Settings& s = Settings::instance();

    if(const QLayout *layout = dynamic_cast<const QLayout *>(widget))
    {
      for(int i = 0; i < layout->count(); i++)
        save(layout->itemAt(i)->widget());
    }
    else if(const QLineEdit *le = dynamic_cast<const QLineEdit *>(widget))
    {
      saveWidget(s, le, le->text());
      saveWidgetVisible(s, le);
    }
    else if(const QTextEdit *te = dynamic_cast<const QTextEdit *>(widget))
    {
      saveWidget(s, te, te->toHtml());
      saveWidgetVisible(s, te);
    }
    else if(const QSpinBox *sb = dynamic_cast<const QSpinBox *>(widget))
    {
      saveWidget(s, sb, sb->value());
      saveWidgetVisible(s, sb);
    }
    else if(const QDoubleSpinBox *dsb = dynamic_cast<const QDoubleSpinBox *>(widget))
    {
      saveWidget(s, dsb, dsb->value());
      saveWidgetVisible(s, dsb);
    }
    else if(const QComboBox *cb = dynamic_cast<const QComboBox *>(widget))
    {
      saveWidget(s, cb, cb->currentIndex());
      if(cb->isEditable())
        saveWidget(s, cb->lineEdit(), cb->lineEdit()->text(), cb->objectName() + "_Edit");
      saveWidgetVisible(s, cb);
    }
    else if(const QAbstractSlider *sl = dynamic_cast<const QAbstractSlider *>(widget))
    {
      saveWidget(s, sl, sl->value());
      saveWidget(s, sl, sl->minimum(), sl->objectName() + "_Min");
      saveWidget(s, sl, sl->maximum(), sl->objectName() + "_Max");
      saveWidgetVisible(s, sl);
    }
    else if(const QTabWidget *tw = dynamic_cast<const QTabWidget *>(widget))
      saveWidget(s, tw, tw->currentIndex());
    else if(const QTabBar *tb = dynamic_cast<const QTabBar *>(widget))
      saveWidget(s, tb, tb->currentIndex());
    else if(const QAction *a = dynamic_cast<const QAction *>(widget))
    {
      if(a->isCheckable())
        saveWidget(s, a, a->isChecked());
    }
    else if(const QActionGroup *ag = dynamic_cast<const QActionGroup *>(widget))
    {
      QStringList actions;
      for(const QAction *a : ag->actions())
      {
        if(a->isChecked())
          actions.append(a->objectName());
      }
      actions.removeAll(QString());
      saveWidget(s, ag, actions);
    }
    else if(const QHeaderView *hv = dynamic_cast<const QHeaderView *>(widget))
      saveWidget(s, hv, hv->saveState());
    else if(const QTableView *tv = dynamic_cast<const QTableView *>(widget))
      saveWidget(s, tv, tv->horizontalHeader()->saveState());
    else if(const QTableWidget *tblw = dynamic_cast<const QTableWidget *>(widget))
      saveWidget(s, tblw, tblw->horizontalHeader()->saveState());
    else if(const QListView *lv = dynamic_cast<const QListView *>(widget))
    {
      QItemSelectionModel *sm = lv->selectionModel();
      if(sm != nullptr)
      {
        QVariantList varList;
        QModelIndex currentIndex = lv->currentIndex();
        if(currentIndex.isValid())
          varList << currentIndex.row() << currentIndex.column();
        else
          varList << -1 << -1;

        for(const QModelIndex& index : sm->selectedIndexes())
          varList << index.row() << index.column();
        saveWidget(s, lv, varList);
      }
    }
    else if(const QTreeView *trv = dynamic_cast<const QTreeView *>(widget))
      saveWidget(s, trv, trv->header()->saveState());
    else if(const QTreeWidget *trw = dynamic_cast<const QTreeWidget *>(widget))
      saveWidget(s, trw, trw->header()->saveState());
    else if(const QFileDialog *fd = dynamic_cast<const QFileDialog *>(widget))
      saveWidget(s, fd, fd->saveState());
    else if(const QMainWindow *mw = dynamic_cast<const QMainWindow *>(widget))
    {
      s.setValueVar(keyPrefix + "_" + mw->objectName() + "_pos", mw->pos());
      s.setValueVar(keyPrefix + "_" + mw->objectName() + "_size", mw->size());
      s.setValueVar(keyPrefix + "_" + mw->objectName() + "_maximized", mw->isMaximized());
      saveWidget(s, mw, mw->saveState());
    }
    else if(const QDialog *dlg = dynamic_cast<const QDialog *>(widget))
    {
      // s.setValueVar(keyPrefix + "_" + dlg->objectName() + "_pos", dlg->pos());
      s.setValueVar(keyPrefix + "_" + dlg->objectName() + "_size", dlg->size());
    }
    else if(const QSplitter *sp = dynamic_cast<const QSplitter *>(widget))
      saveWidget(s, sp, sp->saveState());
    else if(const QStatusBar *stb = dynamic_cast<const QStatusBar *>(widget))
      saveWidget(s, stb, !stb->isHidden());
    else if(const QCheckBox *cbx = dynamic_cast<const QCheckBox *>(widget))
    {
      saveWidget(s, cbx, cbx->checkState());
      saveWidgetVisible(s, cbx);
    }
    else if(const QAbstractButton *b = dynamic_cast<const QAbstractButton *>(widget))
    {
      if(b->isCheckable())
        saveWidget(s, b, b->isChecked());
      saveWidgetVisible(s, b);
    }
    else if(const QFrame *f = dynamic_cast<const QFrame *>(widget))
      saveWidgetVisible(s, f);
    else
      qWarning() << Q_FUNC_INFO << "Found unsupported widet type in save" << widget->metaObject()->className();
  }
}

void WidgetState::restore(QObject *widget) const
{
  if(widget != nullptr)
  {
    if(block)
      widget->blockSignals(true);

    Settings& s = Settings::instance();

    if(const QLayout *layout = dynamic_cast<const QLayout *>(widget))
    {
      for(int i = 0; i < layout->count(); i++)
        restore(layout->itemAt(i)->widget());
    }
    else if(QLineEdit *le = dynamic_cast<QLineEdit *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        le->setText(v.toString());
      loadWidgetVisible(s, le);
    }
    else if(QTextEdit *te = dynamic_cast<QTextEdit *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        te->setHtml(v.toString());
      loadWidgetVisible(s, te);
    }
    else if(QSpinBox *sb = dynamic_cast<QSpinBox *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        sb->setValue(v.toInt());
      loadWidgetVisible(s, sb);
    }
    else if(QDoubleSpinBox *dsb = dynamic_cast<QDoubleSpinBox *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        dsb->setValue(v.toDouble());
      loadWidgetVisible(s, dsb);
    }
    else if(QComboBox *cb = dynamic_cast<QComboBox *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        cb->setCurrentIndex(v.toInt());

      if(cb->isEditable())
      {
        QVariant ev = loadWidget(s, cb->lineEdit(), cb->objectName() + "_Edit");
        if(ev.isValid())
          cb->setEditText(ev.toString());
      }

      loadWidgetVisible(s, cb);
    }
    else if(QAbstractSlider *sl = dynamic_cast<QAbstractSlider *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        sl->setValue(v.toInt());

      v = loadWidget(s, sl, sl->objectName() + "_Min");
      if(v.isValid())
        sl->setMinimum(v.toInt());

      v = loadWidget(s, sl, sl->objectName() + "_Max");
      if(v.isValid())
        sl->setMaximum(v.toInt());

      loadWidgetVisible(s, sl);
    }
    else if(QTabWidget *tw = dynamic_cast<QTabWidget *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        tw->setCurrentIndex(v.toInt());
    }
    else if(QTabBar *tb = dynamic_cast<QTabBar *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        tb->setCurrentIndex(v.toInt());
    }
    else if(QAction *a = dynamic_cast<QAction *>(widget))
    {
      if(a->isCheckable())
      {
        QVariant v = loadWidget(s, widget);
        if(v.isValid())
          a->setChecked(v.toBool());
      }
    }
    else if(QActionGroup *ag = dynamic_cast<QActionGroup *>(widget))
    {
      QVariant v = loadWidget(s, ag);
      if(v.isValid())
      {
        QStringList actions(v.toStringList());
        for(QAction *a : ag->actions())
        {
          if(actions.contains(a->objectName()))
            a->setChecked(true);
        }
      }
    }
    else if(QHeaderView *hv = dynamic_cast<QHeaderView *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        hv->restoreState(v.toByteArray());
    }
    else if(QTableView *tv = dynamic_cast<QTableView *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        tv->horizontalHeader()->restoreState(v.toByteArray());
    }
    else if(QTableWidget *taw = dynamic_cast<QTableWidget *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        taw->horizontalHeader()->restoreState(v.toByteArray());
    }
    else if(QListView *lv = dynamic_cast<QListView *>(widget))
    {
      QVariant var = loadWidget(s, widget);
      if(var.isValid())
      {
        QItemSelectionModel *sm = lv->selectionModel();

        if(sm != nullptr)
        {
          sm->clearSelection();

          QVariantList varList = var.toList();

          for(int i = 0; i < varList.size() / 2; i++)
          {
            int row = varList.value(i * 2).toInt();
            int col = varList.value(i * 2 + 1).toInt();
            if(i == 0)
            {
              if(row != -1 && col != -1)
                lv->setCurrentIndex(lv->model()->index(row, col));
            }
            else
            {
              sm->select(lv->model()->index(row, col), QItemSelectionModel::Select);
            }
          }
        }
      }
    }
    else if(QTreeView *trv = dynamic_cast<QTreeView *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        trv->header()->restoreState(v.toByteArray());
    }
    else if(QTreeWidget *trw = dynamic_cast<QTreeWidget *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        trw->header()->restoreState(v.toByteArray());
    }
    else if(QFileDialog *fd = dynamic_cast<QFileDialog *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        fd->restoreState(v.toByteArray());
    }
    else if(QMainWindow *mw = dynamic_cast<QMainWindow *>(widget))
    {
      QVariant v = loadWidget(s, widget);

      if(v.isValid())
      {
        mw->restoreState(v.toByteArray());
        if(positionRestoreMainWindow)
          mw->move(s.valueVar(keyPrefix + "_" + mw->objectName() + "_pos", mw->pos()).toPoint());
        if(sizeRestoreMainWindow)
          mw->resize(s.valueVar(keyPrefix + "_" + mw->objectName() + "_size", mw->sizeHint()).toSize());

        if(stateRestoreMainWindow)
          if(s.valueVar(keyPrefix + "_" + mw->objectName() + "_maximized", false).toBool())
            mw->setWindowState(mw->windowState() | Qt::WindowMaximized);
      }
    }
    else if(QDialog *dlg = dynamic_cast<QDialog *>(widget))
    {
      // dlg->move(s.valueVar(keyPrefix + "_" + dlg->objectName() + "_pos", dlg->pos()).toPoint());
      dlg->resize(s.valueVar(keyPrefix + "_" + dlg->objectName() + "_size", dlg->sizeHint()).toSize());
    }
    else if(QSplitter *sp = dynamic_cast<QSplitter *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        sp->restoreState(v.toByteArray());
    }
    else if(QStatusBar *stb = dynamic_cast<QStatusBar *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(v.isValid())
        stb->setHidden(!v.toBool());
    }
    else if(QCheckBox *cbx = dynamic_cast<QCheckBox *>(widget))
    {
      QVariant v = loadWidget(s, widget);

      if(v.isValid())
        cbx->setCheckState(static_cast<Qt::CheckState>(v.toInt()));

      loadWidgetVisible(s, cbx);
    }
    else if(QAbstractButton *b = dynamic_cast<QAbstractButton *>(widget))
    {
      QVariant v = loadWidget(s, widget);
      if(b->isCheckable() && v.isValid())
        b->setChecked(v.toBool());

      loadWidgetVisible(s, b);
    }
    else if(QFrame *f = dynamic_cast<QFrame *>(widget))
      loadWidgetVisible(s, f);
    else
      qWarning() << Q_FUNC_INFO << "Found unsupported widet type in load" << widget->metaObject()->className();

    if(block)
      widget->blockSignals(false);
  }
}

bool WidgetState::contains(QObject *widget) const
{
  return containsWidget(Settings::instance(), widget);
}

QString WidgetState::getSettingsKey(QObject *widget) const
{
  return keyPrefix + "_" + widget->objectName();
}

void WidgetState::syncSettings()
{
  Settings::instance().syncSettings();
}

void WidgetState::setMainWindowsRestoreOptions(bool position, bool size, bool state)
{
  positionRestoreMainWindow = position;
  sizeRestoreMainWindow = size;
  stateRestoreMainWindow = state;
}

void WidgetState::save(const QList<QObject *>& widgets) const
{
  for(const QObject *w : widgets)
    save(w);
}

void WidgetState::restore(const QList<QObject *>& widgets) const
{
  for(QObject *w : widgets)
    restore(w);
}

void WidgetState::saveWidgetVisible(Settings& settings, const QWidget *w) const
{
  if(visibility)
  {
    if(!w->objectName().isEmpty())
    {
      if(!w->isVisible())
        settings.setValue(keyPrefix + "_visible_" + w->objectName(), w->isVisible());
    }
    else
      qWarning() << Q_FUNC_INFO << "Found widget with empty name";
  }
}

void WidgetState::saveWidget(Settings& settings, const QObject *w, const QVariant& value, const QString& objName) const
{
  QString name = objName.isEmpty() ? w->objectName() : objName;
  if(!name.isEmpty())
    settings.setValueVar(keyPrefix + "_" + name, value);
  else
    qWarning() << Q_FUNC_INFO << "Found widget with empty name";
}

bool WidgetState::containsWidget(Settings& settings, QObject *w, const QString& objName) const
{
  QString name = objName.isEmpty() ? w->objectName() : objName;
  if(!name.isEmpty())
    return settings.contains(keyPrefix + "_" + name);
  else
    qWarning() << Q_FUNC_INFO << "Found widget with empty name";
  return false;
}

QVariant WidgetState::loadWidget(Settings& settings, QObject *w, const QString& objName) const
{
  QString oname = objName.isEmpty() ? w->objectName() : objName;
  if(!oname.isEmpty())
  {
    QString name = keyPrefix + "_" + oname;
    if(settings.contains(name))
      return settings.valueVar(name);
  }
  else
    qWarning() << Q_FUNC_INFO << "Found widget with empty name";
  return QVariant();
}

void WidgetState::loadWidgetVisible(Settings& settings, QWidget *w) const
{
  if(visibility)
  {
    if(!w->objectName().isEmpty())
    {
      QString name = keyPrefix + "_" + "_visible_" + w->objectName();
      if(settings.contains(name))
      {
        bool visible = settings.valueBool(name);
        if(!visible)
          w->setVisible(visible);
      }
    }
    else
      qWarning() << Q_FUNC_INFO << "Found widget with empty name";
  }
}

} // namespace gui
} // namespace atools
