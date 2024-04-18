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

#include "gui/widgetstate.h"

#include "atools.h"
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
#include <QButtonGroup>
#include <QStringBuilder>

namespace atools {
namespace gui {

using atools::settings::Settings;

WidgetState::WidgetState(const QString& settingsKeyPrefix, bool saveVisibility, bool blockSignals)
  : keyPrefix(settingsKeyPrefix), visibility(saveVisibility), block(blockSignals)
{

}

void WidgetState::save(const QList<QObject *>& widgets) const
{
  QList<const QObject *> constWidgets;
  atools::convertList(constWidgets, widgets);
  save(constWidgets);
}

void WidgetState::save(const QObject *widget) const
{
  if(widget != nullptr)
  {
    Settings& settings = Settings::instance();

    if(const QLayout *layout = dynamic_cast<const QLayout *>(widget))
    {
      for(int i = 0; i < layout->count(); i++)
      {
        save(layout->itemAt(i)->widget());
        save(layout->itemAt(i)->layout());
      }
    }
    else if(const QLineEdit *lineEdit = dynamic_cast<const QLineEdit *>(widget))
    {
      saveWidget(settings, lineEdit, lineEdit->text());
      saveWidgetVisible(settings, lineEdit);
    }
    else if(const QTextEdit *textEdit = dynamic_cast<const QTextEdit *>(widget))
    {
      saveWidget(settings, textEdit, textEdit->toHtml());
      saveWidgetVisible(settings, textEdit);
    }
    else if(const QSpinBox *spinBox = dynamic_cast<const QSpinBox *>(widget))
    {
      saveWidget(settings, spinBox, spinBox->value());
      saveWidgetVisible(settings, spinBox);
    }
    else if(const QDoubleSpinBox *doubleSpingBox = dynamic_cast<const QDoubleSpinBox *>(widget))
    {
      saveWidget(settings, doubleSpingBox, doubleSpingBox->value());
      saveWidgetVisible(settings, doubleSpingBox);
    }
    else if(const QComboBox *comboBox = dynamic_cast<const QComboBox *>(widget))
    {
      saveWidget(settings, comboBox, comboBox->currentIndex());
      if(comboBox->isEditable())
        saveWidget(settings, comboBox->lineEdit(), comboBox->lineEdit()->text(), comboBox->objectName() % "_Edit");
      saveWidgetVisible(settings, comboBox);
    }
    else if(const QAbstractSlider *slider = dynamic_cast<const QAbstractSlider *>(widget))
    {
      saveWidget(settings, slider, slider->value());
      saveWidget(settings, slider, slider->minimum(), slider->objectName() % "_Min");
      saveWidget(settings, slider, slider->maximum(), slider->objectName() % "_Max");
      saveWidgetVisible(settings, slider);
    }
    else if(const QTabWidget *tabWidget = dynamic_cast<const QTabWidget *>(widget))
      saveWidget(settings, tabWidget, tabWidget->currentIndex());
    else if(const QTabBar *tabBar = dynamic_cast<const QTabBar *>(widget))
      saveWidget(settings, tabBar, tabBar->currentIndex());
    else if(const QAction *action = dynamic_cast<const QAction *>(widget))
    {
      if(action->isCheckable())
        saveWidget(settings, action, action->isChecked());
    }
    else if(const QActionGroup *actionGroup = dynamic_cast<const QActionGroup *>(widget))
    {
      QStringList actions;
      const QList<QAction *> acts = actionGroup->actions();
      for(const QAction *act : acts)
      {
        if(act->isChecked())
          actions.append(act->objectName());
      }
      actions.removeAll(QString());
      saveWidget(settings, actionGroup, actions);
    }
    else if(const QHeaderView *headerView = dynamic_cast<const QHeaderView *>(widget))
      saveWidget(settings, headerView, headerView->saveState());
    else if(const QTableView *tableView = dynamic_cast<const QTableView *>(widget))
      saveWidget(settings, tableView, tableView->horizontalHeader()->saveState());
    else if(const QTableWidget *tableWidget = dynamic_cast<const QTableWidget *>(widget))
      saveWidget(settings, tableWidget, tableWidget->horizontalHeader()->saveState());
    else if(const QListView *listView = dynamic_cast<const QListView *>(widget))
    {
      QItemSelectionModel *sm = listView->selectionModel();
      if(sm != nullptr)
      {
        QVariantList varList;
        QModelIndex currentIndex = listView->currentIndex();
        if(currentIndex.isValid())
          varList << currentIndex.row() << currentIndex.column();
        else
          varList << -1 << -1;

        const QModelIndexList idxs = sm->selectedIndexes();
        for(const QModelIndex& index : idxs)
          varList << index.row() << index.column();
        saveWidget(settings, listView, varList);
      }
    }
    else if(const QTreeView *treeView = dynamic_cast<const QTreeView *>(widget))
      saveWidget(settings, treeView, treeView->header()->saveState());
    else if(const QTreeWidget *treeWidget = dynamic_cast<const QTreeWidget *>(widget))
      saveWidget(settings, treeWidget, treeWidget->header()->saveState());
    else if(const QFileDialog *fileDialog = dynamic_cast<const QFileDialog *>(widget))
      saveWidget(settings, fileDialog, fileDialog->saveState());
    else if(const QMainWindow *mainWindow = dynamic_cast<const QMainWindow *>(widget))
    {
      settings.setValueVar(keyPrefix % "_" % mainWindow->objectName() % "_pos", mainWindow->pos());
      settings.setValueVar(keyPrefix % "_" % mainWindow->objectName() % "_size", mainWindow->size());
      settings.setValueVar(keyPrefix % "_" % mainWindow->objectName() % "_maximized", mainWindow->isMaximized());
      saveWidget(settings, mainWindow, mainWindow->saveState());
    }
    else if(const QDialog *dialog = dynamic_cast<const QDialog *>(widget))
    {
      settings.setValueVar(keyPrefix % "_" % dialog->objectName() % "_size", dialog->geometry().size());
      settings.setValueVar(keyPrefix % "_" % dialog->objectName() % "_pos", dialog->geometry().topLeft());
    }
    else if(const QSplitter *splitter = dynamic_cast<const QSplitter *>(widget))
      saveWidget(settings, splitter, splitter->saveState());
    else if(const QStatusBar *statusBar = dynamic_cast<const QStatusBar *>(widget))
      saveWidget(settings, statusBar, !statusBar->isHidden());
    else if(const QCheckBox *checkBox = dynamic_cast<const QCheckBox *>(widget))
    {
      saveWidget(settings, checkBox, checkBox->checkState());
      saveWidgetVisible(settings, checkBox);
    }
    else if(const QAbstractButton *button = dynamic_cast<const QAbstractButton *>(widget))
    {
      if(button->isCheckable())
        saveWidget(settings, button, button->isChecked());
      saveWidgetVisible(settings, button);
    }
    else if(const QFrame *frame = dynamic_cast<const QFrame *>(widget))
      saveWidgetVisible(settings, frame);
    else if(const QButtonGroup *buttonGroup = dynamic_cast<const QButtonGroup *>(widget))
    {
      if(buttonGroup->checkedButton() != nullptr)
        settings.setValueVar(keyPrefix % "_" % buttonGroup->objectName() % "_selected", buttonGroup->checkedButton()->objectName());
      else
        settings.setValueVar(keyPrefix % "_" % buttonGroup->objectName() % "_selected", QString());
    }
    else
      qWarning() << Q_FUNC_INFO << "Found unsupported widget type in save" << widget->metaObject()->className();
  }
}

void WidgetState::restore(QObject *widget) const
{
  if(widget != nullptr)
  {
    if(block)
      widget->blockSignals(true);

    Settings& settings = Settings::instance();

    if(const QLayout *layout = dynamic_cast<const QLayout *>(widget))
    {
      for(int i = 0; i < layout->count(); i++)
      {
        restore(layout->itemAt(i)->widget());
        restore(layout->itemAt(i)->layout());
      }
    }
    else if(QLineEdit *lineEdit = dynamic_cast<QLineEdit *>(widget))
    {
      QVariant v = loadWidget(settings, widget);
      if(v.isValid())
        lineEdit->setText(v.toString());
      loadWidgetVisible(settings, lineEdit);
    }
    else if(QTextEdit *textEdit = dynamic_cast<QTextEdit *>(widget))
    {
      QVariant v = loadWidget(settings, widget);
      if(v.isValid())
        textEdit->setHtml(v.toString());
      loadWidgetVisible(settings, textEdit);
    }
    else if(QSpinBox *spinBox = dynamic_cast<QSpinBox *>(widget))
    {
      QVariant v = loadWidget(settings, widget);
      if(v.isValid())
        spinBox->setValue(v.toInt());
      loadWidgetVisible(settings, spinBox);
    }
    else if(QDoubleSpinBox *doubleSpinBox = dynamic_cast<QDoubleSpinBox *>(widget))
    {
      QVariant v = loadWidget(settings, widget);
      if(v.isValid())
        doubleSpinBox->setValue(v.toDouble());
      loadWidgetVisible(settings, doubleSpinBox);
    }
    else if(QComboBox *comboBox = dynamic_cast<QComboBox *>(widget))
    {
      QVariant v = loadWidget(settings, widget);
      if(v.isValid())
      {
        if(comboBox->isEditable() && !block)
        {
          // Force signal on editable combo boxes when applying index below
          comboBox->blockSignals(true);
          comboBox->setCurrentIndex(-1);
          comboBox->blockSignals(false);
        }
        comboBox->setCurrentIndex(v.toInt());
      }

      if(comboBox->isEditable())
      {
        QVariant ev = loadWidget(settings, comboBox->lineEdit(), comboBox->objectName() % "_Edit");
        if(ev.isValid())
          comboBox->setEditText(ev.toString());
      }

      loadWidgetVisible(settings, comboBox);
    }
    else if(QAbstractSlider *slider = dynamic_cast<QAbstractSlider *>(widget))
    {
      QVariant v = loadWidget(settings, widget);
      if(v.isValid())
        slider->setValue(v.toInt());

      v = loadWidget(settings, slider, slider->objectName() % "_Min");
      if(v.isValid())
        slider->setMinimum(v.toInt());

      v = loadWidget(settings, slider, slider->objectName() % "_Max");
      if(v.isValid())
        slider->setMaximum(v.toInt());

      loadWidgetVisible(settings, slider);
    }
    else if(QTabWidget *tabWidget = dynamic_cast<QTabWidget *>(widget))
    {
      QVariant v = loadWidget(settings, widget);
      if(v.isValid())
        tabWidget->setCurrentIndex(v.toInt());
    }
    else if(QTabBar *tabBar = dynamic_cast<QTabBar *>(widget))
    {
      QVariant v = loadWidget(settings, widget);
      if(v.isValid())
        tabBar->setCurrentIndex(v.toInt());
    }
    else if(QAction *action = dynamic_cast<QAction *>(widget))
    {
      if(action->isCheckable())
      {
        QVariant v = loadWidget(settings, widget);
        if(v.isValid())
          action->setChecked(v.toBool());
      }
    }
    else if(QActionGroup *actionGroup = dynamic_cast<QActionGroup *>(widget))
    {
      QVariant v = loadWidget(settings, actionGroup);
      if(v.isValid())
      {
        QStringList actions(v.toStringList());
        const QList<QAction *> acts = actionGroup->actions();
        for(QAction *act : acts)
        {
          if(actions.contains(act->objectName()))
            act->setChecked(true);
        }
      }
    }
    else if(QHeaderView *headerView = dynamic_cast<QHeaderView *>(widget))
    {
      QVariant v = loadWidget(settings, widget);
      if(v.isValid())
        headerView->restoreState(v.toByteArray());
    }
    else if(QTableView *tableView = dynamic_cast<QTableView *>(widget))
    {
      QVariant v = loadWidget(settings, widget);
      if(v.isValid())
        tableView->horizontalHeader()->restoreState(v.toByteArray());
    }
    else if(QTableWidget *tableWidget = dynamic_cast<QTableWidget *>(widget))
    {
      QVariant v = loadWidget(settings, widget);
      if(v.isValid())
        tableWidget->horizontalHeader()->restoreState(v.toByteArray());
    }
    else if(QListView *listView = dynamic_cast<QListView *>(widget))
    {
      QVariant var = loadWidget(settings, widget);
      if(var.isValid())
      {
        QItemSelectionModel *sm = listView->selectionModel();

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
                listView->setCurrentIndex(listView->model()->index(row, col));
            }
            else
            {
              sm->select(listView->model()->index(row, col), QItemSelectionModel::Select);
            }
          }
        }
      }
    }
    else if(QTreeView *treeView = dynamic_cast<QTreeView *>(widget))
    {
      QVariant v = loadWidget(settings, widget);
      if(v.isValid())
        treeView->header()->restoreState(v.toByteArray());
    }
    else if(QTreeWidget *treeWidget = dynamic_cast<QTreeWidget *>(widget))
    {
      QVariant v = loadWidget(settings, widget);
      if(v.isValid())
        treeWidget->header()->restoreState(v.toByteArray());
    }
    else if(QFileDialog *fileDialog = dynamic_cast<QFileDialog *>(widget))
    {
      QVariant v = loadWidget(settings, widget);
      if(v.isValid())
        fileDialog->restoreState(v.toByteArray());
    }
    else if(QMainWindow *mainWindow = dynamic_cast<QMainWindow *>(widget))
    {
      QVariant v = loadWidget(settings, mainWindow);

      if(v.isValid())
      {
        mainWindow->restoreState(v.toByteArray());

        if(restoreMainWindowPos)
        {
          QString key = keyPrefix % "_" % mainWindow->objectName() % "_pos";
          if(settings.contains(key))
            mainWindow->move(settings.valueVar(key, mainWindow->pos()).toPoint());
        }

        if(restoreMainWindowSize)
        {
          QString key = keyPrefix % "_" % mainWindow->objectName() % "_size";
          if(settings.contains(key))
            mainWindow->resize(settings.valueVar(key, mainWindow->sizeHint()).toSize());
        }

        if(restoreMainWindowState)
          if(settings.valueVar(keyPrefix % "_" % mainWindow->objectName() % "_maximized", false).toBool())
            mainWindow->setWindowState(mainWindow->windowState() | Qt::WindowMaximized);
      }
    }
    else if(QDialog *dialog = dynamic_cast<QDialog *>(widget))
    {
      if(restoreDialogSize || restoreDialogPos)
      {
        QString keyPos = keyPrefix % "_" % dialog->objectName() % "_pos";
        QString keySize = keyPrefix % "_" % dialog->objectName() % "_size";

        QPoint pos = restoreDialogPos ? settings.valueVar(keyPos, dialog->geometry().topLeft()).toPoint() : dialog->geometry().topLeft();
        QSize size = restoreDialogSize ? settings.valueVar(keySize, dialog->geometry().size()).toSize() : dialog->geometry().size();

        dialog->setGeometry(QRect(pos, size));
      }
    }
    else if(QSplitter *splitter = dynamic_cast<QSplitter *>(widget))
    {
      QVariant v = loadWidget(settings, widget);
      if(v.isValid())
        splitter->restoreState(v.toByteArray());
    }
    else if(QStatusBar *statusBar = dynamic_cast<QStatusBar *>(widget))
    {
      QVariant v = loadWidget(settings, widget);
      if(v.isValid())
        statusBar->setHidden(!v.toBool());
    }
    else if(QCheckBox *checkBox = dynamic_cast<QCheckBox *>(widget))
    {
      QVariant v = loadWidget(settings, widget);

      if(v.isValid())
        checkBox->setCheckState(static_cast<Qt::CheckState>(v.toInt()));

      loadWidgetVisible(settings, checkBox);
    }
    else if(QAbstractButton *button = dynamic_cast<QAbstractButton *>(widget))
    {
      QVariant v = loadWidget(settings, widget);
      if(button->isCheckable() && v.isValid())
        button->setChecked(v.toBool());

      loadWidgetVisible(settings, button);
    }
    else if(QFrame *frame = dynamic_cast<QFrame *>(widget))
      loadWidgetVisible(settings, frame);
    else if(const QButtonGroup *buttonGroup = dynamic_cast<const QButtonGroup *>(widget))
    {
      QString key = keyPrefix % "_" % buttonGroup->objectName() % "_selected";
      if(settings.contains(key))
      {
        QString value = settings.valueStr(key);
        if(!value.isEmpty())
        {
          const QList<QAbstractButton *> btns = buttonGroup->buttons();
          for(QAbstractButton *btn : btns)
          {
            if(btn->objectName() == value)
              btn->setChecked(true);
          }
        }
      }
    }
    else
      qWarning() << Q_FUNC_INFO << "Found unsupported widget type in load" << widget->metaObject()->className();

    if(block)
      widget->blockSignals(false);
  }
}

bool WidgetState::contains(const QObject *widget) const
{
  return containsWidget(Settings::instance(), widget);
}

QString WidgetState::getSettingsKey(const QObject *widget) const
{
  return keyPrefix % "_" % widget->objectName();
}

void WidgetState::syncSettings()
{
  Settings::syncSettings();
}

void WidgetState::save(const QList<const QObject *>& widgets) const
{
  for(const QObject *w : widgets)
    save(w);
}

void WidgetState::restore(const QList<QObject *>& widgets) const
{
  for(QObject *w : widgets)
    restore(w);
}

void WidgetState::saveWidgetVisible(Settings& settings, const QWidget *widget) const
{
  if(visibility)
  {
    if(!widget->objectName().isEmpty())
    {
      if(!widget->isVisible())
        settings.setValue(keyPrefix % "_visible_" % widget->objectName(), widget->isVisible());
    }
    else
      qWarning() << Q_FUNC_INFO << "Found widget with empty name";
  }
}

void WidgetState::saveWidget(Settings& settings, const QObject *object, const QVariant& value, const QString& objName) const
{
  QString name = objName.isEmpty() ? object->objectName() : objName;
  if(!name.isEmpty())
    settings.setValueVar(keyPrefix % "_" % name, value);
  else
    qWarning() << Q_FUNC_INFO << "Found widget with empty name";
}

bool WidgetState::containsWidget(Settings& settings, const QObject *object, const QString& objName) const
{
  QString name = objName.isEmpty() ? object->objectName() : objName;
  if(!name.isEmpty())
  {
    if(dynamic_cast<const QDialog *>(object) != nullptr)
      // Need to check for size since dialogs are stored using only this key
      return settings.contains(keyPrefix % "_" % name % "_size") || settings.contains(keyPrefix % "_" % name % "_pos");
    else
      return settings.contains(keyPrefix % "_" % name);
  }
  else
    qWarning() << Q_FUNC_INFO << "Found widget with empty name";
  return false;
}

QVariant WidgetState::loadWidget(Settings& settings, QObject *object, const QString& objName) const
{
  QString oname = objName.isEmpty() ? object->objectName() : objName;
  if(!oname.isEmpty())
  {
    QString name = keyPrefix % "_" % oname;
    if(settings.contains(name))
      return settings.valueVar(name);
  }
  else
    qWarning() << Q_FUNC_INFO << "Found widget with empty name";
  return QVariant();
}

void WidgetState::loadWidgetVisible(Settings& settings, QWidget *widget) const
{
  if(visibility)
  {
    if(!widget->objectName().isEmpty())
    {
      QString name = keyPrefix % "_" % "_visible_" % widget->objectName();
      if(settings.contains(name))
      {
        bool visible = settings.valueBool(name);
        if(!visible)
          widget->setVisible(visible);
      }
    }
    else
      qWarning() << Q_FUNC_INFO << "Found widget with empty name";
  }
}

} // namespace gui
} // namespace atools
