#pragma once

#include <memory>
#include <vector>

#include "sat_interface.hpp"

namespace triangulator {

class CardinalityNetworkBuilder {
public:
  CardinalityNetworkBuilder(std::shared_ptr<SatInterface> solver);
  void AddEqual(std::vector<Lit> input, int k);
  std::vector<Lit> EqualNetwork(std::vector<Lit> input);
private:
  std::shared_ptr<SatInterface> solver_;
  int free_var_;
  Lit Var(int id);
  std::vector<Lit> add_vars_;
  std::vector<Lit> HMerge(const std::vector<Lit>& A, const std::vector<Lit>& B, bool d1, bool d2);
  std::vector<Lit> SMerge(const std::vector<Lit>& A, const std::vector<Lit>& B, bool d1, bool d2);
  std::vector<Lit> HSort(const std::vector<Lit>& A, int l, int r, bool d1, bool d2);
  std::vector<Lit> Card(const std::vector<Lit>& A, int l, int r, int k, bool d1, bool d2);
};

class TotalizerBuilder {
 public:
  TotalizerBuilder(std::shared_ptr<SatInterface> solver);
  std::vector<Lit> Init(std::vector<Lit> input);
  void BuildToSize(int size);
 private:
  std::shared_ptr<SatInterface> solver_;
  int cur_size_;
  std::vector<std::vector<Lit>> n_lits_;
  std::vector<Lit> input_;
  void Build(int i, int l, int r, int size);
  void BuildNode(int ti, int li, int ri, int x);
};
} // namespace triangulator
