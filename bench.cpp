#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "cache.hpp"

// See https://researcher.watson.ibm.com/researcher/view_person_subpage.php?id=4700 for
// ARC traces

using trace = std::pair<size_t, size_t>;

std::vector<trace> loadmem(const char* fname) {
  std::ifstream f(fname);
  std::vector<trace> io;
  io.reserve(1UL << 19);
  std::string dummy;
  trace tmp = {0UL, 0UL};
  while (((f >> tmp.first) &&
          (f >> tmp.second) &&
          (f >> dummy) &&
          (f >> dummy)))
    io.push_back(tmp);
  return io;
}

template <class Cache>
auto hit_rate(std::vector<trace> io, Cache cache) {
  cache.describe();
  size_t total = 0;
  size_t hit = 0;
  for (auto [start, length] : io) {
    for (auto key = start; key < start + length; ++key)
      hit += cache.set(key, nullptr);
    total += length;
  }
  auto ratio = (double)hit / (double)total;
  std::cout << hit << " / " << total << " = " << ratio << " = " << ratio * 100 << " %" << std::endl;
}

int main(int argc, char const* argv[]) {
  if (argc < 2) return 1;

  auto fname = argv[1];
  auto io = loadmem(fname);

  const size_t size = 1 << 15;

  lru cache_lru(size);
  hit_rate(io, cache_lru);

  mru cache_mru(size);
  hit_rate(io, cache_mru);

  std::cout << std::flush;
  return 0;
}
