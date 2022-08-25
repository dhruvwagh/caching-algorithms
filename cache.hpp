#include <algorithm>
#include <array>
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

  using leaf = std::pair<size_t, size_t>;
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
      while (table.size() >= size) evict();
      lookup = table.insert({key, {order, val}}).first;
    }
    heap.push_back({order, key});
    std::push_heap(heap.begin(), heap.end(), cmp);
    return hit;
  }

  static bool cmp(leaf const& lhs, leaf const& rhs) {
    return lhs.first < rhs.first;
  }

  void evict() {
    auto [order, key] = heap.front();
    std::pop_heap(heap.begin(), heap.end(), cmp);
    heap.pop_back();
    if (auto victim = table.find(key);
        victim != table.end() &&
        order == victim->second.first)
      table.erase(victim);
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

template <size_t K>
struct lru_k {
  const size_t size = max_size;
  using order = std::list<size_t>;
  using k_last = std::pair<order::iterator, size_t>;
  using element = std::pair<k_last, void*>;
  std::unordered_map<size_t, element> table;
  std::array<order, K> lru_n;

  lru_k(size_t size) : size(size) {
    table.reserve(size);
  }

  auto set(size_t key, void* val) {
    auto lookup = table.find(key);
    auto hit = lookup != table.end();
    if (hit)
      lookup->second.first = move_to_front(lookup->second.first);
    else {
      if (table.size() >= size) evict();
      auto& lru_ = lru_n[0];
      lru_.push_front(key);
      table.insert({key, {{lru_.begin(), 0}, val}});
    }
    return hit;
  }

  void evict() {
    for (size_t i = K; i--;)
      if (auto& lru_ = lru_n[i]; !lru_.empty()) {
        auto victim = lru_.back();
        lru_.pop_back();
        table.erase(victim);
        return;
      }
  }

  k_last move_to_front(k_last el) {
    auto [it, k] = el;
    auto& lru_ = lru_n[k];
    if (k == K - 1) {
      lru_.splice(lru_.begin(), lru_, it);
      return el;
    } else {
      auto& lru_next = lru_n[k + 1];
      lru_next.push_front(*it);
      lru_.erase(it);
      return {lru_next.begin(), k + 1};
    }
  }

  void describe() {
    std::cout
        << "Cache Eviction Policy: LRU-" << K << '\n'
        << "Hash table size: " << size << std::endl;
  }
};
