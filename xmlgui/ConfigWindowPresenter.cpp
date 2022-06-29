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

#include "ConfigWindowPresenter.h"
#include "ConfigWindowView.h"

#include <QStandardItemModel>
#include <QStringListModel>
#include <iostream>
#include <sstream>
#include <tgmath.h>

#include <Cache/AssocCache.h>
#include <Cache/CEASERCache.h>
#include <Cache/CEASERSCache.h>
#include <Cache/CacheHierarchy.h>
#include <Cache/NewCache.h>
#include <Cache/PLcache.h>
#include <Cache/PhantomCache.h>
#include <Cache/ScatterCache.h>
#include <Cache/SetAssocCache.h>
#include <Cache/WayPartitionCache.h>

const char* ConfigWindowPresenter::PROPERTY_CACHE_NWAYS = "Number of Ways";
const char* ConfigWindowPresenter::PROPERTY_CACHE_NSECWAYS =
    "Number of Secure Ways";
const char* ConfigWindowPresenter::PROPERTY_CACHE_REPLALG =
    "Replacement Algorithm";
const char* ConfigWindowPresenter::PROPERTY_CACHE_NBITS = "NBits";
const char* ConfigWindowPresenter::PROPERTY_CACHE_NSETS = "Number of Sets";
const char* ConfigWindowPresenter::PROPERTY_CACHE_KBITS = "KBits";
const char* ConfigWindowPresenter::PROPERTY_CACHE_NPARTITIONS = "Partitions";
const char* ConfigWindowPresenter::PROPERTY_CACHE_NRANDOMSETS = "NRandomSets";

ConfigWindowPresenter::ConfigWindowPresenter(ConfigWindowView* v,
                                             std::string configFile)
    : view(v),
      cacheHierarchy(std::move(
          CacheFactory::Instance()->loadConfigurationFromFile(configFile))),
      cachePropertiesModel(NULL), selectedCacheIndex(-1),
      cfgFilePath(configFile)
{

  repAlgStrMap[REPL_LRU] = "lru";
  repAlgStrMap[REPL_BIT_PLRU] = "bit-plru";
  repAlgStrMap[REPL_TREE_PLRU] = "tree-plru";
  repAlgStrMap[REPL_RANDOM] = "random";
  strRepAlgMap["lru"] = REPL_LRU;
  strRepAlgMap["random"] = REPL_RANDOM;
  strRepAlgMap["bit-plru"] = REPL_BIT_PLRU;
  strRepAlgMap["tree-plru"] = REPL_TREE_PLRU;

  initView();
}

ConfigWindowPresenter::~ConfigWindowPresenter()
{
  delete cacheListModel;
  delete cacheTypeListModel;
  delete replAlgListModel;
  delete cachePropertiesModel;
}

CacheHierarchy* ConfigWindowPresenter::getCacheHierarchy()
{
  return (CacheHierarchy*)cacheHierarchy.get();
}

QStringList ConfigWindowPresenter::getCacheList()
{
  QStringList cacheList;

  CacheHierarchy* ch = getCacheHierarchy();

  for (size_t i = 0; i < ch->getCacheLevels(); i++)
  {
    Cache* c = ch->getCache(i);
    std::stringstream ss;
    ss << c->getCacheType() << " / " << c->getNLines() << " lines";
    cacheList.append(ss.str().c_str());
  }

  return cacheList;
}

QStringList ConfigWindowPresenter::getCacheTypeList()
{
  QStringList cacheTypeList;

  cacheTypeList.append(AssocCache::CACHE_TYPESTR);
  cacheTypeList.append(AssocPLcache::CACHE_TYPESTR);
  cacheTypeList.append(NewCache::CACHE_TYPESTR);
  cacheTypeList.append(PLcache::CACHE_TYPESTR);
  cacheTypeList.append(ScatterCache::CACHE_TYPESTR);
  cacheTypeList.append(SetAssocCache::CACHE_TYPESTR);
  cacheTypeList.append(WayPartitionCache::CACHE_TYPESTR);
  cacheTypeList.append(CEASERCache::CACHE_TYPESTR);
  cacheTypeList.append(CEASERSCache::CACHE_TYPESTR);
  cacheTypeList.append(PhantomCache::CACHE_TYPESTR);

  return cacheTypeList;
}

QStringList ConfigWindowPresenter::getReplAlgList()
{
  QStringList replAlgList;

  replAlgList.append(QString::fromStdString(repAlgStrMap[replAlg::REPL_LRU]));
  replAlgList.append(
      QString::fromStdString(repAlgStrMap[replAlg::REPL_RANDOM]));
  replAlgList.append(
      QString::fromStdString(repAlgStrMap[replAlg::REPL_BIT_PLRU]));
  replAlgList.append(
      QString::fromStdString(repAlgStrMap[replAlg::REPL_TREE_PLRU]));

  return replAlgList;
}

void ConfigWindowPresenter::initView()
{
  cacheTypeListModel = new QStringListModel(getCacheTypeList());
  cacheListModel = new QStringListModel(getCacheList());
  replAlgListModel = new QStringListModel(getReplAlgList());

  view->setListCaches(cacheListModel);
  view->setListCacheTypes(cacheTypeListModel);
  view->registerPresenter(this);
}

void ConfigWindowPresenter::upButtonClicked()
{
  if (selectedCacheIndex != -1)
  {
    getCacheHierarchy()->moveCacheUp(selectedCacheIndex);
    cacheListModel->setStringList(getCacheList());
    selectedCacheIndex--;
    view->setListCacheSelected(selectedCacheIndex);
  }
}

void ConfigWindowPresenter::downButtonClicked()
{
  if (selectedCacheIndex != -1)
  {
    getCacheHierarchy()->moveCacheDown(selectedCacheIndex);
    cacheListModel->setStringList(getCacheList());
    selectedCacheIndex++;
    view->setListCacheSelected(selectedCacheIndex);
  }
}

void ConfigWindowPresenter::removeButtonClicked()
{
  if (selectedCacheIndex != -1)
  {
    CacheHierarchy* ch = getCacheHierarchy();
    ch->removeCacheLevel(selectedCacheIndex);
    cacheListModel->setStringList(getCacheList());
    if (ch->getCacheLevels() == 0)
    {
      selectedCacheIndex = -1;
      updateCacheDescription();
    }
    else
    {
      if (selectedCacheIndex >= (int)ch->getCacheLevels())
      {
        selectedCacheIndex--;
      }
      view->setListCacheSelected(selectedCacheIndex);
      updateCacheDescription();
    }
  }
}

void ConfigWindowPresenter::newButtonClicked()
{
  getCacheHierarchy()->addCacheLevel(
      std::move(CacheFactory::Instance()->createCache(
          SetAssocCache::CACHE_TYPESTR, 64, 8, REPL_LRU)));
  cacheListModel->setStringList(getCacheList());
  view->setListCacheSelected(selectedCacheIndex);
  updateCacheDescription();
}

void ConfigWindowPresenter::cacheListSelectionChanged(QModelIndex indexNew,
                                                      QModelIndex indexOld)
{
  selectedCacheIndex = indexNew.row();
  updateCacheDescription();
}

void ConfigWindowPresenter::updateCacheModel()
{
  Cache* cache = getCacheHierarchy()->getCache(selectedCacheIndex);
  std::string cacheType(cache->getCacheType());
  bool ok;

  if (cacheType == SetAssocCache::CACHE_TYPESTR)
  {
    SetAssocCache* c = (SetAssocCache*)cache;
    int nSets = cachePropertiesModel->item(0, 1)->text().toInt(&ok);
    if (!ok)
    {
      nSets = c->getNSets();
    }
    int nWays = cachePropertiesModel->item(1, 1)->text().toInt(&ok);
    if (!ok)
    {
      nWays = c->getNWays();
    }
    std::string strAlgo = view->getSelectedReplAlgorithm().toStdString();
    replAlg replAlgorithm = strRepAlgMap[strAlgo];
    c = new SetAssocCache(replAlgorithm, nSets, nWays);
    getCacheHierarchy()->replaceCacheLevel(std::move(std::unique_ptr<Cache>(c)),
                                           selectedCacheIndex);
  }
  else if (cacheType == AssocCache::CACHE_TYPESTR)
  {
    AssocCache* c = (AssocCache*)cache;
    int nLines = cachePropertiesModel->item(0, 1)->text().toInt(&ok);
    if (!ok)
    {
      nLines = c->getNLines();
    }
    std::string strAlgo = view->getSelectedReplAlgorithm().toStdString();
    replAlg replAlgorithm = strRepAlgMap[strAlgo];
    c = new AssocCache(replAlgorithm, nLines);
    getCacheHierarchy()->replaceCacheLevel(std::move(std::unique_ptr<Cache>(c)),
                                           selectedCacheIndex);
  }
  else if (cacheType == ScatterCache::CACHE_TYPESTR)
  {
    ScatterCache* c = (ScatterCache*)cache;
    int nSets = cachePropertiesModel->item(0, 1)->text().toInt(&ok);
    if (!ok)
    {
      nSets = c->getNSets();
    }
    int nWays = cachePropertiesModel->item(1, 1)->text().toInt(&ok);
    if (!ok)
    {
      nWays = c->getNWays();
    }
    c = new ScatterCache(nSets, nWays);
    getCacheHierarchy()->replaceCacheLevel(std::move(std::unique_ptr<Cache>(c)),
                                           selectedCacheIndex);
  }
  else if (cacheType == NewCache::CACHE_TYPESTR)
  {
    NewCache* c = (NewCache*)cache;
    int nBits = cachePropertiesModel->item(0, 1)->text().toInt(&ok);
    if (!ok)
    {
      nBits = c->getNBits();
    }
    int kBits = cachePropertiesModel->item(1, 1)->text().toInt(&ok);
    if (!ok)
    {
      kBits = c->getKBits();
    }
    c = new NewCache(nBits, kBits);
    getCacheHierarchy()->replaceCacheLevel(std::move(std::unique_ptr<Cache>(c)),
                                           selectedCacheIndex);
  }
  else if (cacheType == WayPartitionCache::CACHE_TYPESTR)
  {
    WayPartitionCache* c = (WayPartitionCache*)cache;
    int nSets = cachePropertiesModel->item(0, 1)->text().toInt(&ok);
    if (!nSets)
    {
      nSets = c->getNSets();
    }
    int nWays = cachePropertiesModel->item(1, 1)->text().toInt(&ok);
    if (!ok)
    {
      nWays = c->getNWays();
    }
    int nSecWays = cachePropertiesModel->item(2, 1)->text().toInt(&ok);
    if (!ok)
    {
      nSecWays = c->getNSecWays();
    }
    std::string strAlgo = view->getSelectedReplAlgorithm().toStdString();
    replAlg replAlgorithm = strRepAlgMap[strAlgo];
    c = new WayPartitionCache(replAlgorithm, nSets, nWays, nSecWays);
    getCacheHierarchy()->replaceCacheLevel(std::move(std::unique_ptr<Cache>(c)),
                                           selectedCacheIndex);
  }
  else if (cacheType == PLcache::CACHE_TYPESTR)
  {
    PLcache* c = (PLcache*)cache;
    int nSets = cachePropertiesModel->item(0, 1)->text().toInt(&ok);
    if (!ok)
    {
      nSets = c->getNSets();
    }
    int nWays = cachePropertiesModel->item(1, 1)->text().toInt(&ok);
    if (!ok)
    {
      nWays = c->getNWays();
    }
    std::string strAlgo = view->getSelectedReplAlgorithm().toStdString();
    replAlg replAlgorithm = strRepAlgMap[strAlgo];
    c = new PLcache(replAlgorithm, nSets, nWays);
    getCacheHierarchy()->replaceCacheLevel(std::move(std::unique_ptr<Cache>(c)),
                                           selectedCacheIndex);
  }
  else if (cacheType == AssocPLcache::CACHE_TYPESTR)
  {
    AssocPLcache* c = (AssocPLcache*)cache;
    int nLines = cachePropertiesModel->item(0, 1)->text().toInt(&ok);
    if (!ok)
    {
      nLines = c->getNLines();
    }
    std::string strAlgo = view->getSelectedReplAlgorithm().toStdString();
    replAlg replAlgorithm = strRepAlgMap[strAlgo];
    c = new AssocPLcache(replAlgorithm, nLines);
    getCacheHierarchy()->replaceCacheLevel(std::move(std::unique_ptr<Cache>(c)),
                                           selectedCacheIndex);
  }
  else if (cacheType == CEASERCache::CACHE_TYPESTR)
  {
    CEASERCache* c = (CEASERCache*)cache;
    int nSets = cachePropertiesModel->item(0, 1)->text().toInt(&ok);
    if (!ok)
    {
      nSets = c->getNSets();
    }
    int nWays = cachePropertiesModel->item(1, 1)->text().toInt(&ok);
    if (!ok)
    {
      nWays = c->getNWays();
    }
    std::string strAlgo = view->getSelectedReplAlgorithm().toStdString();
    replAlg replAlgorithm = strRepAlgMap[strAlgo];
    c = new CEASERCache(replAlgorithm, nSets, nWays);
    getCacheHierarchy()->replaceCacheLevel(std::move(std::unique_ptr<Cache>(c)),
                                           selectedCacheIndex);
  }
  else if (cacheType == CEASERSCache::CACHE_TYPESTR)
  {
    CEASERSCache* c = (CEASERSCache*)cache;
    int nSets = cachePropertiesModel->item(0, 1)->text().toInt(&ok);
    if (!ok)
    {
      nSets = c->getNSets();
    }
    int nWays = cachePropertiesModel->item(1, 1)->text().toInt(&ok);
    if (!ok)
    {
      nWays = c->getNWays();
    }
    int nPartitions = cachePropertiesModel->item(2, 1)->text().toInt(&ok);
    if (!ok || ((nWays % nPartitions) != 0))
    {
      nPartitions = c->getNPartitions();
    }
    c = new CEASERSCache(nSets, nWays, nPartitions);
    getCacheHierarchy()->replaceCacheLevel(std::move(std::unique_ptr<Cache>(c)),
                                           selectedCacheIndex);
  }
  else if (cacheType == PhantomCache::CACHE_TYPESTR)
  {
    PhantomCache* c = (PhantomCache*)cache;
    int nSets = cachePropertiesModel->item(0, 1)->text().toInt(&ok);
    if (!ok)
    {
      nSets = c->getNSets();
    }
    int nWays = cachePropertiesModel->item(1, 1)->text().toInt(&ok);
    if (!ok)
    {
      nWays = c->getNWays();
    }
    int nRandomSets = cachePropertiesModel->item(2, 1)->text().toInt(&ok);
    if (!ok)
    {
      nRandomSets = c->getNRandomSets();
    }
    std::string strAlgo = view->getSelectedReplAlgorithm().toStdString();
    replAlg replAlgorithm = strRepAlgMap[strAlgo];
    c = new PhantomCache(replAlgorithm, nSets, nWays, nRandomSets);
    getCacheHierarchy()->replaceCacheLevel(std::move(std::unique_ptr<Cache>(c)),
                                           selectedCacheIndex);
  }
}

void ConfigWindowPresenter::cacheTypeSelectionChanged(int index)
{
  if (selectedCacheIndex != -1)
  {
    QStringList list = cacheTypeListModel->stringList();
    std::string typeName = list[index].toStdString();

    if (typeName !=
        getCacheHierarchy()->getCache(selectedCacheIndex)->getCacheType())
    {
      getCacheHierarchy()->replaceCacheLevel(
          (CacheFactory::Instance())->createCache(typeName, 256, 16),
          selectedCacheIndex);
      cacheListModel->setStringList(getCacheList());
      updateCacheDescription();
    }
  }
}

void ConfigWindowPresenter::updateCacheDescription()
{
  if (selectedCacheIndex != -1)
  {
    Cache* c = getCacheHierarchy()->getCache(selectedCacheIndex);
    view->setCacheTypeSelected(
        cacheTypeListModel->stringList().indexOf(c->getCacheType()));
    view->setNLines(c->getNLines());
    createPropertyTableModel(c);
  }
}

void ConfigWindowPresenter::createPropertyTableModel(Cache* cache)
{
  delete cachePropertiesModel;

  cachePropertiesModel = NULL;

  std::string cacheType(cache->getCacheType());
  if (cacheType == SetAssocCache::CACHE_TYPESTR)
  {
    SetAssocCache* c = (SetAssocCache*)cache;
    QStandardItem* item;
    cachePropertiesModel = new QStandardItemModel(3, 2);
    item = new QStandardItem(PROPERTY_CACHE_NSETS);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(0, 0, item);
    item = new QStandardItem(PROPERTY_CACHE_NWAYS);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(1, 0, item);
    item = new QStandardItem(PROPERTY_CACHE_REPLALG);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(2, 0, item);
    cachePropertiesModel->setItem(
        0, 1, new QStandardItem(QString::number(c->getNSets())));
    cachePropertiesModel->setItem(
        1, 1, new QStandardItem(QString::number(c->getNWays())));
    view->setCachePropertyTable(cachePropertiesModel);
    view->installReplAlgComboBox(2, replAlgListModel, this);
    view->setSelectedReplAlgorithm(
        QString::fromStdString((repAlgStrMap[c->getAlgorithm()])));
  }
  else if (cacheType == AssocCache::CACHE_TYPESTR)
  {
    AssocCache* c = (AssocCache*)cache;
    QStandardItem* item;
    cachePropertiesModel = new QStandardItemModel(2, 2);
    item = new QStandardItem(PROPERTY_CACHE_NWAYS);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(0, 0, item);
    item = new QStandardItem(PROPERTY_CACHE_REPLALG);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(1, 0, item);
    cachePropertiesModel->setItem(
        0, 1, new QStandardItem(QString::number(c->getNLines())));
    view->setCachePropertyTable(cachePropertiesModel);
    view->installReplAlgComboBox(1, replAlgListModel, this);
    view->setSelectedReplAlgorithm(
        QString::fromStdString((repAlgStrMap[c->getAlgorithm()])));
  }
  else if (cacheType == ScatterCache::CACHE_TYPESTR)
  {
    ScatterCache* c = (ScatterCache*)cache;
    QStandardItem* item;
    cachePropertiesModel = new QStandardItemModel(2, 2);
    item = new QStandardItem(PROPERTY_CACHE_NSETS);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(0, 0, item);
    item = new QStandardItem(PROPERTY_CACHE_NWAYS);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(1, 0, item);
    cachePropertiesModel->setItem(
        0, 1, new QStandardItem(QString::number(c->getNSets())));
    cachePropertiesModel->setItem(
        1, 1, new QStandardItem(QString::number(c->getNWays())));
    view->setCachePropertyTable(cachePropertiesModel);
  }
  else if (cacheType == NewCache::CACHE_TYPESTR)
  {
    NewCache* c = (NewCache*)cache;
    QStandardItem* item;
    cachePropertiesModel = new QStandardItemModel(2, 2);
    item = new QStandardItem(PROPERTY_CACHE_NBITS);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(0, 0, item);
    item = new QStandardItem(PROPERTY_CACHE_KBITS);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(1, 0, item);
    cachePropertiesModel->setItem(
        0, 1, new QStandardItem(QString::number(c->getNBits())));
    cachePropertiesModel->setItem(
        1, 1, new QStandardItem(QString::number(c->getKBits())));
    view->setCachePropertyTable(cachePropertiesModel);
  }
  else if (cacheType == WayPartitionCache::CACHE_TYPESTR)
  {
    WayPartitionCache* c = (WayPartitionCache*)cache;
    QStandardItem* item;
    cachePropertiesModel = new QStandardItemModel(4, 2);
    item = new QStandardItem(PROPERTY_CACHE_NSETS);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(0, 0, item);
    item = new QStandardItem(PROPERTY_CACHE_NWAYS);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(1, 0, item);
    item = new QStandardItem(PROPERTY_CACHE_NSECWAYS);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(2, 0, item);
    item = new QStandardItem(PROPERTY_CACHE_REPLALG);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(3, 0, item);
    cachePropertiesModel->setItem(
        0, 1, new QStandardItem(QString::number(c->getNSets())));
    cachePropertiesModel->setItem(
        1, 1, new QStandardItem(QString::number(c->getNWays())));
    cachePropertiesModel->setItem(
        2, 1, new QStandardItem(QString::number(c->getNSecWays())));
    view->setCachePropertyTable(cachePropertiesModel);
    view->installReplAlgComboBox(3, replAlgListModel, this);
    view->setSelectedReplAlgorithm(
        QString::fromStdString((repAlgStrMap[c->getAlgorithm()])));
  }
  else if (cacheType == PLcache::CACHE_TYPESTR)
  {
    PLcache* c = (PLcache*)cache;
    QStandardItem* item;
    cachePropertiesModel = new QStandardItemModel(3, 2);
    item = new QStandardItem(PROPERTY_CACHE_NSETS);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(0, 0, item);
    item = new QStandardItem(PROPERTY_CACHE_NWAYS);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(1, 0, item);
    item = new QStandardItem(PROPERTY_CACHE_REPLALG);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(2, 0, item);
    cachePropertiesModel->setItem(
        0, 1, new QStandardItem(QString::number(c->getNSets())));
    cachePropertiesModel->setItem(
        1, 1, new QStandardItem(QString::number(c->getNWays())));
    view->setCachePropertyTable(cachePropertiesModel);
    view->installReplAlgComboBox(2, replAlgListModel, this);
    view->setSelectedReplAlgorithm(
        QString::fromStdString((repAlgStrMap[c->getAlgorithm()])));
  }
  else if (cacheType == AssocPLcache::CACHE_TYPESTR)
  {
    AssocPLcache* c = (AssocPLcache*)cache;
    QStandardItem* item;
    cachePropertiesModel = new QStandardItemModel(2, 2);
    item = new QStandardItem(PROPERTY_CACHE_NWAYS);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(0, 0, item);
    item = new QStandardItem(PROPERTY_CACHE_REPLALG);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(1, 0, item);
    cachePropertiesModel->setItem(
        0, 1, new QStandardItem(QString::number(c->getNLines())));
    view->setCachePropertyTable(cachePropertiesModel);
    view->installReplAlgComboBox(1, replAlgListModel, this);
    view->setSelectedReplAlgorithm(
        QString::fromStdString((repAlgStrMap[c->getAlgorithm()])));
  }
  else if (cacheType == CEASERCache::CACHE_TYPESTR)
  {
    CEASERCache* c = (CEASERCache*)cache;
    QStandardItem* item;
    cachePropertiesModel = new QStandardItemModel(3, 2);
    item = new QStandardItem(PROPERTY_CACHE_NSETS);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(0, 0, item);
    item = new QStandardItem(PROPERTY_CACHE_NWAYS);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(1, 0, item);
    item = new QStandardItem(PROPERTY_CACHE_REPLALG);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(2, 0, item);
    cachePropertiesModel->setItem(
        0, 1, new QStandardItem(QString::number(c->getNSets())));
    cachePropertiesModel->setItem(
        1, 1, new QStandardItem(QString::number(c->getNWays())));
    view->setCachePropertyTable(cachePropertiesModel);
    view->installReplAlgComboBox(2, replAlgListModel, this);
    view->setSelectedReplAlgorithm(
        QString::fromStdString((repAlgStrMap[c->getAlgorithm()])));
  }
  else if (cacheType == CEASERSCache::CACHE_TYPESTR)
  {
    CEASERSCache* c = (CEASERSCache*)cache;
    QStandardItem* item;
    cachePropertiesModel = new QStandardItemModel(3, 2);
    item = new QStandardItem(PROPERTY_CACHE_NSETS);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(0, 0, item);
    item = new QStandardItem(PROPERTY_CACHE_NWAYS);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(1, 0, item);
    item = new QStandardItem(PROPERTY_CACHE_NPARTITIONS);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(2, 0, item);
    cachePropertiesModel->setItem(
        0, 1, new QStandardItem(QString::number(c->getNSets())));
    cachePropertiesModel->setItem(
        1, 1, new QStandardItem(QString::number(c->getNWays())));
    cachePropertiesModel->setItem(
        2, 1, new QStandardItem(QString::number(c->getNPartitions())));
    view->setCachePropertyTable(cachePropertiesModel);
  }
  else if (cacheType == PhantomCache::CACHE_TYPESTR)
  {
    PhantomCache* c = (PhantomCache*)cache;
    QStandardItem* item;
    cachePropertiesModel = new QStandardItemModel(4, 2);
    item = new QStandardItem(PROPERTY_CACHE_NSETS);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(0, 0, item);
    item = new QStandardItem(PROPERTY_CACHE_NWAYS);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(1, 0, item);
    item = new QStandardItem(PROPERTY_CACHE_NRANDOMSETS);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(2, 0, item);
    item = new QStandardItem(PROPERTY_CACHE_REPLALG);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    cachePropertiesModel->setItem(3, 0, item);
    cachePropertiesModel->setItem(
        0, 1, new QStandardItem(QString::number(c->getNSets())));
    cachePropertiesModel->setItem(
        1, 1, new QStandardItem(QString::number(c->getNWays())));
    cachePropertiesModel->setItem(
        2, 1, new QStandardItem(QString::number(c->getNRandomSets())));
    view->setCachePropertyTable(cachePropertiesModel);
    view->installReplAlgComboBox(3, replAlgListModel, this);
    view->setSelectedReplAlgorithm(
        QString::fromStdString((repAlgStrMap[c->getAlgorithm()])));
  }
  connect(cachePropertiesModel, SIGNAL(itemChanged(QStandardItem*)), this,
          SLOT(cachePropertyChanged(QStandardItem*)));
}

void ConfigWindowPresenter::cachePropertyChanged(QStandardItem* item)
{
  updateCacheModel();
  updateCacheDescription();
  cacheListModel->setStringList(getCacheList());
}

void ConfigWindowPresenter::algorithmSelectionChanged(QString algo)
{
  std::string strAlgo = algo.toStdString();
  updateCacheModel();
}

void ConfigWindowPresenter::saveConfigurationFile()
{
  (CacheFactory::Instance())
      ->storeConfigurationToFile(cacheHierarchy.get(), cfgFilePath);
}
