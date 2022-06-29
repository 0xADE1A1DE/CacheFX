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
 
#include <Cache/CEASERCache.h>
#include <Cache/CEASERSCache.h>
#include <Cache/CacheHierarchy.h>
#include <Cache/NewCache.h>
#include <Cache/PLcache.h>
#include <Cache/PhantomCache.h>
#include <Cache/ScatterCache.h>
#include <Cache/SetAssocCache.h>
#include <Cache/WayPartitionCache.h>
#include <CacheFactory/CacheFactory.h>
#include <tgmath.h>

#include <iostream>
#include <vector>

#include <unistd.h>

#include <pugixml.hpp>

std::unique_ptr<CacheFactory> CacheFactory::_instance = nullptr;

const char* CacheFactory::XMLNODE_CACHEHIERARCHY = "CacheHierarchy";
const char* CacheFactory::XMLNODE_CACHE = "Cache";
const char* CacheFactory::XMLNODE_CACHE_TYPE = "Type";
const char* CacheFactory::XMLNODE_CACHE_NLINES = "NLines";
const char* CacheFactory::XMLNODE_CACHE_NWAYS = "NWays";
const char* CacheFactory::XMLNODE_CACHE_NSECWAYS = "NSecWays";
const char* CacheFactory::XMLNODE_CACHE_REPLALG = "ReplAlg";
const char* CacheFactory::XMLNODE_CACHE_LEVEL = "Level";
const char* CacheFactory::XMLNODE_CACHE_NBITS = "NBits";
const char* CacheFactory::XMLNODE_CACHE_KBITS = "KBits";
const char* CacheFactory::XMLNODE_CACHE_NPARTITIONS = "Partitions";
const char* CacheFactory::XMLNODE_CACHE_NRANDOMSETS = "NRandomSets";

CacheFactory::CacheFactory()
{
  repAlgStrMap[REPL_LRU] = "lru";
  repAlgStrMap[REPL_BIT_PLRU] = "bit-plru";
  repAlgStrMap[REPL_TREE_PLRU] = "tree-plru";
  repAlgStrMap[REPL_RANDOM] = "random";
  strRepAlgMap["lru"] = REPL_LRU;
  strRepAlgMap["random"] = REPL_RANDOM;
  strRepAlgMap["bit-plru"] = REPL_BIT_PLRU;
  strRepAlgMap["tree-plru"] = REPL_TREE_PLRU;
}

CacheFactory::~CacheFactory() {}

CacheFactory* CacheFactory::Instance()
{
  if (_instance == nullptr)
  {
    _instance.reset(new CacheFactory());
  }
  return _instance.get();
}

std::unique_ptr<Cache> CacheFactory::createCache(const std::string& description,
                                                 int32_t nCacheLines,
                                                 int32_t nWays,
                                                 replAlg replAlgorithm)
{
  if (description == SetAssocCache::CACHE_TYPESTR)
  {
    return std::unique_ptr<Cache>(
        new SetAssocCache(replAlgorithm, nCacheLines / nWays, nWays));
  }
  else if (description == AssocCache::CACHE_TYPESTR)
  {
    return std::unique_ptr<Cache>(new AssocCache(replAlgorithm, nCacheLines));
  }
  else if (description == ScatterCache::CACHE_TYPESTR)
  {
    return std::unique_ptr<Cache>(new ScatterCache(nCacheLines / nWays, nWays));
  }
  else if (description == NewCache::CACHE_TYPESTR)
  {
    return std::unique_ptr<Cache>(new NewCache((int32_t)log2(nCacheLines), 2));
  }
  else if (description == AssocPLcache::CACHE_TYPESTR)
  {
    return std::unique_ptr<Cache>(new AssocPLcache(replAlgorithm, nCacheLines));
  }
  else if (description == PLcache::CACHE_TYPESTR)
  {
    return std::unique_ptr<Cache>(
        new PLcache(replAlgorithm, nCacheLines / nWays, nWays));
  }
  else if (description == WayPartitionCache::CACHE_TYPESTR)
  {
    return std::unique_ptr<Cache>(
        new WayPartitionCache(replAlgorithm, nCacheLines / nWays, nWays, 1));
  }
  else if (description == CEASERCache::CACHE_TYPESTR)
  {
    return std::unique_ptr<Cache>(
        new CEASERCache(replAlgorithm, nCacheLines / nWays, nWays));
  }
  else if (description == CEASERSCache::CACHE_TYPESTR)
  {
    return std::unique_ptr<Cache>(new CEASERSCache(nCacheLines / nWays, nWays));
  }
  else if (description == PhantomCache::CACHE_TYPESTR)
  {
    return std::unique_ptr<Cache>(
        new PhantomCache(nCacheLines / nWays, nWays, 2));
  }

  return std::unique_ptr<Cache>(nullptr);
}

void CacheFactory::serializeCache(pugi::xml_node xmlCache, Cache* cache)
{
  std::string cacheType(cache->getCacheType());

  // xmlCache.append_child(XMLNODE_CACHE_TYPE).append_child(pugi::node_pcdata).set_value(cacheType.c_str());
  xmlCache.append_attribute(XMLNODE_CACHE_TYPE).set_value(cacheType.c_str());

  if (cacheType == SetAssocCache::CACHE_TYPESTR)
  {
    SetAssocCache* c = (SetAssocCache*)cache;
    xmlCache.append_attribute(XMLNODE_CACHE_NLINES)
        .set_value(c->getNSets() * c->getNWays());
    xmlCache.append_attribute(XMLNODE_CACHE_NWAYS).set_value(c->getNWays());
    xmlCache.append_attribute(XMLNODE_CACHE_REPLALG)
        .set_value(repAlgStrMap[c->getAlgorithm()].c_str());
  }
  else if (cacheType == AssocCache::CACHE_TYPESTR)
  {
    AssocCache* c = (AssocCache*)cache;
    xmlCache.append_attribute(XMLNODE_CACHE_NLINES).set_value(c->getSize());
    xmlCache.append_attribute(XMLNODE_CACHE_REPLALG)
        .set_value(repAlgStrMap[c->getAlgorithm()].c_str());
  }
  else if (cacheType == ScatterCache::CACHE_TYPESTR)
  {
    ScatterCache* c = (ScatterCache*)cache;
    xmlCache.append_attribute(XMLNODE_CACHE_NLINES)
        .set_value(c->getNSets() * c->getNWays());
    xmlCache.append_attribute(XMLNODE_CACHE_NWAYS).set_value(c->getNWays());
  }
  else if (cacheType == NewCache::CACHE_TYPESTR)
  {
    NewCache* c = (NewCache*)cache;
    xmlCache.append_attribute(XMLNODE_CACHE_NBITS).set_value(c->getNBits());
    xmlCache.append_attribute(XMLNODE_CACHE_KBITS).set_value(c->getKBits());
  }
  else if (cacheType == WayPartitionCache::CACHE_TYPESTR)
  {
    WayPartitionCache* c = (WayPartitionCache*)cache;
    xmlCache.append_attribute(XMLNODE_CACHE_NLINES).set_value(c->getNLines());
    xmlCache.append_attribute(XMLNODE_CACHE_NWAYS).set_value(c->getNWays());
    xmlCache.append_attribute(XMLNODE_CACHE_NSECWAYS)
        .set_value(c->getNSecWays());
    xmlCache.append_attribute(XMLNODE_CACHE_REPLALG)
        .set_value(repAlgStrMap[c->getAlgorithm()].c_str());
  }
  else if (cacheType == PLcache::CACHE_TYPESTR)
  {
    PLcache* c = (PLcache*)cache;
    xmlCache.append_attribute(XMLNODE_CACHE_NLINES).set_value(c->getNLines());
    xmlCache.append_attribute(XMLNODE_CACHE_NWAYS).set_value(c->getNWays());
    xmlCache.append_attribute(XMLNODE_CACHE_REPLALG)
        .set_value(repAlgStrMap[c->getAlgorithm()].c_str());
  }
  else if (cacheType == AssocPLcache::CACHE_TYPESTR)
  {
    AssocPLcache* c = (AssocPLcache*)cache;
    xmlCache.append_attribute(XMLNODE_CACHE_NLINES).set_value(c->getSize());
    xmlCache.append_attribute(XMLNODE_CACHE_REPLALG)
        .set_value(repAlgStrMap[c->getAlgorithm()].c_str());
  }
  else if (cacheType == CEASERCache::CACHE_TYPESTR)
  {
    CEASERCache* c = (CEASERCache*)cache;
    xmlCache.append_attribute(XMLNODE_CACHE_NLINES)
        .set_value(c->getNSets() * c->getNWays());
    xmlCache.append_attribute(XMLNODE_CACHE_NWAYS).set_value(c->getNWays());
    xmlCache.append_attribute(XMLNODE_CACHE_REPLALG)
        .set_value(repAlgStrMap[c->getAlgorithm()].c_str());
  }
  else if (cacheType == CEASERSCache::CACHE_TYPESTR)
  {
    CEASERSCache* c = (CEASERSCache*)cache;
    xmlCache.append_attribute(XMLNODE_CACHE_NLINES)
        .set_value(c->getNSets() * c->getNWays());
    xmlCache.append_attribute(XMLNODE_CACHE_NWAYS).set_value(c->getNWays());
    xmlCache.append_attribute(XMLNODE_CACHE_NPARTITIONS)
        .set_value(c->getNPartitions());
  }
  else if (cacheType == PhantomCache::CACHE_TYPESTR)
  {
    PhantomCache* c = (PhantomCache*)cache;
    xmlCache.append_attribute(XMLNODE_CACHE_NLINES)
        .set_value(c->getNSets() * c->getNWays());
    xmlCache.append_attribute(XMLNODE_CACHE_NWAYS).set_value(c->getNWays());
    xmlCache.append_attribute(XMLNODE_CACHE_NRANDOMSETS)
        .set_value(c->getNRandomSets());
    xmlCache.append_attribute(XMLNODE_CACHE_REPLALG)
        .set_value(repAlgStrMap[c->getAlgorithm()].c_str());
  }
}

std::unique_ptr<Cache> CacheFactory::deserializeCache(pugi::xml_node xmlCache)
{
  std::unique_ptr<Cache> cache = nullptr;
  std::string cacheType(xmlCache.attribute(XMLNODE_CACHE_TYPE).as_string());

  if (cacheType == SetAssocCache::CACHE_TYPESTR)
  {
    int32_t nWays = xmlCache.attribute(XMLNODE_CACHE_NWAYS).as_int();
    cache.reset(new SetAssocCache(
        strRepAlgMap[xmlCache.attribute(XMLNODE_CACHE_REPLALG).as_string()],
        xmlCache.attribute(XMLNODE_CACHE_NLINES).as_int() / nWays, nWays));
  }
  else if (cacheType == AssocCache::CACHE_TYPESTR)
  {
    cache.reset(new AssocCache(
        strRepAlgMap[xmlCache.attribute(XMLNODE_CACHE_REPLALG).as_string()],
        xmlCache.attribute(XMLNODE_CACHE_NLINES).as_int()));
  }
  else if (cacheType == ScatterCache::CACHE_TYPESTR)
  {
    int32_t nWays = xmlCache.attribute(XMLNODE_CACHE_NWAYS).as_int();
    cache.reset(new ScatterCache(
        xmlCache.attribute(XMLNODE_CACHE_NLINES).as_int() / nWays, nWays));
  }
  else if (cacheType == NewCache::CACHE_TYPESTR)
  {
    size_t nbits = 0;
    if (xmlCache.attribute(XMLNODE_CACHE_NBITS).empty())
    {
      nbits = (int32_t)log2(xmlCache.attribute(XMLNODE_CACHE_NLINES).as_int());
    }
    else
    {
      nbits = xmlCache.attribute(XMLNODE_CACHE_NBITS).as_int();
    }
    cache.reset(
        new NewCache(nbits, xmlCache.attribute(XMLNODE_CACHE_KBITS).as_int()));
  }
  else if (cacheType == WayPartitionCache::CACHE_TYPESTR)
  {
    int32_t nWays = xmlCache.attribute(XMLNODE_CACHE_NWAYS).as_int();
    int32_t nSecWays = xmlCache.attribute(XMLNODE_CACHE_NSECWAYS).as_int();
    cache.reset(new WayPartitionCache(
        strRepAlgMap[xmlCache.attribute(XMLNODE_CACHE_REPLALG).as_string()],
        xmlCache.attribute(XMLNODE_CACHE_NLINES).as_int() / nWays, nWays,
        nSecWays));
  }
  else if (cacheType == PLcache::CACHE_TYPESTR)
  {
    int32_t nWays = xmlCache.attribute(XMLNODE_CACHE_NWAYS).as_int();
    cache.reset(new PLcache(
        strRepAlgMap[xmlCache.attribute(XMLNODE_CACHE_REPLALG).as_string()],
        xmlCache.attribute(XMLNODE_CACHE_NLINES).as_int() / nWays, nWays));
  }
  else if (cacheType == AssocPLcache::CACHE_TYPESTR)
  {
    cache.reset(new AssocPLcache(
        strRepAlgMap[xmlCache.attribute(XMLNODE_CACHE_REPLALG).as_string()],
        xmlCache.attribute(XMLNODE_CACHE_NLINES).as_int()));
  }
  else if (cacheType == CEASERCache::CACHE_TYPESTR)
  {
    int32_t nWays = xmlCache.attribute(XMLNODE_CACHE_NWAYS).as_int();
    cache.reset(new CEASERCache(
        strRepAlgMap[xmlCache.attribute(XMLNODE_CACHE_REPLALG).as_string()],
        xmlCache.attribute(XMLNODE_CACHE_NLINES).as_int() / nWays, nWays));
  }
  else if (cacheType == CEASERSCache::CACHE_TYPESTR)
  {
    int32_t nWays = xmlCache.attribute(XMLNODE_CACHE_NWAYS).as_int();
    cache.reset(new CEASERSCache(
        xmlCache.attribute(XMLNODE_CACHE_NLINES).as_int() / nWays, nWays,
        xmlCache.attribute(XMLNODE_CACHE_NPARTITIONS).as_int()));
  }
  else if (cacheType == PhantomCache::CACHE_TYPESTR)
  {
    int32_t nWays = xmlCache.attribute(XMLNODE_CACHE_NWAYS).as_int();
    cache.reset(new PhantomCache(
        strRepAlgMap[xmlCache.attribute(XMLNODE_CACHE_REPLALG).as_string()],
        xmlCache.attribute(XMLNODE_CACHE_NLINES).as_int() / nWays, nWays,
        xmlCache.attribute(XMLNODE_CACHE_NRANDOMSETS).as_int()));
  }

  return cache;
}

void CacheFactory::storeConfigurationToFile(Cache* cache,
                                            const std::string& file)
{
  pugi::xml_document doc;

  pugi::xml_node xmlCacheHierarchy = doc.append_child(XMLNODE_CACHEHIERARCHY);

  std::string cacheType(cache->getCacheType());
  if (cacheType == CacheHierarchy::CACHE_TYPESTR)
  {
    CacheHierarchy* cacheHierarchy = (CacheHierarchy*)cache;
    for (size_t i; i < cacheHierarchy->getCacheLevels(); i++)
    {
      pugi::xml_node xmlCache = xmlCacheHierarchy.append_child(XMLNODE_CACHE);
      serializeCache(xmlCache, cacheHierarchy->getCache(i));
      xmlCache.append_attribute(XMLNODE_CACHE_LEVEL).set_value(i + 1);
    }
  }
  else
  {
    pugi::xml_node xmlCache = xmlCacheHierarchy.append_child(XMLNODE_CACHE);
    serializeCache(xmlCache, cache);
    xmlCache.append_attribute(XMLNODE_CACHE_LEVEL).set_value(1);
  }

  doc.save_file(file.c_str());
}

std::unique_ptr<Cache>
CacheFactory::loadConfigurationFromFile(const std::string& file)
{
  std::unique_ptr<CacheHierarchy> cacheHierarchy(new CacheHierarchy());
  if (access(file.c_str(), F_OK) == -1)
  {
    fprintf(stderr, "Invalid configuration file path! (%s)\n", file.c_str());
    exit(-2);
  }

  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_file(file.c_str());

  if (result)
  {

    std::vector<int32_t> levels;
    std::vector<std::unique_ptr<Cache>> caches_unordered;
    for (pugi::xml_node xmlCache :
         doc.child(XMLNODE_CACHEHIERARCHY).children(XMLNODE_CACHE))
    {
      int32_t level = xmlCache.attribute(XMLNODE_CACHE_LEVEL).as_int();
      levels.push_back(level);
      caches_unordered.push_back(std::move(deserializeCache(xmlCache)));
    }

    std::vector<std::unique_ptr<Cache>> caches_ordered(caches_unordered.size());
    for (size_t i = 0; i < caches_unordered.size(); i++)
    {
      if (levels[i] < 1 || levels[i] > caches_unordered.size())
      {
        std::cerr << "Invalid cache level provided!" << std::endl;
        break;
      }
      caches_ordered[levels[i] - 1] = std::move(caches_unordered[i]);
    }

    cacheHierarchy->setCacheLevels(std::move(caches_ordered));
  }

  cacheHierarchy->printHierarchy();

  return cacheHierarchy;
}
