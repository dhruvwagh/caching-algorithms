#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "cache.hpp"
#include "felru.hpp"

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
  std::cout << "Hit rate: " << hit << " / " << total << " = "
            << ratio << " = " << ratio * 100 << " %" << std::endl;
}

static const char* sep = "-------------------------------------------------------";

int main(int argc, char const* argv[]) {
  if (argc < 2) return 1;

  auto fname = argv[1];
  auto io = load_mem(fname);

  const size_t size = 1 << 15;

  std::cout << sep << std::endl;

  belady cache_opt(io, size);
  hit_rate(io, cache_opt);

  std::cout << sep << std::endl;

  lru cache_lru(size);
  hit_rate(io, cache_lru);

  std::cout << sep << std::endl;

  mru cache_mru(size);
  hit_rate(io, cache_mru);

  std::cout << sep << std::endl;

  lru_k<2> cache_lru_2(size);
  hit_rate(io, cache_lru_2);

  std::cout << sep << std::endl;

  lru_k<5> cache_lru_5(size);
  hit_rate(io, cache_lru_5);

  std::cout << sep << std::endl;

  clock_lru cache_clock(size);
  hit_rate(io, cache_clock);

  std::cout << sep << std::endl;

  felru<bin_dictionary> cache_felru(size);
  hit_rate(io, cache_felru);

  std::cout << sep << std::endl;

  return 0;
}
