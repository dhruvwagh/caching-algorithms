#include <algorithm>
#include <cstddef>
#include <limits>
#include <list>
#include <unordered_map>

static const size_t max_size = 1 << 20;

struct belady {
  const size_t size = max_size;
  using element = std::pair<size_t, void*>;
  using cache_table = std::unordered_map<size_t, element>;
  cache_table table;

  using leaf = std::pair<size_t, cache_table::iterator>;
  std::vector<leaf> heap;

  using order = std::vector<size_t>;
  order chain;
  order::iterator head;

  belady(std::vector<size_t> future, size_t size)
      : size(size), chain(order(future.size(), 0)), head(chain.begin()) {
    std::unordered_map<size_t, size_t> history;
    for (size_t i = 0; i < future.size(); ++i) {
      auto item = future[i];
      auto prev = history.find(item);
      if (prev != history.end()) {
        chain[prev->second] = i - prev->second;
        prev->second = i;
      } else
        history.insert({item, i});
    }
    for (auto [key, val] : history)
      chain[val] = std::numeric_limits<size_t>::max();
  }

  auto set(size_t key, void* val) {
    auto lookup = table.find(key);
    auto hit = lookup != table.end();
    auto order = *head++;
    if (hit)
      lookup->second = {order, val};
    else {
      if (table.size() >= size) evict();
      lookup = table.insert({key, {order, val}}).first;
    }
    heap.push_back({order, lookup});
    std::push_heap(heap.begin(), heap.end(), cmp);
    return hit;
  }

  static bool cmp(leaf const& lhs, leaf const& rhs) {
    return lhs.first < rhs.first;
  }

  void evict() {
    auto victim = heap.front();
    std::pop_heap(heap.begin(), heap.end(), cmp);
    heap.pop_back();
    table.erase(victim.second);
  }

  void describe() {
    std::cout
        << "Cache Eviction Policy: Belady's algorithm\n"
        << "Cache size: " << size << std::endl;
  }
};

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
