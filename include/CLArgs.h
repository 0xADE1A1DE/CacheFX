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
 
#ifndef __CLARGS_H__
#define __CLARGS_H__ 1

enum MeasurementType
{
  MT_INVALID = 0x0,
  MT_ENTROPY = 0x01,
  MT_PROFILING = 0x02,
  MT_ATTACKER = 0x04,
  MT_EFFICIENCY = 0x05
};
enum VictimType
{
  VT_INVALID,
  VT_AES,
  VT_SQUAREMULT,
  VT_SINGLE,
  VT_BINARY
};
enum AttackEfficacyType
{
  AET_ALL,
  AET_PROBABILITY,
  AET_SIZE,
  AET_NOISE_SIZE,
  AET_HEATMAP
};

enum AttackerType
{
  AT_INVALID,
  AT_OCCUPANCY,
  AT_EVICTION
};
enum ProbeType
{
  PT_ATTACKER,
  PT_LAST,
  PT_VICTIM
};
enum AccessType
{
  ACT_ALL,
  ACT_TARGET,
  ACT_FIVE,
  ACT_TEN,
  ACT_FIFTEEN
};
enum NoiseType
{
  NT_SEPARATE,
  NT_SAME,
  NT_PROBE_SIZE
};

class CLArgs
{
private:
  std::string cfgFilePath;
  std::string outputFile;
  bool fileOutput;
  MeasurementType measurements;
  VictimType victim;
  AttackerType attacker;
  int32_t repeats;
  int32_t giveup;
  AttackEfficacyType attackEfficacyType;
  std::string attackEfficacyLimit;
  std::string attackEfficacyLimit2;
  ProbeType probeType;
  AccessType accessType;
  NoiseType noiseType;
  std::string increment;
  std::string endPoint;
  std::string noiseIncrement;
  std::string noiseEndPoint;
  bool alwaysNoise;

public:
  CLArgs()
  {
    cfgFilePath = "config.csv";
    outputFile = "output.csv";
    measurements = MT_INVALID;
    victim = VT_INVALID;
    attacker = AT_OCCUPANCY;
    repeats = 1;
    giveup = 10000;
    attackEfficacyLimit = "";
    attackEfficacyLimit2 = "";
    probeType = PT_ATTACKER;
    accessType = ACT_ALL;
    noiseType = NT_SEPARATE;
    increment = "";
    endPoint = "";
    noiseEndPoint = "";
    alwaysNoise = false;
  }

  std::string& get_cfgFilePath() { return cfgFilePath; };
  std::string& get_outputFile() { return outputFile; };
  bool get_fileOutput() { return fileOutput; };
  MeasurementType get_measurements() { return measurements; };
  VictimType get_victim() { return victim; };
  AttackerType get_attacker() { return attacker; };
  int32_t get_repeats() { return repeats; };
  int32_t get_giveup() { return giveup; };
  AttackEfficacyType get_attackEfficacyType() { return attackEfficacyType; };
  std::string get_attackEfficacyLimit() { return attackEfficacyLimit; };
  std::string get_attackEfficacyLimit2() { return attackEfficacyLimit2; };
  ProbeType get_probeType() { return probeType; };
  AccessType get_accessType() { return accessType; };
  NoiseType get_noiseType() { return noiseType; };
  std::string get_increment() { return increment; };
  std::string get_noiseIncrement() { return noiseIncrement; };
  std::string get_endPoint() { return endPoint; };
  std::string get_noiseEndPoint() { return noiseEndPoint; };
  bool get_alwaysNoise() { return alwaysNoise; };

  void parse(int32_t argc, char** argv);
};

#endif // __CLARGS_H__
