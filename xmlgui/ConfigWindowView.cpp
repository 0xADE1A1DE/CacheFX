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

#include "ConfigWindowView.h"
#include "ConfigWindowPresenter.h"
#include <QCloseEvent>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QListView>
#include <QMessageBox>
#include <QModelIndexList>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QTreeView>

ConfigWindowView::ConfigWindowView(QWidget* parent) : QWidget(parent)
{

  setFixedSize(720, 400);
  setWindowTitle("XML Cache Config Editor");

  qBoxCaches = new QGroupBox("Caches", this);
  qBoxCaches->setGeometry(0, 0, 240, 400);

  qButtonUp = new QPushButton("Up", qBoxCaches);
  qButtonUp->setGeometry(0, 350, 60, 50);
  qButtonDown = new QPushButton("Down", qBoxCaches);
  qButtonDown->setGeometry(60, 350, 60, 50);
  qButtonRemove = new QPushButton("Remove", qBoxCaches);
  qButtonRemove->setGeometry(120, 350, 60, 50);
  qButtonNew = new QPushButton("New", qBoxCaches);
  qButtonNew->setGeometry(180, 350, 60, 50);

  qListCaches = new QListView(qBoxCaches);
  qListCaches->setGeometry(0, 20, 240, 330);
  qListCaches->setEditTriggers(QAbstractItemView::NoEditTriggers);
  qListCaches->setSelectionMode(
      QAbstractItemView::SelectionMode::SingleSelection);

  qBoxCacheConfig = new QGroupBox("Cache Config", this);
  qBoxCacheConfig->setGeometry(240, 0, 480, 400);

  qLabelCacheType = new QLabel("Cache Type:", qBoxCacheConfig);
  qLabelCacheType->setGeometry(0, 20, 240, 30);
  qComboCacheType = new QComboBox(qBoxCacheConfig);
  qComboCacheType->setGeometry(240, 20, 240, 30);
  qLabelNLines = new QLabel("Number of Cache Lines:", qBoxCacheConfig);
  qLabelNLines->setGeometry(0, 50, 240, 30);
  qLabelNLinesData = new QLabel("", qBoxCacheConfig);
  qLabelNLinesData->setGeometry(240, 50, 240, 30);
  qLabelProperties = new QLabel("Properties:", qBoxCacheConfig);
  qLabelProperties->setGeometry(0, 80, 240, 30);
  qTreeProperties = new QTreeView(qBoxCacheConfig);
  qTreeProperties->setGeometry(0, 120, 480, 320);
  qTreeProperties->setHeaderHidden(true);
}

ConfigWindowView::~ConfigWindowView() {}

int ConfigWindowView::getSelectedCacheIdx()
{
  auto selectedRows = qListCaches->selectionModel()->selectedRows();
  if (selectedRows.size() > 0)
  {
    return selectedRows[0].row();
  }
  else
  {
    return 0;
  }
}

void ConfigWindowView::setListCaches(QStringListModel* cacheList)
{
  qListCaches->setModel(cacheList);
}

void ConfigWindowView::setListCacheTypes(QStringListModel* cacheTypeList)
{
  qComboCacheType->setModel(cacheTypeList);
}

void ConfigWindowView::setCacheTypeSelected(int idx)
{
  qComboCacheType->setCurrentIndex(idx);
}

void ConfigWindowView::setNLines(int value)
{
  qLabelNLinesData->setText(QString::number(value));
}

int ConfigWindowView::getNLines() { return qLabelNLinesData->text().toInt(); }

void ConfigWindowView::setListCacheSelected(int row)
{
  auto index = qListCaches->model()->index(row, 0);
  qListCaches->selectionModel()->select(
      index, QItemSelectionModel::SelectionFlag::Select);
}

void ConfigWindowView::registerPresenter(ConfigWindowPresenter* presenter)
{
  connect(qButtonUp, SIGNAL(clicked()), presenter, SLOT(upButtonClicked()));
  connect(qButtonDown, SIGNAL(clicked()), presenter, SLOT(downButtonClicked()));
  connect(qButtonRemove, SIGNAL(clicked()), presenter,
          SLOT(removeButtonClicked()));
  connect(qButtonNew, SIGNAL(clicked()), presenter, SLOT(newButtonClicked()));
  connect(qListCaches->selectionModel(),
          SIGNAL(currentRowChanged(QModelIndex, QModelIndex)), presenter,
          SLOT(cacheListSelectionChanged(QModelIndex, QModelIndex)));
  connect(qComboCacheType, SIGNAL(currentIndexChanged(int)), presenter,
          SLOT(cacheTypeSelectionChanged(int)));
  connect(this, SIGNAL(saveConfigFile()), presenter,
          SLOT(saveConfigurationFile()));
}

void ConfigWindowView::closeEvent(QCloseEvent* event)
{
  QMessageBox msgBox;
  msgBox.setText("Do you want to save the changes made to your config file?");
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No |
                            QMessageBox::Cancel);
  msgBox.setDefaultButton(QMessageBox::Yes);
  int ret = msgBox.exec();
  if (ret == QMessageBox::Yes)
  {
    emit saveConfigFile();
  }
  if (ret != QMessageBox::Cancel)
  {
    event->accept();
  }
  else
  {
    event->ignore();
  }
}

void ConfigWindowView::setCachePropertyTable(QStandardItemModel* propertyTable)
{
  qTreeProperties->setModel(propertyTable);
  qTreeProperties->resizeColumnToContents(0);
  qTreeProperties->resizeColumnToContents(1);
}

void ConfigWindowView::installReplAlgComboBox(int row,
                                              QStringListModel* replAlgList,
                                              ConfigWindowPresenter* presenter)
{
  if (row != -1)
  {
    auto index = qTreeProperties->model()->index(row, 1);
    qComboReplAlg = new QComboBox(qTreeProperties);
    qComboReplAlg->setModel(replAlgList);
    qComboReplAlg->setVisible(true);
    qTreeProperties->setIndexWidget(index, qComboReplAlg);
    connect(qComboReplAlg, SIGNAL(currentTextChanged(QString)), presenter,
            SLOT(algorithmSelectionChanged(QString)));
  }
}

QString ConfigWindowView::getSelectedReplAlgorithm()
{
  return qComboReplAlg->currentText();
}

void ConfigWindowView::setSelectedReplAlgorithm(QString algo)
{
  qComboReplAlg->setCurrentText(algo);
}
