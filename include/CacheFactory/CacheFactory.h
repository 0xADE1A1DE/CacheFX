/*
 * Copyright 2022 The University of Adelaide
 *
 * This file is part of CacheFX.
 *
 * Created on: Jul 23, 2019
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

#ifndef CACHEFACTORY_H_
#define CACHEFACTORY_H_

#include <Cache/Cache.h>
#include <map>
#include <memory>
#include <pugixml.hpp>
#include <string>

class CacheFactory
{
public:
  static CacheFactory* Instance();
  virtual ~CacheFactory();

  std::unique_ptr<Cache> createCache(const std::string& description,
                                     int32_t nCacheLines, int32_t nWays,
                                     replAlg replAlgorithm = REPL_RANDOM);

  const std::string& getCfgFilePath() const { return cfgFilePath; };
  void setCfgFilePath(const std::string& cfgFile) { cfgFilePath = cfgFile; };

  void storeConfigurationToFile(Cache* cache)
  {
    storeConfigurationToFile(cache, cfgFilePath);
  };
  void storeConfigurationToFile(Cache* cache, const std::string& file);
  std::unique_ptr<Cache> loadConfigurationFromFile()
  {
    return loadConfigurationFromFile(cfgFilePath);
  }
  std::unique_ptr<Cache> loadConfigurationFromFile(const std::string& file);

private:
  CacheFactory();
  CacheFactory(CacheFactory const&){};
  CacheFactory& operator=(CacheFactory const&) { return *(Instance()); };

  static std::unique_ptr<CacheFactory> _instance;

  void serializeCache(pugi::xml_node xmlCache, Cache* cache);
  std::unique_ptr<Cache> deserializeCache(pugi::xml_node xmlCache);

  std::map<replAlg, std::string> repAlgStrMap;
  std::map<std::string, replAlg> strRepAlgMap;
  std::string cfgFilePath;

private:
  static const char* XMLNODE_CACHEHIERARCHY;
  static const char* XMLNODE_CACHE;
  static const char* XMLNODE_CACHE_TYPE;
  static const char* XMLNODE_CACHE_NLINES;
  static const char* XMLNODE_CACHE_NWAYS;
  static const char* XMLNODE_CACHE_REPLALG;
  static const char* XMLNODE_CACHE_LEVEL;
  static const char* XMLNODE_CACHE_NBITS;
  static const char* XMLNODE_CACHE_KBITS;
  static const char* XMLNODE_CACHE_NSECWAYS;
  static const char* XMLNODE_CACHE_NPARTITIONS;
  static const char* XMLNODE_CACHE_NRANDOMSETS;
};

#endif /* CACHEFACTORY_H_ */
