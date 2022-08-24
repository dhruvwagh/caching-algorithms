#include <cstddef>
#include <list>
#include <unordered_map>

struct lru {
  const size_t size = 1 << 20;
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
    lru_.splice(lru_.begin(), lru_, el, std::next(el));
  }

  void describe() {
    std::cout
        << "Cache Eviction Policy: LRU\n"
        << "Hash table size: " << size << std::endl;
  }
};
