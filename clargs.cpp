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

#include <getopt.h>
#include <iostream>
#include <map>
#include <string>
#include <unistd.h>

using namespace std;

#include <CLArgs.h>
#include <Victim/Victim.h>

void CLArgs::parse(int32_t argc, char** argv)
{
  // Parse command line options

  static option longOptions[] = {{"config", required_argument, 0, 'c'},
                                 {"output", required_argument, 0, 'o'},
                                 {"measure", required_argument, 0, 'm'},
                                 {"help", no_argument, 0, 'h'},
                                 {"victim", required_argument, 0, 'v'},
                                 {"attacker", required_argument, 0, 'a'},
                                 {"repeats", required_argument, 0, 'r'},
                                 {"giveup", required_argument, 0, 'g'},
                                 {NULL, 0, NULL, 0}};

  int32_t ch = 0;
  int32_t optionIndex = 0;
  while ((ch = getopt_long(argc, argv, "a:v:c:o:m:r:h:g:t:l:p:x:z:b:e:i:n:y:f:",
                           longOptions, &optionIndex)) != -1)
  {
    switch (ch)
    {
    case 'a': {
      std::string attackerType(optarg);
      if (attackerType == "occupancy")
        attacker = AT_OCCUPANCY;
      else if (attackerType == "eviction")
        attacker = AT_EVICTION;
      else
      {
        fprintf(stderr, "Unkown attacker type '%s'\n", optarg);
        exit(1);
      }
      break;
    }
    case 'v': {
      std::string victimType(optarg);
      if (victimType == "AES")
        victim = VT_AES;
      else if (victimType == "SquareMult")
        victim = VT_SQUAREMULT;
      else if (victimType == "single")
        victim = VT_SINGLE;
      else if (victimType == "binary")
        victim = VT_BINARY;
      else
      {
        fprintf(stderr, "Unkown victim type '%s'\n", optarg);
        exit(1);
      }
      break;
    }
    case 'g': {
      giveup = atoi(optarg);
      break;
    }
    case 'r': {
      repeats = atoi(optarg);
      break;
    }
    case 'c': {
      cfgFilePath = optarg;
      break;
    }
    case 'o': {
      outputFile = optarg;
      fileOutput = true;
      break;
    }
    case 'm': {
      std::string measurementType(optarg);
      if (measurementType == "entropy")
        measurements = MT_ENTROPY;
      else if (measurementType == "profiling")
        measurements = MT_PROFILING;
      else if (measurementType == "attacker")
        measurements = MT_ATTACKER;
      else if (measurementType == "efficiency")
        measurements = MT_EFFICIENCY;
      break;
    }
    case 't': {
      const std::string attackEfficacyTypeStr(optarg);
      attackEfficacyType = AET_ALL;
      if (attackEfficacyTypeStr == "probability")
        attackEfficacyType = AET_PROBABILITY;
      else if (attackEfficacyTypeStr == "size")
        attackEfficacyType = AET_SIZE;
      else if (attackEfficacyTypeStr == "noise")
        attackEfficacyType = AET_NOISE_SIZE;
      else if (attackEfficacyTypeStr == "heatmap")
        attackEfficacyType = AET_HEATMAP;
      break;
    }
    case 'l': {
      attackEfficacyLimit = std::string(optarg);
      break;
    }
    case 'z': {
      attackEfficacyLimit2 = std::string(optarg);
      break;
    }
    case 'p': {
      const std::string probeTypeStr(optarg);
      if (probeTypeStr == "allow")
        probeType = PT_ATTACKER;
      else if (probeTypeStr == "last")
        probeType = PT_LAST;
      else if (probeTypeStr == "disallow")
        probeType = PT_VICTIM;
      break;
    }
    case 'x': {
      const std::string accessTypeStr(optarg);
      if (accessTypeStr == "all")
        accessType = ACT_ALL;
      else if (accessTypeStr == "target")
        accessType = ACT_TARGET;
      else if (accessTypeStr == "five")
        accessType = ACT_FIVE;
      else if (accessTypeStr == "ten")
        accessType = ACT_TEN;
      else if (accessTypeStr == "fifteen")
        accessType = ACT_FIFTEEN;
      break;
    }
    case 'b': {
      const std::string noiseTypeStr(optarg);
      if (noiseTypeStr == "separate")
        noiseType = NT_SEPARATE;
      else if (noiseTypeStr == "same")
        noiseType = NT_SAME;
      else if (noiseTypeStr == "probe")
        noiseType = NT_PROBE_SIZE;
      break;
    }
    case 'f': {
      const std::string noiseTypeStr(optarg);
      if (noiseTypeStr == "always")
        alwaysNoise = true;
      break;
    }
    case 'i':
      increment = optarg;
      break;
    case 'e':
      endPoint = optarg;
      break;
    case 'n':
      noiseIncrement = optarg;
      break;
    case 'y':
      noiseEndPoint = optarg;
      break;
    case 'h': {
      std::cout << "Usage: ./cachefx [OPTIONS]" << std::endl << std::endl;
      std::cout << "-c file, --config file: use config file specified by "
                   "parameter file"
                << std::endl;
      std::cout << "-o file, --output file: create output report in specified "
                   "output file"
                << std::endl;
      std::cout
          << "-m type, --measure type: performs measurements of the "
             "specified type (entropy, profiling, efficiency, or attacker)"
          << std::endl;
      std::cout << "-v victim, --victim victim: Victim type to use. Possible "
                   "values are 'AES' and 'SquareMult'"
                << std::endl;
      std::cout << "-a attacker, --attacker attacker: Attacker type "
                   "(occupancy, eviction) "
                << std::endl;
      std::cout << "-t attack efficiency mode: "
                   "(probability, size, noise, heatmap) - probability "
                   "evaluates eviction (probability, size, noise)"
                   "vs attack complexity. heatmap generates heatmap for attack "
                   "complexity"
                   "relative to eviction set size, artificial attack noise"
                << std::endl;
      std::cout << "-l start point (set size/probability/noise) " << std::endl;
      std::cout << "-z start point for noise in heatmap " << std::endl;
      std::cout << "-e end point for (set size/probability/noise) "
                << std::endl;
      std::cout << "-y end point for noise in heatmap " << std::endl;
      std::cout << "-i increment value for (set size/probability/noise) "
                << std::endl;
      std::cout << "-i increment value for noise in heatmap " << std::endl;
      std::cout << "-p self eviction in probe: (allow, disallow) "
                   "sets the probing mechanism used. allow: allows self "
                   "eviction to occur,"
                   "disallow prevents self eviction to happen."
                << std::endl;
      std::cout
          << "-r repeats, --repeats repeats: Number of times to run the attack"
          << std::endl;
      std::cout << "-g tries, --givup tries: Number of encryptions to try "
                   "before giving up"
                << std::endl;
      std::cout << "-h, --help: print this help" << std::endl;
      exit(1);
      break;
    }
    }
  }
}
