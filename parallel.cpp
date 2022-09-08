#include <atomic>
#include <chrono>
#include <cstddef>
#include <numeric>
#include <thread>
#include <vector>

#include "felru.hpp"
#include "io.hpp"

template <typename Cache>
void hit_rate(Cache& cache, const size_t num, std::vector<size_t>& io) {
  std::atomic<size_t> hit(0);

  using iter = std::vector<size_t>::iterator;
  auto fn = [&](const iter begin, const iter end) {
    size_t local_hit = 0;
    for (auto it = begin; it != end; ++it)
      local_hit += cache.set(*it, nullptr);
    hit += local_hit;
  };

  std::vector<std::thread> threads;
  auto it = io.begin();
  size_t stride = io.size() / num;
  for (size_t i = 1; i < num; ++i) {
    threads.emplace_back(fn, it, it + stride);
    it += stride;
  }
  threads.emplace_back(fn, it, io.end());

  auto start = std::chrono::high_resolution_clock::now();
  for (auto& th : threads) th.join();
  auto stop = std::chrono::high_resolution_clock::now();
  auto diff = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

  auto hit_rate = (double)hit / (double)io.size();
  auto throughput = (double)io.size() / (double)diff.count();
  std::cout << "        throughput: " << throughput << std::endl;
  std::cout << "        hit_rate: " << hit_rate << std::endl;
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

  std::vector sizes{1 << 13, 1 << 14, 1 << 15, 1 << 16, 1 << 17};

  std::cout << "fe_lru:" << std::endl;
  using pd = bin_dictionary::par_pd<>;

  for (auto size : sizes) {
    std::cout << "  -\n"
              << "    size: " << size << '\n'
              << "    threads: " << std::endl;
    for (auto num = 1; num <= 8; ++num) {
      std::cout << "      -\n"
                << "        num: " << num << std::endl;
      par_bin_cache<pd, mul_shift> cache(size);
      hit_rate(cache, num, io);
    }
  }
}
