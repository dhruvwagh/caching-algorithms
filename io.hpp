#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

std::vector<size_t> load_zipf(std::string fname) {
  std::ifstream f(fname);

  std::vector<size_t> io;
  io.reserve(1UL << 23);

  std::string dash;
  for (size_t key = 0UL; (f >> dash) && (f >> key);)
    io.push_back(key);
  return io;
}

// See http://www.wikibench.eu/?page_id=60 for Wiki traces

std::vector<size_t> load_wiki(std::string fname) {
  std::ifstream f(fname);

  std::hash<std::string> key;
  std::map<size_t, size_t> timeline;
  std::string link, flag;
  double time;
  for (size_t counter = 0UL;
       (f >> counter) &&
       (f >> time) &&
       (f >> link) &&
       (f >> flag);) {
    timeline.insert({counter, key(link)});
  }

  std::vector<size_t> io;
  io.reserve(timeline.size());
  for (auto [_, key] : timeline)
    io.push_back(key);
  return io;
}

// See https://researcher.watson.ibm.com/researcher/view_person_subpage.php?id=4700 for
// ARC traces

std::vector<size_t> load_arc(std::string fname) {
  std::ifstream f(fname);

  std::vector<size_t> io;
  io.reserve(1UL << 23);

  std::string dummy;
  for (size_t start, length = 0UL;
       (f >> start) &&
       (f >> length) &&
       (f >> dummy) &&
       (f >> dummy);) {
    for (auto key = start; key < start + length; ++key)
      io.push_back(key);
  }
  return io;
}
