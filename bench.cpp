#include <iostream>
#include <string>
#include <vector>

#include "cache.hpp"
#include "felru.hpp"
#include "io.hpp"

template <class Cache>
auto hit_rate(std::vector<size_t> trace, Cache cache) {
  size_t total = trace.size();
  size_t hit = 0;
  for (auto key : trace)
    hit += cache.set(key, nullptr);

  auto ratio = (double)hit / (double)total;

  std::cout << "  -\n"
            << "    size: " << cache.size << '\n'
            << "    hit_rate: " << ratio << std::endl;
}

int main(int argc, char const* argv[]) {
  if (argc < 2) return 1;

  auto fname = std::string(argv[1]);

  std::vector<size_t> io;
  if (fname.ends_with(".lis"))
    io = load_arc(fname);
  else if (fname.ends_with(".yaml"))
    io = load_zipf(fname);
  else
    io = load_wiki(fname);

  std::vector<size_t> sizes;
  sizes.reserve(10);
  for (size_t i = 1; i <= (1 << 10); i *= 2)
    sizes.push_back(i << 10);

  std::cout << "belady:" << std::endl;
  for (auto size : sizes) {
    belady cache_opt(io, size);
    hit_rate(io, cache_opt);
  }

  std::cout << "lru:" << std::endl;
  for (auto size : sizes) {
    lru cache_lru(size);
    hit_rate(io, cache_lru);
  }

  std::cout << "mru:" << std::endl;
  for (auto size : sizes) {
    mru cache_mru(size);
    hit_rate(io, cache_mru);
  }

  std::cout << "lru_2:" << std::endl;
  for (auto size : sizes) {
    lru_k<2> cache_lru_2(size);
    hit_rate(io, cache_lru_2);
  }

  std::cout << "lfu:" << std::endl;
  for (auto size : sizes) {
    lfu cache_lfu(size);
    hit_rate(io, cache_lfu);
  }

  std::cout << "clock:" << std::endl;
  for (auto size : sizes) {
    clock_lru cache_clock(size);
    hit_rate(io, cache_clock);
  }

  std::cout << "bin_lru:" << std::endl;
  using bin_pd = bin_dictionary::pd<bin_dictionary::lru<>>;
  for (auto size : sizes) {
    bin_cache<bin_pd, mul_shift> cache_bin_lru(size);
    hit_rate(io, cache_bin_lru);
  }

  std::cout << "fe_lru:" << std::endl;
  using pd = fano_elias::pd<>;
  for (auto size : sizes) {
    bin_cache<pd, mul_shift> cache_fe_lru(size);
    hit_rate(io, cache_fe_lru);
  }

  return 0;
}
