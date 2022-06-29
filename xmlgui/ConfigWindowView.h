/*
 * Copyright 2022 The University of Adelaide
 *
 * This file is part of CacheFX.
 *
 * Created on: Jan 24, 2020
 *     Author: thomas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CONFIGWINDOWVIEW_H
#define CONFIGWINDOWVIEW_H

#include <QObject>
#include <QWidget>
#include <memory>

class QGroupBox;
class QPushButton;
class QListView;
class QStringListModel;
class QSpinBox;
class QComboBox;
class QLabel;
class QTreeView;
class QCloseEvent;
class ConfigWindowPresenter;
class QStandardItemModel;

class ConfigWindowView : public QWidget
{
  Q_OBJECT
public:
  explicit ConfigWindowView(QWidget* parent = 0);
  virtual ~ConfigWindowView();

  void setListCaches(QStringListModel* cacheList);
  void setListCacheTypes(QStringListModel* cacheTypeList);
  void setCachePropertyTable(QStandardItemModel* propertyTable);
  void setListCacheSelected(int row);
  void setCacheTypeSelected(int idx);
  void setNLines(int value);
  int getNLines();
  void registerPresenter(ConfigWindowPresenter* presenter);
  int getSelectedCacheIdx();
  QString getSelectedReplAlgorithm();
  void setSelectedReplAlgorithm(QString algo);
  void installReplAlgComboBox(int row, QStringListModel* replAlgList,
                              ConfigWindowPresenter* presenter);
  virtual void closeEvent(QCloseEvent* event);

signals:
  void saveConfigFile();

private:
  QGroupBox* qBoxCaches;
  QPushButton* qButtonUp;
  QPushButton* qButtonDown;
  QPushButton* qButtonRemove;
  QPushButton* qButtonNew;
  QListView* qListCaches;

  QGroupBox* qBoxCacheConfig;
  QLabel* qLabelNLines;
  QLabel* qLabelNLinesData;
  QLabel* qLabelCacheType;
  QComboBox* qComboCacheType;
  QLabel* qLabelProperties;
  QTreeView* qTreeProperties;
  QComboBox* qComboReplAlg;
};

#endif // CONFIGWINDOWVIEW_H
