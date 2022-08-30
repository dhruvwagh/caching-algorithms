#include <algorithm>
#include <array>
#include <cinttypes>
#include <deque>
#include <iostream>
#include <optional>

static const size_t max_entries = (1 << 20) / 27;

static size_t bucket[27] = {0};

struct mul_shift {
  static const uint64_t hi = 0x51502a8334304aae;
  static const uint64_t lo = 0x9743df29cdf1096f;
  uint64_t operator()(uint64_t x) {
    return static_cast<unsigned __int128>(x) * ((static_cast<unsigned __int128>(hi) << 64) | lo) >> 64;
  };
};

template <class pd, typename Hash = std::identity>
struct felru {
  // reference : https://github.com/jbapple/crate-dictionary
  const size_t size = max_entries * 27;
  const size_t entries = max_entries;
  std::vector<pd> pds;
  Hash hasher;

  felru(size_t size) : size(size), entries(size / 27) {
    pds.resize(entries);
  }

  auto set(size_t key, void* val) {
    auto hash = hasher(key);
    auto b = hash % entries;
    uint16_t fp = static_cast<uint16_t>(hash / entries);
    auto& pd_ = pds[b];
    auto lookup = pd_.find(fp);
    auto hit = lookup.has_value();
    if (!hit) pd_.insert(fp, val);
    return hit;
  }

  void describe() {
    std::cout
        << "Cache Eviction Policy: FELRU\n"
        << "Cache size: " << size << std::endl;
  }

  std::array<size_t, 27> buckets() {
    std::array<size_t, 27> b;
    for (size_t i = 0; i < 27; ++i) {
      b[i] = bucket[i];
      bucket[i] = 0;
    }
    return b;
  }
};

struct bin_dictionary {
  struct element {
    uint16_t fp;
    void* val;
    bool operator==(const element& other) const {
      return fp == other.fp;
    }
  };
  struct bin : std::deque<element> {
    bool operator<(const bin& other) const {
      return size() < other.size();
    }
  };
  std::array<bin, 32> bins;
  size_t occupancy = 0;

  std::optional<element> find(uint16_t fp) {
    uint16_t q = fp & 31U;
    uint16_t r = fp >> 5;
    auto bin_ = bins[q];
    auto slot = std::find(bin_.begin(), bin_.end(), element{r, nullptr});
    if (slot == bin_.end())
      return {};
    else {
      std::rotate(bin_.begin(), slot, slot + 1);
      return bin_.front();
    }
  }

  void insert(uint16_t fp, void* val) {
    for (; occupancy >= 27; --occupancy) evict();
    uint16_t q = fp & 31U;
    uint16_t r = fp >> 5;
    bins[q].push_back({r, val});
    ++occupancy;
  }

  void evict() {
    auto maximal = std::max_element(bins.begin(), bins.end());
    maximal->pop_front();
    ++bucket[maximal->size()];
  }
};