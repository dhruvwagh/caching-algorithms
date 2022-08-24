#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "cache.hpp"

// See https://researcher.watson.ibm.com/researcher/view_person_subpage.php?id=4700 for
// ARC traces

std::vector<size_t> load_mem(const char* fname) {
  std::ifstream f(fname);
  std::vector<size_t> io;
  io.reserve(1UL << 23);
  std::string dummy;
  for (size_t start, length = 0UL;
       (f >> start) &&
       (f >> length) &&
       (f >> dummy) &&
       (f >> dummy);) {
    for (auto key = start; key < start + length; ++key)
      io.push_back(key);
  }
  return io;
}

template <class Cache>
auto hit_rate(std::vector<size_t> trace, Cache cache) {
  cache.describe();
  size_t total = trace.size();
  size_t hit = 0;
  for (auto key : trace)
    hit += cache.set(key, nullptr);

  auto ratio = (double)hit / (double)total;
  std::cout << hit << " / " << total << " = " << ratio << " = " << ratio * 100 << " %" << std::endl;
}

int main(int argc, char const* argv[]) {
  if (argc < 2) return 1;

  auto fname = argv[1];
  auto io = load_mem(fname);

  const size_t size = 1 << 15;

  belady cache_opt(io, size);
  hit_rate(io, cache_opt);

  lru cache_lru(size);
  hit_rate(io, cache_lru);

  mru cache_mru(size);
  hit_rate(io, cache_mru);

  std::cout << std::flush;
  return 0;
}
