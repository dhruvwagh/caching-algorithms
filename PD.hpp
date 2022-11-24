#include <folly/experimental/Select64.h>

#include <algorithm>
#include <atomic>
#include <bit>
#include <cinttypes>
#include <cstring>

namespace FELRU {

struct spin_lock {
  std::atomic_flag flag = ATOMIC_FLAG_INIT;

  void lock() {
    while (flag.test_and_set(std::memory_order_acquire)) {
#if defined(__cpp_lib_atomic_flag_test)
      while (flag.test(std::memory_order_relaxed))
#endif
        ;  // spin
    }
  }
  void unlock() { flag.clear(std::memory_order_release); }
};

// Static cast of pointer
// Can be overloaded to compress with known allocator
struct ptr {
  using type = uint64_t;

  type raw = 0;
  ptr() = default;
  ptr(type raw) noexcept : raw(raw) {}
  type getRaw() const noexcept { return raw; }
};

template <typename Ptr, typename LockT>
struct PD {
  template <class Instructions = folly::compression::instructions::Default>
  static inline uint16_t select(uint64_t el, uint16_t s) {
    uint64_t sel = folly::select64<Instructions>(el, static_cast<uint64_t>(s));
    return static_cast<uint16_t>(sel);
  }

  struct element {
    uint16_t index : 5;
    uint16_t fp : 11;
  };

  uint64_t header = 0xffff'ffffUL;
  element bins[27] = {element{0, 0}};
  int8_t occupancy = 0;
  LockT lock;

  using PtrType = typename Ptr::type;
  PtrType ptr_table[27] = {1, 2, 3, 4, 5, 6, 7, 8, 9,
                           10, 11, 12, 13, 14, 15, 16, 17, 18,
                           19, 20, 21, 22, 23, 24, 25, 26, 27};

  Ptr find(uint16_t fp) {
    return find(fp, [](PtrType) { return true; });
  }

  // we can confirm there is no collision by
  // dereferencing the resulting pointer
  template <typename Fn>
  Ptr find(uint16_t fp, Fn confirm) {
    uint16_t q = fp & 31U;
    uint16_t r = fp >> 5;

    uint16_t begin = q ? (select(header, q - 1) + 1 - q) : 0;
    uint16_t end = select(header, q) - q;

    element* slot = bins + begin;
    auto finder = [this, &confirm, &r](element el) {
      return (el.fp == r) && confirm(ptr_table[el.index]);
    };
    slot = std::find_if(bins + begin, bins + end, finder);
    if (slot == bins + end) return Ptr();

    std::rotate(bins + begin, slot, slot + 1);
    return Ptr(ptr_table[bins[begin].index]);
  }

  void insert(uint16_t fp, Ptr key) {
    if (occupancy >= 27) return;
    uint16_t q = fp & 31U;
    uint16_t r = fp >> 5;

    uint16_t sel = q ? (select(header, q - 1) + 1) : 0;
    uint64_t mask = (1UL << sel) - 1;
    header = (header & mask) | ((header & ~mask) << 1);

    uint16_t slot = q ? (select(header, q - 1) + 1 - q) : 0;
    std::memmove(bins + slot + 1, bins + slot,
                 (27 - slot - 1) * sizeof(element));

    uint16_t ptr_slot = occupancy;
    occupancy = ptr_table[occupancy];
    bins[slot] = {ptr_slot, r};
    ptr_table[ptr_slot] = key.getRaw();
  }

  void remove(uint16_t fp, Ptr key) {
    uint16_t q = fp & 31U;
    uint16_t r = fp >> 5;

    uint16_t begin = q ? (select(header, q - 1) + 1 - q) : 0;
    uint16_t end = select(header, q) - q;

    auto raw = key.getRaw();
    auto finder = [this, &raw, &r](element el) {
      return (el.fp == r) && (ptr_table[el.index] == raw);
    };
    auto slot = std::find_if(bins + begin, bins + end, finder) - bins;
    if (slot >= end) return;

    uint64_t mask = (1UL << slot) - 1;
    header = (header & mask) | ((header >> 1) & ~mask);

    auto prev = bins[slot].index;
    std::memmove(bins + slot, bins + slot + 1,
                 (27 - slot - 1) * sizeof(element));
    ptr_table[prev] = occupancy;
    occupancy = prev;
  }

  template <typename F>
  void for_each(F func) {
    std::for_each(bins, bins + occupancy, [this, &func](element el) {
      auto ptr = Ptr(ptr_table[el.index]);
      func(ptr);
    });
  }
};

};  // namespace FELRU
