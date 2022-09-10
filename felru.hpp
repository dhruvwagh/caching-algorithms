#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cinttypes>
#include <cstring>
#include <deque>
#include <functional>
#include <immintrin.h>
#include <iostream>
#include <optional>


static const size_t max_entries = (1 << 20) / 27;

struct mul_shift {
  static const uint64_t hi = 0x51502a8334304aae;
  static const uint64_t lo = 0x9743df29cdf1096f;
  uint64_t operator()(uint64_t x) {
    return static_cast<unsigned __int128>(x) * ((static_cast<unsigned __int128>(hi) << 64) | lo) >> 64;
  };
};

template <class pd, typename Hash = std::identity>
struct bin_cache {
  // reference : https://github.com/jbapple/crate-dictionary

  const size_t size = max_entries * 27;
  const size_t entries = max_entries;
  std::vector<pd> pds;
  Hash hasher;

  bin_cache(size_t size) : size(size), entries(size / 27) {
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
};

template <class pd, typename Hash = std::identity>
struct par_bin_cache {
  const size_t entries = max_entries;
  std::vector<pd> pds;
  Hash hasher;

  par_bin_cache(size_t size) : entries(size / 27), pds(entries) {}
  
  auto set(size_t key, void* val) {
    auto hash = hasher(key);
    auto b = hash % entries;
    uint16_t fp = static_cast<uint16_t>(hash / entries);
    auto& pd_ = pds[b];

    while (pd_.locked())
      ;  // spin lock

    auto lookup = pd_.find(fp);
    auto hit = lookup.has_value();
    if (!hit) pd_.insert(fp, val);

    pd_.release();
    return hit;
  }
};

namespace bin_dictionary {

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
using cache = std::array<bin, 32>;

struct evict_q {
  bin& operator()(cache& bins, uint16_t q) {
    for (uint16_t e = (q + 1) & 31U; e != q; e = (e + 1) & 31U)
      if (!bins[e].empty())
        return bins[e];
    return bins[q];
  }
};

template <typename Evict = evict_q>
struct lru {
  Evict evict;
  void operator()(cache& bins, uint16_t q) {
    auto& victim = evict(bins, q);
    victim.pop_back();
  }
};

template <typename Policy = lru<>>
struct pd {
  cache bins;
  size_t occupancy = 0;
  Policy evict;

  std::optional<element> find(uint16_t fp) {
    uint16_t q = fp & 31U;
    uint16_t r = fp >> 5;
    auto& bin_ = bins[q];
    auto slot = std::find(bin_.begin(), bin_.end(), element{r, nullptr});
    if (slot == bin_.end())
      return {};
    else {
      std::rotate(bin_.begin(), slot, slot + 1);
      return bin_.front();
    }
  }

  void insert(uint16_t fp, void* val) {
    uint16_t q = fp & 31U;
    for (; occupancy >= 27; --occupancy) evict(bins, q);
    uint16_t r = fp >> 5;
    bins[q].push_front({r, val});
    ++occupancy;
  }
};

template <typename Policy = lru<>>
struct par_pd : pd<Policy> {
  std::atomic<bool> atom;

  par_pd() : atom(false) {}

  bool locked() { return atom.exchange(true, std::memory_order_acquire); }
  void release() { atom.store(false, std::memory_order_release); }
};

};  // namespace bin_dictionary


namespace fano_elias {

inline uint64_t pdep(uint64_t mask, uint64_t el) {
  return _pdep_u64(mask, el);
}

inline uint64_t bit_index(uint64_t el, uint16_t s) {
  return pdep(1UL << s, el);
}

inline uint16_t select(uint64_t el, uint16_t s) {
  return std::countr_zero(bit_index(el, s));
}

struct element {
  uint16_t index : 5;
  uint16_t fp : 11;
  bool operator==(const element& other) const {
    return fp == other.fp;
  }
};
  
using cache = element[27];

struct evict_q {
  uint64_t operator()(uint64_t header, uint16_t q) {
    uint64_t pivot = bit_index(header, q);
    uint64_t h =  (pivot - 1) | header;
    if (h == 0x7ff'ffff'ffff'ffffUL) h = ~(pivot - 1) | header;
    return ~h & (h + 1);
  }
};



template <typename Evict = evict_q>
struct pd {
  uint64_t header = 0xffff'ffffUL;
  element bins[27] = {element{0, 0}};
  int8_t freelist = 0;
  uint64_t ptr_table[27] =
    { 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13,
     14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27};
  
  Evict policy;
  void evict(uint16_t q) {
    auto victim = policy(header, q) - 1;

    auto prefix = ~victim & header;
    prefix = (-prefix & prefix) - 1;
    auto slot = std::popcount(~header & (prefix >> 1));

    header = (victim & header) | (~victim & (header >> 1));
    
    auto prev = bins[slot].index;
    std::memmove(bins + slot, bins + slot + 1, (27 - slot - 1) * sizeof(element));
    ptr_table[prev] = freelist;
    freelist = prev;
  }

  std::optional<void*> find(uint16_t fp) {
    uint16_t q = fp & 31U;
    uint16_t r = fp >> 5;

    uint16_t begin = q ? (select(header, q - 1) + 1 - q) : 0;
    uint16_t end = select(header, q) - q;

    auto slot = std::find(bins + begin, bins + end, element{0, r});
    if (slot == bins + end)
      return {};
    else {
      std::rotate(bins + begin, slot, slot + 1);
      return (void*) ptr_table[bins[begin].index];
    }
  }

  void insert(uint16_t fp, void* val) {
    uint16_t q = fp & 31U;
    if (freelist >= 27) evict(q);
    uint16_t r = fp >> 5;

    uint64_t mask = q ? ((bit_index(header, q - 1) << 1) - 1) : 0;
    header = (header & mask) | ((header & ~mask) << 1);
    
    uint16_t slot = q ? (select(header, q - 1) + 1 - q) : 0;
    std::memmove(bins + slot + 1, bins + slot, (27 - slot - 1) * sizeof(element));

    uint16_t ptr_slot = freelist;
    freelist = ptr_table[freelist];
    bins[slot] = {ptr_slot, r};
    ptr_table[ptr_slot] = (uint64_t)val;
  }
};


template <typename Evict = evict_q>
struct par_pd : pd<Evict> {
  std::atomic<bool> atom;

  par_pd() : atom(false) {}

  bool locked() { return atom.exchange(true, std::memory_order_acquire); }
  void release() { atom.store(false, std::memory_order_release); }
};

}; // namespace fano_elias
