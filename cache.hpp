#include <cstddef>
#include <list>
#include <unordered_map>

static const size_t max_size = 1 << 20;

struct lru {
  const size_t size = max_size;
  using order = std::list<size_t>;
  using element = std::pair<order::iterator, void*>;
  std::unordered_map<size_t, element> table;
  order lru_;

  lru(size_t size) : size(size) {
    table.reserve(size);
  }

  auto set(size_t key, void* val) {
    auto lookup = table.find(key);
    auto hit = lookup != table.end();
    if (hit)
      move_to_front(lookup->second.first);
    else {
      if (table.size() >= size) evict();
      lru_.push_front(key);
      table.insert({key, {lru_.begin(), val}});
    }
    return hit;
  }

  void evict() {
    auto victim = lru_.back();
    lru_.pop_back();
    table.erase(victim);
  }

  void move_to_front(order::iterator el) {
    lru_.splice(lru_.begin(), lru_, el);
  }

  void describe() {
    std::cout
        << "Cache Eviction Policy: LRU\n"
        << "Hash table size: " << size << std::endl;
  }
};

struct mru {
  const size_t size = max_size;
  using order = std::list<size_t>;
  using element = std::pair<order::iterator, void*>;
  std::unordered_map<size_t, element> table;
  order mru_;

  mru(size_t size) : size(size) {
    table.reserve(size);
  }

  auto set(size_t key, void* val) {
    auto lookup = table.find(key);
    auto hit = lookup != table.end();
    if (hit) {
      move_to_front(lookup->second.first);
    } else {
      if (table.size() >= size) evict();
      mru_.push_front(key);
      table.insert({key, {mru_.begin(), val}});
    }
    return hit;
  }

  void evict() {
    auto victim = mru_.front();
    mru_.pop_front();
    table.erase(victim);
  }

  void move_to_front(order::iterator el) {
    mru_.splice(mru_.begin(), mru_, el);
  }

  void describe() {
    std::cout
        << "Cache Eviction Policy: MRU\n"
        << "Hash table size: " << size << std::endl;
  }
};
