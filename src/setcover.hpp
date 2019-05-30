#pragma once

#include <vector>

namespace triangulator {
class Setcover {
 public:
  Setcover(std::vector<std::vector<int>> sets);
  std::vector<int> Solve(std::vector<int> universe, int ok_cover, int upper_bound);
 private:
  int gm_, upper_bound_, tno_s_, ok_cover_, git_;
  std::vector<std::vector<int>> g_sets_, where_, sets_;
  std::vector<int> cv_, opt_, sel_, cnt_, no_, tno_, plc_, is_, um_, heur_;
  std::vector<std::pair<int, int>> cc_;

  void Go(int i, int n);
};

} // namespace triangulator