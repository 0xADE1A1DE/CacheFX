/*
 * Copyright 2022 The University of Adelaide
 *
 * This file is part of CacheFX.
 *
 * Created on: Feb 19, 2020
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

#ifndef ENTROPYLOSS_H_
#define ENTROPYLOSS_H_

#include <Cache/Cache.h>
#include <MMU/DirectMMU.h>
#include <MMU/MMU.h>
#include <evaluation.h>
#include <types.h>
#include <vector>

class EntropyLoss
{
public:
  EntropyLoss();
  virtual ~EntropyLoss();

  void evaluate();

  void printStatistics();
  void printStatistics(std::string& outputFilePath);

private:
  void accessAndTrackOrder(MemHandle* memHandle, address_t offset);
  void accessAndTrackContent(MemHandle* memHandle, address_t offset);
  void promoteCacheLineFront(tag_t tag);
  void replaceAndPromoteCacheLineFront(tag_t oldTag, tag_t newTag);
  void insertCacheLineFront(tag_t tag);
  double randomPattern(DirectMMU* mmu, MemHandle* memHandle, int32_t rep,
                       int32_t len);
  double linearPattern(DirectMMU* mmu, MemHandle* memHandle, int32_t rep,
                       int32_t stride, int32_t len);
  double increasingStridePattern(DirectMMU* mmu, MemHandle* memHandle,
                                 int32_t rep, int32_t stride, int32_t len);
  void printDistribution();
  double calcInformationFlow(size_t cacheSize);
  double calcInformationLoss();
  void flushCache(DirectMMU* mmu);
  void prefillCache(DirectMMU* mmu);
  double measureInformationFlow(uint32_t numExperiments, uint32_t ASMult);

private:
  std::list<tag_t> _faCache;
  std::vector<uint32_t> _evictionCnt;
  std::vector<uint32_t> _usageCnt;

  std::vector<std::tuple<const char*, uint32_t, uint32_t, uint32_t>>
      _evaluationConfigs;
  std::vector<double> _evaluationResults;
  EvaluationCacheCfg _evaluationCacheCfg;

  std::map<replAlg, std::string> repAlgStrMap;

  static const char* INFORMATION_FLOW;
  static const char* PATTERN_LINEAR;
  static const char* PATTERN_INCREASING_STRIDE;
  static const char* PATTERN_RANDOM;
};

#endif /* ENTROPYLOSS_H_ */
