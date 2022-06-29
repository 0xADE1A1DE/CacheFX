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

#ifndef CONFIGWINDOWPRESENTER_H_
#define CONFIGWINDOWPRESENTER_H_

#include <Cache/Cache.h>
#include <Cache/CacheHierarchy.h>
#include <CacheFactory/CacheFactory.h>
#include <QModelIndexList>
#include <QObject>
#include <memory>

class ConfigWindowView;
class QStringListModel;
class QStandardItemModel;
class QStandardItem;
class ConfigWindowPresenter : public QObject
{
  Q_OBJECT
public:
  ConfigWindowPresenter(ConfigWindowView* v, std::string configFile);
  virtual ~ConfigWindowPresenter();

public slots:
  void upButtonClicked();
  void downButtonClicked();
  void removeButtonClicked();
  void newButtonClicked();
  void cacheListSelectionChanged(QModelIndex indexNew, QModelIndex indexOld);
  void cacheTypeSelectionChanged(int index);
  void cachePropertyChanged(QStandardItem* item);
  void algorithmSelectionChanged(QString algo);
  void saveConfigurationFile();

private:
  void initView();
  QStringList getCacheList();
  QStringList getCacheTypeList();
  QStringList getReplAlgList();
  CacheHierarchy* getCacheHierarchy();
  void updateCacheDescription();
  void updateCacheModel();
  void createPropertyTableModel(Cache* cache);

private:
  ConfigWindowView* view;
  std::unique_ptr<Cache> cacheHierarchy;
  QStringListModel* cacheListModel;
  QStringListModel* cacheTypeListModel;
  QStringListModel* replAlgListModel;
  QStandardItemModel* cachePropertiesModel;
  int selectedCacheIndex;

  std::map<replAlg, std::string> repAlgStrMap;
  std::map<std::string, replAlg> strRepAlgMap;

  std::string cfgFilePath;

  static const char* PROPERTY_CACHE_NSETS;
  static const char* PROPERTY_CACHE_NWAYS;
  static const char* PROPERTY_CACHE_NSECWAYS;
  static const char* PROPERTY_CACHE_REPLALG;
  static const char* PROPERTY_CACHE_NBITS;
  static const char* PROPERTY_CACHE_KBITS;
  static const char* PROPERTY_CACHE_NPARTITIONS;
  static const char* PROPERTY_CACHE_NRANDOMSETS;
};

#endif /* CONFIGWINDOWPRESENTER_H_ */
