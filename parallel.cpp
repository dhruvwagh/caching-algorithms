#include <atomic>
#include <cstddef>
#include <thread>
#include <vector>

#include "felru.hpp"
#include "io.hpp"

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

  const size_t num = 8;
  std::atomic<size_t> hit(0);

  std::cout << "fe_lru:" << std::endl;
  using pd = bin_dictionary::par_pd<>;
  par_bin_cache<pd, 1 << 14, mul_shift> cache;

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
  for (size_t i = 0; i < num; ++i) {
    threads.emplace_back(fn, it, it + stride);
    it += stride;
  }

  for (auto& th : threads) th.join();

  auto ratio = (double)hit / (double)(stride * num);
  std::cout << "  -\n"
            << "    size: " << (1 << 14) << '\n'
            << "    hit_rate: " << ratio << std::endl;
}
