/*
 * Copyright 2022 The University of Adelaide
 *
 * This file is part of CacheFX.
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

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

using namespace std;

#include <AttackEfficiencyController.h>
#include <Attacker/Attacker.h>
#include <Cache/Cache.h>
#include <MMU/MMU.h>
#include <MemArray.h>
#include <Victim/Victim.h>
#include <types.h>

#include <Random.h>

#include <Attacker/EvictionAttacker.h>
#include <Attacker/OccupancyAttacker.h>
#include <Attacker/SquareMultAttacker.h>
#include <Cache/SetAssocCache.h>
#include <Victim/AESVictim.h>
#include <Victim/BinaryVictim.h>
#include <Victim/SingleAccessVictim.h>
#include <Victim/SquareMultVictim.h>

#include <CacheFactory/CacheFactory.h>

AttackEfficiencyController::AttackEfficiencyController(CLArgs args) : args(args)
{
  std::srand(time(NULL));
  std::unique_ptr<Cache> cache(
      CacheFactory::Instance()->loadConfigurationFromFile());

  mmu = std::unique_ptr<DirectMMU>(new DirectMMU(std::move(cache), true));
}

#define ENCRYPTION_TIMES 20
#define EVICTION_SET_GENERATION_TIMES 10

#define ANALYSIS_EVICTION_PROBABILITY 1
#define ANALYSIS_SET_SIZE 2
#define ANALYSIS_NOISE_SIZE 3
#define ANALYSIS_NOISE_SIZE_HEATMAP 4

template <typename T>
static void runAnalysis(T startingValue, T lastValue, T increment, CLArgs args,
                        std::unique_ptr<DirectMMU>& mmu, const size_t nlines,
                        const int32_t analysisType,
                        const uint64_t noiseTestEvictionSetSize = 1)
{
  double noiseSize = 0.1;
  string outputFileName = args.get_outputFile();

  const NoiseType noiseType = args.get_noiseType();

  ofstream decileOut(args.get_outputFile() + "_decile");
  switch (analysisType)
  {
  case ANALYSIS_EVICTION_PROBABILITY:
    outputFileName += "_probability";
    break;
  case ANALYSIS_SET_SIZE:
    outputFileName += "_size";
    break;
  case ANALYSIS_NOISE_SIZE:
    outputFileName += "_noise_" + to_string(noiseTestEvictionSetSize);
  case ANALYSIS_NOISE_SIZE_HEATMAP:
    outputFileName += "_heatmap";
    break;
  };

  outputFileName += ".csv";

  ifstream outputFilePeekIfs(outputFileName);
  bool outputFileIsEmpty =
      outputFilePeekIfs.peek() == ifstream::traits_type::eof();
  outputFilePeekIfs.close();

  ofstream output;
  output.open(outputFileName, ofstream::out | ofstream::app);

  if (outputFileIsEmpty)
  {
    output << "# Eviction Probability, Eviction Set Size, Noise Size, "
              "Accesses, Encryptions,"
              "Noise Accesses, Self Eviction Rate, Unique Victim Lines, Real "
              "Evictions, Correct Eviction Rate"
           << endl;
  }

  for (T i = startingValue; i <= lastValue + numeric_limits<double>::epsilon();
       i += increment)
  {
    cout << "-i value: " << i << "\n";
    vector<uint64_t> nEncryptionsVector;

    ofstream statsOfstream(args.get_outputFile() + "_i_" + to_string(i) +
                           "noisesetsize_" +
                           to_string(noiseTestEvictionSetSize) + ".csv");

    statsOfstream
        << "Encryptions\tAavg\tASavg\tASD\tBavg\tBSavg\tBSD"
           "AREALAVG\tASREALAVG\tAREALSD\tBREALAVG\tBREALSAVG\tBREALSD\t"
           "ASELFEVICTAVG\tASELFEVICTSAVG\tASELFEVICTSD\tBSELFEVICTAVG\tBSELFEV"
           "ICTSAVG\t"
           "BSELFEVICTSD"
        << endl;

    uint64_t totalAccesses = 0;
    uint64_t totalEncryptions = 0;

    uint64_t totalConstructedEvictionSetSize = 0;
    double totalConstructedEvictionProbability = 0.0;
    uint64_t totalNoiseAccesses = 0;

    double totalSelfEvictionRate = 0.0;
    uint64_t totalUniqueVictimLines = 0;

    double totalCorrectEvictionRate = 0.0;
    double falseEvictions = 0.0;

    double totalABDiff = 0.0;

    if (analysisType == ANALYSIS_NOISE_SIZE ||
        analysisType == ANALYSIS_NOISE_SIZE_HEATMAP)
      noiseSize = i;
    for (int32_t j = 0; j < EVICTION_SET_GENERATION_TIMES; j++)
    {
      unique_ptr<EvictionAttacker> attacker;
      unique_ptr<Victim> victim;

      switch (args.get_victim())
      {
      case VT_AES:
        attacker =
            make_unique<EvictionAttacker>(mmu.get(), nlines * CACHE_LINE_SIZE,
                                          -1, args, noiseSize, noiseType);
        victim = std::unique_ptr<Victim>(new AESVictim(mmu.get(), args));
        break;
      case VT_SQUAREMULT: {
        int32_t modsize = 32;
        uint16_t mod[modsize];
        for (int32_t i = 0; i < modsize; i++)
          mod[i] = 0xffff - i;

        int32_t secbit = 7;
        attacker =
            make_unique<EvictionAttacker>(mmu.get(), nlines * CACHE_LINE_SIZE,
                                          secbit, args, noiseSize, noiseType);
        victim = std::unique_ptr<Victim>(new SquareMultVictim(
            mmu.get(), attacker.get(), 2, 32, mod, secbit, args));
        break;
      }
      case VT_SINGLE: {
        const size_t cacheSize = nlines * CACHE_LINE_SIZE;
        attacker = make_unique<EvictionAttacker>(mmu.get(), cacheSize, -1, args,
                                                 noiseSize, noiseType);
        victim = std::unique_ptr<Victim>(
            new SingleAccessVictim(mmu.get(), cacheSize, true));
        break;
      }
      case VT_BINARY: {
        attacker =
            make_unique<EvictionAttacker>(mmu.get(), nlines * CACHE_LINE_SIZE,
                                          -1, args, noiseSize, noiseType);
        victim = std::unique_ptr<Victim>(new BinaryVictim(mmu.get(), args));
        break;
      }
      }
      attacker->setOutputStatsOfs(&statsOfstream);

      int32_t keylen = victim->getKeySize();
      const auto keys = victim->genKeyPair();

      uint64_t constructedEvictionSetSize = 0;
      double constructedEvictionProbability = 0.0;

      attacker->zeroStats();
      if (analysisType == ANALYSIS_EVICTION_PROBABILITY)
      {
        if (!attacker->warmup(victim.get(), true, TAG_NONE, i,
                              &constructedEvictionSetSize,
                              &constructedEvictionProbability))
          cout << "--Failed Warmup\n";
      }
      else if (analysisType == ANALYSIS_SET_SIZE)
      {
        if (!attacker->warmup(victim.get(), true, i, 2.0,
                              &constructedEvictionSetSize,
                              &constructedEvictionProbability))
          cout << "--Failed Warmup\n";
      }
      else if (analysisType == ANALYSIS_NOISE_SIZE ||
               analysisType == ANALYSIS_NOISE_SIZE_HEATMAP)
      {
        uint64_t totalEvictionSetSize = noiseTestEvictionSetSize;
        if (args.get_noiseType() == NT_PROBE_SIZE)
          totalEvictionSetSize += noiseSize;
        if (!attacker->warmup(victim.get(), true, totalEvictionSetSize, 2.0,
                              &constructedEvictionSetSize,
                              &constructedEvictionProbability))
          cout << "--Failed Warmup\n";
      }

      totalConstructedEvictionSetSize += constructedEvictionSetSize;
      totalConstructedEvictionProbability += constructedEvictionProbability;

      uint64_t successCounter = 0;

      for (int32_t i = 0; i < ENCRYPTION_TIMES; i++)
      {
        victim->clearUniqueVictimTags();
        attacker->zeroStats();
        attacker->train(victim.get(), keys);

        totalAccesses += attacker->getMemAccesses();
        totalEncryptions += attacker->getVictimCalls();
        totalNoiseAccesses += attacker->getNoiseAccesses();
        totalSelfEvictionRate += attacker->getSelfEvictionRate();
        totalUniqueVictimLines += victim->getUniqueVictimTags();
        totalCorrectEvictionRate += attacker->getCorrectEvictionRate();
        falseEvictions +=
            attacker->getSelfEvictions() - attacker->getRealEvictions();
        if (attacker->getSuccess())
        {
          successCounter++;
          totalABDiff += attacker->getABDiff();
        }

        nEncryptionsVector.push_back(attacker->getVictimCalls());
      }
    }

    ofstream lastOfs(args.get_outputFile() + ".last");

    output << (totalConstructedEvictionProbability /
               EVICTION_SET_GENERATION_TIMES)
           << ", "
           << (totalConstructedEvictionSetSize / EVICTION_SET_GENERATION_TIMES)
           << ", " << noiseSize << ", "
           << (totalAccesses /
               (ENCRYPTION_TIMES * EVICTION_SET_GENERATION_TIMES))
           << ", "
           << (totalEncryptions /
               (ENCRYPTION_TIMES * EVICTION_SET_GENERATION_TIMES))
           << ", "
           << (totalNoiseAccesses /
               (ENCRYPTION_TIMES * EVICTION_SET_GENERATION_TIMES))
           << ", "
           << (totalSelfEvictionRate /
               (ENCRYPTION_TIMES * EVICTION_SET_GENERATION_TIMES))
           << ", "
           << (totalUniqueVictimLines /
               (ENCRYPTION_TIMES * EVICTION_SET_GENERATION_TIMES))
           << ", "
           << (totalCorrectEvictionRate /
               (ENCRYPTION_TIMES * EVICTION_SET_GENERATION_TIMES));
    output << endl;

    if (analysisType == ANALYSIS_EVICTION_PROBABILITY ||
        analysisType == ANALYSIS_SET_SIZE)
      lastOfs << "-l " << i + increment << endl;
    else if (analysisType == ANALYSIS_NOISE_SIZE)
    {
      double lastNoise = i + increment;
      if (noiseTestEvictionSetSize == nlines / 2)
      {
        lastNoise += 10.0;
      }
      else if (noiseTestEvictionSetSize == nlines)
      {
        lastNoise += 20.0;
      }
      lastOfs << "-l " << lastNoise << endl;
    }
    else if (analysisType == ANALYSIS_NOISE_SIZE_HEATMAP)
      lastOfs << "-l " << noiseTestEvictionSetSize << " -z " << i + increment;
    lastOfs.flush();
    lastOfs.close();

    statsOfstream.close();

    sort(nEncryptionsVector.begin(), nEncryptionsVector.end());

    decileOut << to_string(i) << ", ";
    for (uint32_t i = 0; i <= 10; i++)
    {
      decileOut << nEncryptionsVector[uint64_t(
                       i * (0.1 * nEncryptionsVector.size() - 1))]
                << ", ";
    }
    decileOut << endl;
  }
}

void AttackEfficiencyController::run(int32_t nattacks)
{
  double probabilityStartingPoint = 0.05;
  double noiseStartingPoint = 0.0;
  uint64_t setStartingPoint = 1;

  const AttackEfficacyType type = args.get_attackEfficacyType();

  const bool limit = !args.get_attackEfficacyLimit().empty();
  const bool limit2 = !args.get_attackEfficacyLimit2().empty();
  const bool endPointLimit = !args.get_endPoint().empty();
  const bool customIncrement = !args.get_increment().empty();
  const bool customNoiseIncrement = !args.get_noiseIncrement().empty();
  const bool customNoiseEndpoint = !args.get_noiseEndPoint().empty();

  if (limit)
  {
    if (type == AET_PROBABILITY)
      probabilityStartingPoint = stod(args.get_attackEfficacyLimit());
    else if (type == AET_SIZE)
      setStartingPoint = stoi(args.get_attackEfficacyLimit());
    else if (type == AET_NOISE_SIZE)
      noiseStartingPoint = stod(args.get_attackEfficacyLimit());
  }

  const size_t nlines = mmu->getCache()->getNLines();
  cout << args.get_outputFile() << endl;

  double increment;
  double endPoint;

  if (type == AET_HEATMAP)
  {
    increment = 1;
    endPoint = 10;
    double noiseTarget = 10;
    double noiseIncrement = 1;

    if (limit)
      setStartingPoint = stoi(args.get_attackEfficacyLimit());
    if (limit2)
      noiseStartingPoint = stod(args.get_attackEfficacyLimit2());
    if (endPointLimit)
      endPoint = stod(args.get_endPoint());
    if (customIncrement)
      increment = stod(args.get_increment());
    if (customNoiseIncrement)
      noiseIncrement = stod(args.get_noiseIncrement());
    if (customNoiseEndpoint)
      noiseTarget = stod(args.get_noiseEndPoint());

    for (uint64_t setSize = setStartingPoint;
         setSize <= endPoint + numeric_limits<double>::epsilon();
         setSize += increment)
    {
      runAnalysis(noiseStartingPoint,
                  noiseTarget + numeric_limits<double>::epsilon(),
                  noiseIncrement, args, mmu, nlines,
                  ANALYSIS_NOISE_SIZE_HEATMAP, setSize);
      noiseStartingPoint = 0;
    }
  }
  else if (type == AET_PROBABILITY)
  {
    increment = 0.05;
    endPoint = 1.00;

    if (customIncrement)
      increment = stod(args.get_increment());
    if (endPointLimit)
      endPoint = stod(args.get_endPoint());

    runAnalysis(probabilityStartingPoint, endPoint, increment, args, mmu,
                nlines, ANALYSIS_EVICTION_PROBABILITY);
  }
  else if (type == AET_SIZE)
  {
    increment = 25;
    endPoint = nlines * 100;

    if (customIncrement)
      increment = stod(args.get_increment());
    if (endPointLimit)
      endPoint = stod(args.get_endPoint());

    runAnalysis(setStartingPoint, uint64_t(endPoint), uint64_t(increment), args,
                mmu, nlines, ANALYSIS_SET_SIZE);
  }
  else if (type == AET_NOISE_SIZE)
  {
    increment = 25;
    endPoint = 5.0;

    if (customIncrement)
      increment = stod(args.get_increment());
    if (endPointLimit)
      endPoint = stod(args.get_endPoint());

    runAnalysis(noiseStartingPoint, endPoint, increment, args, mmu, nlines,
                ANALYSIS_NOISE_SIZE, nlines / 2);
  }
}
