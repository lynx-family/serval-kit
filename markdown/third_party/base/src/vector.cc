// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/vector.h"

#if 0

#include <cstdint>
#include <map>
#include <mutex>
#include <vector>

#include "base/include/log/logging.h"

namespace {
struct MapStatisticsEntry {
  uint32_t max_count;
  uint32_t insert_count;
  uint32_t erase_count;
  uint32_t total_find_count;
  uint32_t find_count;
  uint32_t insert_find_collision_count;
};

static std::mutex MapSt_Lock;
static std::map<size_t, size_t> MapSt_TotalFindOfCount;
static std::map<size_t, size_t> MapSt_FindOfCount;
static std::vector<MapStatisticsEntry> MapSt_Entries;
}  // namespace

extern "C" {
void ClearBaseMapStatistics() {
  MapSt_FindOfCount.clear();
  MapSt_Entries.clear();
}

void PrintBaseMapStatistics() {
  std::map<uint32_t, uint32_t> max_count_info;
  std::map<uint32_t, uint32_t> insert_count_info;
  std::map<uint32_t, uint32_t> erase_count_info;
  std::map<uint32_t, uint32_t> total_find_count_info;
  std::map<uint32_t, uint32_t> find_count_info;
  std::map<uint32_t, uint32_t> insert_find_collision_count_info;

  size_t has_erase_count = 0;
  size_t map_count = 0;
  for (const auto& i : MapSt_Entries) {
    map_count++;
    if (i.erase_count > 0) {
      has_erase_count++;
    }
    max_count_info[i.max_count]++;
    insert_count_info[i.insert_count]++;
    erase_count_info[i.erase_count]++;
    total_find_count_info[i.total_find_count]++;
    find_count_info[i.find_count]++;
    insert_find_collision_count_info[i.insert_find_collision_count]++;
  }
  LOGI("[AA] map total count " << map_count);
  LOGI("[AA] erase occurred map count "
       << has_erase_count << ", accounting for "
       << (double)has_erase_count / map_count * 100.f << "%");

  double sum_percentage = 0.f;
  for (const auto& it : max_count_info) {
    double p = (double)it.second / map_count * 100.f;
    sum_percentage += p;
    LOGI("[AA] max count of " << it.first << " maps are " << it.second
                              << ", accounting for " << p << "%"
                              << ", sum accounting for " << sum_percentage
                              << "%");
  }

  sum_percentage = 0.f;
  for (const auto& it : insert_count_info) {
    double p = (double)it.second / map_count * 100.f;
    sum_percentage += p;
    LOGI("[AA] insert count of " << it.first << " maps are " << it.second
                                 << ", accounting for " << p << "%"
                                 << ", sum accounting for " << sum_percentage
                                 << "%");
  }

  sum_percentage = 0.f;
  for (const auto& it : erase_count_info) {
    double p = (double)it.second / map_count * 100.f;
    sum_percentage += p;
    LOGI("[AA] erase count of " << it.first << " maps are " << it.second
                                << ", accounting for " << p << "%"
                                << ", sum accounting for " << sum_percentage
                                << "%");
  }

  sum_percentage = 0.f;
  for (const auto& it : total_find_count_info) {
    double p = (double)it.second / map_count * 100.f;
    sum_percentage += p;
    LOGI("[AA] total find count of " << it.first << " maps are " << it.second
                                     << ", accounting for " << p << "%"
                                     << ", sum accounting for "
                                     << sum_percentage << "%");
  }

  sum_percentage = 0.f;
  for (const auto& it : find_count_info) {
    double p = (double)it.second / map_count * 100.f;
    sum_percentage += p;
    LOGI("[AA] find count of "
         << it.first << " by `find()`, `[]` or `contains()`"
         << " maps are " << it.second << ", accounting for " << p << "%"
         << ", sum accounting for " << sum_percentage << "%");
  }

  sum_percentage = 0.f;
  for (const auto& it : insert_find_collision_count_info) {
    double p = (double)it.second / map_count * 100.f;
    sum_percentage += p;
    LOGI("[AA] find when inserting and collision count of "
         << it.first << " maps are " << it.second << ", accounting for " << p
         << "%"
         << ", sum accounting for " << sum_percentage << "%");
  }

  size_t total_find_count = 0;
  for (const auto& it : MapSt_TotalFindOfCount) {
    total_find_count += it.second;
  }

  sum_percentage = 0.f;
  for (const auto& it : MapSt_TotalFindOfCount) {
    double p = (double)it.second / total_find_count * 100.f;
    sum_percentage += p;
    LOGI("[AA] find(include insertion) from map of "
         << it.first << " elements happen " << it.second
         << " times, accounting for " << p << "%"
         << ", sum accounting for " << sum_percentage << "%");
  }

  size_t find_count = 0;
  for (const auto& it : MapSt_FindOfCount) {
    find_count += it.second;
  }

  sum_percentage = 0.f;
  for (const auto& it : MapSt_FindOfCount) {
    double p = (double)it.second / find_count * 100.f;
    sum_percentage += p;
    LOGI("[AA] find by `find()`, `[]` or `contains()` from map of "
         << it.first << " elements happen " << it.second
         << " times, accounting for " << p << "%"
         << ", sum accounting for " << sum_percentage << "%");
  }
}
}

namespace lynx {
namespace base {

MapStatisticsBase<true>::~MapStatisticsBase() {
  std::lock_guard<std::mutex> lock(MapSt_Lock);
  MapSt_Entries.push_back(
      {.max_count = max_count,
       .insert_count = insert_count,
       .erase_count = erase_count,
       .total_find_count = total_find_count,
       .find_count = find_count,
       .insert_find_collision_count = insert_find_collision_count});
}

void MapStatisticsBase<true>::UpdateMaxCount(size_t v) const {
  if (v > max_count) {
    max_count = static_cast<uint32_t>(v);
  }
}

void MapStatisticsBase<true>::RecordFind(MapStatisticsFindKind kind,
                                         size_t find_of_count) const {
  total_find_count++;
  switch (kind) {
    case MapStatisticsFindKind::kFind:
      find_count++;
      break;
    case MapStatisticsFindKind::kInsertFindCollision:
      insert_find_collision_count++;
      IncreaseInsertCount();
      break;
    case MapStatisticsFindKind::kInsertFind:
      UpdateMaxCount(find_of_count + 1);
      IncreaseInsertCount();
      break;
    default:
      break;
  }
  IncreaseFindOfCount(kind, find_of_count);
}

void MapStatisticsBase<true>::IncreaseFindOfCount(MapStatisticsFindKind kind,
                                                  size_t find_of_count) {
  std::lock_guard<std::mutex> lock(MapSt_Lock);
  MapSt_TotalFindOfCount[find_of_count]++;
  if (kind == MapStatisticsFindKind::kFind) {
    MapSt_FindOfCount[find_of_count]++;
  }
}

}  // namespace base
}  // namespace lynx

#endif
