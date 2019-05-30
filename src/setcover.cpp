#include "setcover.hpp"

#include <vector>
#include <algorithm>
#include <map>
#include <cassert>

#include "utils.hpp"

namespace triangulator {
Setcover::Setcover(std::vector<std::vector<int>> sets) {
  gm_ = sets.size();
  int M = 0;
  for (const auto& s : sets) {
    for (int e : s) {
      M = std::max(M, e);
    }
  }
  M++;

  g_sets_.resize(sets.size());  
  sets_.resize(sets.size());
  where_.resize(M);
  sel_.resize(sets.size());
  cnt_.resize(M);
  tno_s_ = 0;
  git_ = 1;
  tno_.resize(sets.size() + M);
  no_.resize(sets.size() + M);
  plc_.resize(M);
  is_.resize(M);
  cc_.resize(M);
  um_.resize(M);
  heur_.resize(sets.size());

  for (int i = 0; i < gm_; i++) {
    g_sets_[i] = sets[i];
  }
}

void Setcover::Go(int i, int n) {
  if (i == n) {
    assert((int)cv_.size() <= upper_bound_);
    upper_bound_ = (int)cv_.size();
    if (opt_.size() == 0) {
      opt_ = cv_;
    }
    else {
      assert((int)cv_.size() < (int)opt_.size());
      opt_ = cv_;
    }
    assert(opt_.size() > 0);
    return;
  }
  bool f = false;
  for (int x : where_[i]) {
    if (sel_[x]) {
      f = true;
      break;
    }
  }
  if (!f) {
    int ss = tno_s_;
    for (int x : where_[i]) {
      if ((int)cv_.size() + 1 > upper_bound_) break;
      if ((int)cv_.size() + 1 == (int)opt_.size()) break;
      if (opt_.size() != 0 && (int)opt_.size() <= ok_cover_) break;
      if (no_[x]) continue;
      sel_[x] = 1;
      cv_.push_back(x);
      Go(i+1, n);
      sel_[x] = 0;
      cv_.pop_back();
      no_[x] = 1;
      tno_[tno_s_++] = x;
    }
    while (tno_s_ > ss) {
      no_[tno_[tno_s_ - 1]] = 0;
      tno_s_--;
    }
  }
  else {
    Go(i+1, n);
  }
}


std::vector<int> Setcover::Solve(std::vector<int> universe, int ok_cover, int upper_bound) {
  if (universe.size() == 0) return {};
  git_++;
  ok_cover_ = ok_cover;
  upper_bound_ = upper_bound;
  assert(ok_cover_ <= upper_bound_);
  utils::SortAndDedup(universe);
  int n = universe.size();
  for (int i = 0; i < n; i++) {
    plc_[universe[i]] = i;
    is_[universe[i]] = git_;
  }
  int m = 0;
  for (int i = 0; i < gm_; i++) {
    sets_[m].clear();
    for (int j = 0; j < (int)g_sets_[i].size(); j++) {
      if (is_[g_sets_[i][j]] == git_) {
        sets_[m].push_back(plc_[g_sets_[i][j]]);
      }
    }
    if (sets_[m].size() > 0) {
      for (int j = 0; j < (int)sets_[m].size(); j++) {
        if (j > 0) assert(sets_[m][j] > sets_[m][j-1]);
        assert(sets_[m][j] >= 0 && sets_[m][j] < n);
      }
      m++;
    }
  }
  sort(sets_.begin(), sets_.begin() + m);
  auto last = unique(sets_.begin(), sets_.begin() + m);
  m = last - sets_.begin();
  assert(m <= gm_);
  for (int i = 1; i < m; i++) {
    assert(sets_[i] > sets_[i-1]);
  }
  for (int i = 0; i < n; i++) {
    cnt_[i] = 0;
  }
  for (int i = 0; i < m; i++) {
    for (int x : sets_[i]) {
      cnt_[x]++;
    }
  }
  for (int i = 0; i < n; i++) {
    assert(cnt_[i] > 0);
    cc_[i] = {cnt_[i], i};
  }
  sort(cc_.begin(), cc_.begin() + n);
  for (int i = 0; i < n; i++) {
    um_[cc_[i].second] = i;
    where_[i].clear();
  }
  for (int i = 0; i < m; i++) {
    for (int& x : sets_[i]) {
      x = um_[x];
      where_[x].push_back(i);
    }
    std::sort(sets_[i].begin(), sets_[i].end());
  }
  for (int i = 0; i < n; i++) {
    for (int x : where_[i]) {
      int f = 0;
      for (int j = 0; j < (int)sets_[x].size(); j++) {
        if (sets_[x][j] == i) {
          f = (int)sets_[x].size() - j;
          break;
        }
      }
      assert(f > 0);
      heur_[x] = f;
    }
    assert(cc_[i].first == (int)where_[i].size());
    auto cmp = [&](int a, int b) {
      return heur_[a] > heur_[b];
    };
    sort(where_[i].begin(), where_[i].end(), cmp);
  }
  for (int i = 0; i < m; i++) sel_[i] = 0;
  opt_.clear();
  cv_.clear();
  Go(0, n);
  if (opt_.size() == 0) {
    opt_ = {-1};
  }
  return opt_;
}

} // namespace triangulator