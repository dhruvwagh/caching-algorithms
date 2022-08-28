#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <list>
#include <set>
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
      : size(size), chain(order(future.size())), head(chain.begin()) {
    table.reserve(size);
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
      lookup->second.first = order;
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
  // reference : https://dl.acm.org/doi/10.1145/170036.170081
  const size_t size = max_size;
  struct frame {
    size_t key;
    size_t freq;
    size_t window[K];
    bool operator<(const frame other) const {
      return (freq < other.freq) ||
             ((freq == other.freq) && (window[0] < other.window[0]));
    }
  };
  using order = std::set<frame>;
  using element = std::pair<typename order::iterator, void*>;
  std::unordered_map<size_t, element> table;
  order lru_;
  size_t t = 0;

  lru_k(size_t size) : size(size) {
    table.reserve(size);
  }

  auto set(size_t key, void* val) {
    auto lookup = table.find(key);
    auto hit = lookup != table.end();
    if (hit)
      move_to_front(lookup->second.first);
    else {
      if (table.size() >= size) evict();
      auto it = lru_.insert({key, 0, {t}}).first;
      table.insert({key, {it, val}});
    }
    ++t;
    return hit;
  }

  void evict() {
    // recursive eviction subsidiary policy
    auto victim = lru_.begin();  // smallest in the set
    table.erase(victim->key);
    lru_.erase(victim);
  }

  void move_to_front(typename order::iterator& el) {
    auto node = lru_.extract(el);
    auto& frame = node.value();
    if (frame.freq == K - 1)
      std::rotate(frame.window, frame.window + 1, frame.window + K);
    else
      ++frame.freq;
    frame.window[frame.freq] = t;
    el = lru_.insert(std::move(node)).position;
  }

  void describe() {
    std::cout
        << "Cache Eviction Policy: LRU-" << K << '\n'
        << "Hash table size: " << size << std::endl;
  }
};

struct lfu {
  const size_t size = max_size;
  struct frame {
    size_t key;
    size_t freq;
    size_t last;
    bool operator<(const frame other) const {
      return (freq < other.freq) ||
             ((freq == other.freq) && (last < other.last));
    }
  };
  using order = std::set<frame>;
  using element = std::pair<typename order::iterator, void*>;
  std::unordered_map<size_t, element> table;
  order lru_;
  size_t t = 0;

  lfu(size_t size) : size(size) {
    table.reserve(size);
  }

  auto set(size_t key, void* val) {
    auto lookup = table.find(key);
    auto hit = lookup != table.end();
    if (hit)
      move_to_front(lookup->second.first);
    else {
      if (table.size() >= size) evict();
      auto it = lru_.insert({key, 0, t}).first;
      table.insert({key, {it, val}});
    }
    ++t;
    return hit;
  }

  void evict() {
    auto victim = lru_.begin();  // smallest in the set
    table.erase(victim->key);
    lru_.erase(victim);
  }

  void move_to_front(typename order::iterator& el) {
    auto node = lru_.extract(el);
    ++node.value().freq;
    el = lru_.insert(std::move(node)).position;
  }

  void describe() {
    std::cout
        << "Cache Eviction Policy: LFU" << '\n'
        << "Hash table size: " << size << std::endl;
  }
};

struct clock_lru {
  const size_t size = max_size;
  struct frame {
    bool bit;
    size_t key;
  };
  using order = std::list<frame>;
  using element = std::pair<order::iterator, void*>;
  std::unordered_map<size_t, element> table;
  order clock_;

  clock_lru(size_t size) : size(size) {
    table.reserve(size);
  }

  auto set(size_t key, void* val) {
    auto lookup = table.find(key);

    auto hit = lookup != table.end();
    if (hit)
      lookup->second.first->bit = true;
    else {
      while (table.size() >= size) evict();
      clock_.push_front({false, key});
      table.insert({key, {clock_.begin(), val}});
    }
    return hit;
  }

  void evict() {
    for (auto frame = clock_.end(); --frame != clock_.begin();) {
      if (frame->bit)
        frame->bit = false;
      else {
        auto victim = frame;
        table.erase(victim->key);
        rotate_to_front(victim);
        clock_.erase(victim);
        return;
      }
    }
  }

  void rotate_to_front(order::iterator el) {
    clock_.splice(clock_.begin(), clock_, el, clock_.end());
  }

  void describe() {
    std::cout
        << "Cache Eviction Policy: CLOCK\n"
        << "Hash table size: " << size << std::endl;
  }
};