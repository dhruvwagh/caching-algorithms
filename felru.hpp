#include <algorithm>
#include <array>
#include <cinttypes>
#include <deque>
#include <iostream>

static const size_t max_entries = (1 << 20) / 27;

template <class pd>
struct felru {
  // reference : https://github.com/jbapple/crate-dictionary
  const size_t size = max_entries * 27;
  const size_t entries = max_entries;
  std::vector<pd> pds;

  felru(size_t size) : size(size), entries(size / 27) {
    pds.resize(entries);
  }

  auto set(size_t key, void* val) {
    auto hash = hasher(key);
    auto [b, fp] = std::pair{hash % entries, static_cast<uint16_t>(hash / entries)};
    auto& pd_ = pds[b];
    auto lookup = pd_.find(fp);
    auto hit = lookup != pd_.end();
    if (!hit) pd_.insert(fp, val);
    return hit;
  }

  static const uint64_t hi = 0x51502a8334304aae;
  static const uint64_t lo = 0x9743df29cdf1096f;

  uint64_t hasher(uint64_t x) {
    return static_cast<unsigned __int128>(x) * ((static_cast<unsigned __int128>(hi) << 64) | lo) >> 64;
  };

  void describe() {
    std::cout
        << "Cache Eviction Policy: FELRU\n"
        << "Cache size: " << size << std::endl;
  }
};

struct bin_dictionary {
  using element = std::pair<uint16_t, void*>;
  using bin = std::deque<element>;
  std::array<bin, 32> bins;
  size_t occupancy = 0;

  auto find(uint16_t fp) {
    uint16_t q = fp & 31U;
    uint16_t r = fp >> 5;
    auto cmp = [=](auto entry) { return entry.first == r; };
    auto bin_ = bins[q];
    auto slot = std::find_if(bin_.begin(), bin_.end(), cmp);
    if (slot == bin_.end())
      return end();
    else {
      std::rotate(bin_.begin(), slot, slot + 1);
      return bin_.begin();
    }
  }

  void insert(uint16_t fp, void* val) {
    auto cmp = [](auto a, auto b) { return a.size() < b.size(); };
    for (; occupancy >= 27; --occupancy)
      std::max_element(bins.begin(), bins.end(), cmp)->pop_front();

    uint16_t q = fp & 31U;
    uint16_t r = fp >> 5;
    bins[q].push_back({r, val});
    ++occupancy;
  }

  bin::iterator begin() { return bins.front().begin(); }
  bin::iterator end() { return bins.back().end(); }
};