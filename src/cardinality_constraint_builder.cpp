#include "cardinality_constraint_builder.hpp"

#include <vector>
#include <memory>
#include <cassert>
#include <iostream>

#include "sat_interface.hpp"
#include "utils.hpp"

namespace triangulator {

CardinalityNetworkBuilder::CardinalityNetworkBuilder(std::shared_ptr<SatInterface> solver) : solver_(solver), free_var_(0) { }
std::vector<Lit> CardinalityNetworkBuilder::EqualNetwork(std::vector<Lit> input) {
  int n = input.size();
  assert(n > 0);
  int k = 1;
  while (k < (int)input.size()) k *= 2;
  while ((int)input.size() < k) {
    Lit nl = Var(free_var_++);
    solver_->AddClause({-nl});
    input.push_back(nl);
  }
  assert((int)input.size() == k);
  auto output = Card(input, 0, (int)input.size()-1, k, true, true);
  assert(output.size() == input.size());
  while ((int)output.size() > n) {
    solver_->AddClause({-output.back()});
    output.pop_back();
  }
  return output;
}
void CardinalityNetworkBuilder::AddEqual(std::vector<Lit> input, int k) {
  assert(k > 0 && k < (int)input.size());
  if ((int)input.size() - k < k) {
    k = (int)input.size() - k;
    for (Lit& lit : input) {
      lit = -lit;
    }
  }
  int k2 = 1;
  while (k2 <= k) k2 *= 2;
  while ((int)input.size()%k2 != 0) {
    Lit nl = Var(free_var_++);
    solver_->AddClause({-nl});
    input.push_back(nl);
  }
  auto output = Card(input, 0, (int)input.size()-1, k2, true, true);
  solver_->AddClause({output[k - 1]});
  solver_->AddClause({-output[k]});
}
std::vector<Lit> CardinalityNetworkBuilder::Card(const std::vector<Lit>& A, int l, int r, int k, bool d1, bool d2) {
  int n = (r-l+1);
  assert(n%k == 0);
  if (n == k) {
    return HSort(A, l, r, d1, d2);
  }
  else {
    std::vector<Lit> a = Card(A, l, l+k-1, k, d1, d2);
    std::vector<Lit> b = Card(A, l+k, r, k, d1, d2);
    assert((int)a.size() == k && (int)b.size() == k);
    return SMerge(a, b, d1, d2);
  }
}
std::vector<Lit> CardinalityNetworkBuilder::HSort(const std::vector<Lit>& A, int l, int r, bool d1, bool d2) {
  int le = r-l+1;
  assert(le >= 2);
  assert(le%2 == 0);
  if (le == 2) {
    std::vector<Lit> a = {A[l]};
    std::vector<Lit> b = {A[r]};
    return HMerge(a, b, d1, d2);
  }
  else {
    int m = (l+r)/2;
    std::vector<Lit> a = HSort(A, l, m, d1, d2);
    std::vector<Lit> b = HSort(A, m+1, r, d1, d2);
    return HMerge(a, b, d1, d2);
  }
}
std::vector<Lit> CardinalityNetworkBuilder::SMerge(const std::vector<Lit>& A, const std::vector<Lit>& B, bool d1, bool d2) {
  // use the bit reversal trick to do SMerge without recursion
  assert(A.size() == B.size());
  int n = A.size();
  std::vector<Lit> c(2*n);
  for (int i = 0; i < n; i++) {
    c[2*i] = Var(free_var_++);
    if (i < n/2) c[2*i+1] = Var(free_var_++);
    if (d1) {
      if (i < n/2) solver_->AddClause({-A[i], -B[i], c[2*i+1]});
      solver_->AddClause({-A[i], c[2*i]});
      solver_->AddClause({-B[i], c[2*i]});
    }
    if (d2) {
      solver_->AddClause({A[i], B[i], -c[2*i]});
      if (i < n/2) solver_->AddClause({A[i], -c[2*i+1]});
      if (i < n/2) solver_->AddClause({B[i], -c[2*i+1]});
    }
  }
  for (int i = 0; i < n; i++) {
    int u = 0;
    for (int j = 1; j < n; j *= 2) {
      u *= 2;
      if (i & j) u++;
    }
    if (i < u) {
      std::swap(c[2*i], c[2*u]);
      std::swap(c[2*i+1], c[2*u+1]);
    }
  }
  int oc = 0;
  for (int le = 4; le <= 2*n; le *= 2) {
    for (int i = 0; i < 2*n; i+=le) {
      if (le == 2*n) oc = 1;
      for (int j = 0; j < le/4; j++) {
        if (d1) {
          if (j+1 < le/4 || !oc) solver_->AddClause({-c[i+j+1], -c[i+j+le/2], Var(free_var_+2*j+1)});
          solver_->AddClause({-c[i+j+1], Var(free_var_+2*j)});
          solver_->AddClause({-c[i+j+le/2], Var(free_var_+2*j)});
        }
        if (d2) {
          solver_->AddClause({c[i+j+1], c[i+j+le/2], -Var(free_var_+2*j)});
          if (j+1 < le/4 || !oc) solver_->AddClause({c[i+j+1], -Var(free_var_+2*j+1)});
          if (j+1 < le/4 || !oc) solver_->AddClause({c[i+j+le/2], -Var(free_var_+2*j+1)});
        }
      }
      for (int j = 1; j < le/2; j++) c[i+j] = Var(free_var_++);
      if (!oc) c[i+le/2] = Var(free_var_++);
      oc ^= 1;
    }
  }
  c.resize(n);
  return c;
}
std::vector<Lit> CardinalityNetworkBuilder::HMerge(const std::vector<Lit>& A, const std::vector<Lit>& B, bool d1, bool d2) {
  // use the bit reversal trick to do HMerge without recursion
  assert(A.size() == B.size());
  int n = A.size();
  std::vector<Lit> c(2*n);
  for (int i = 0; i < n; i++) {
    c[2*i] = Var(free_var_++);
    c[2*i+1] = Var(free_var_++);
    if (d1) {
      solver_->AddClause({-A[i], -B[i], c[2*i+1]});
      solver_->AddClause({-A[i], c[2*i]});
      solver_->AddClause({-B[i], c[2*i]});
    }
    if (d2) {
      solver_->AddClause({A[i], B[i], -c[2*i]});
      solver_->AddClause({A[i], -c[2*i+1]});
      solver_->AddClause({B[i], -c[2*i+1]});
    }
  }
  for (int i = 0; i < n; i++) {
    int u = 0;
    for (int j = 1; j < n; j *= 2) {
      u *= 2;
      if (i & j) u++;
    }
    if (i < u) {
      std::swap(c[2*i], c[2*u]);
      std::swap(c[2*i+1], c[2*u+1]);
    }
  }
  for (int le = 4; le <= 2*n; le *= 2) {
    for (int i = 0; i < 2*n; i+=le) {
      for (int j = 0; j+1 < le/2; j++) {
        if (d1) {
          solver_->AddClause({-c[i+j+1], -c[i+j+le/2], Var(free_var_+2*j+1)});
          solver_->AddClause({-c[i+j+1], Var(free_var_+2*j)});
          solver_->AddClause({-c[i+j+le/2], Var(free_var_+2*j)});
        }
        if (d2) {
          solver_->AddClause({c[i+j+1], c[i+j+le/2], -Var(free_var_+2*j)});
          solver_->AddClause({c[i+j+1], -Var(free_var_+2*j+1)});
          solver_->AddClause({c[i+j+le/2], -Var(free_var_+2*j+1)});
        }
      }
      for (int j = 1; j+1 < le; j++) c[i+j] = Var(free_var_++);
    }
  }
  return c;
}
Lit CardinalityNetworkBuilder::Var(int id) {
  while (id >= add_vars_.size()) {
    add_vars_.push_back(solver_->NewVar());
  }
  return add_vars_[id];
}

TotalizerBuilder::TotalizerBuilder(std::shared_ptr<SatInterface> solver) : solver_(solver), cur_size_(-1) {}
std::vector<Lit> TotalizerBuilder::Init(std::vector<Lit> input) {
  input_ = input;
  if (input_.size() <= 1) {
    cur_size_ = (int)input.size();
    return input;
  }
  n_lits_.resize(2);
  n_lits_[1].push_back(Lit::TrueLit());
  std::vector<Lit> output;
  for (int i = 0; i < (int)input.size(); i++){
    n_lits_[1].push_back(solver_->NewVar());
    output.push_back(n_lits_[1].back());
  }
  return output;
}
void TotalizerBuilder::BuildToSize(int size) {
  assert(size <= (int)input_.size());
  assert(size >= cur_size_);
  assert(size >= 1);
  if (input_.size() <= 1 || cur_size_ >= size) {
    return;
  }
  Build(1, 0, (int)input_.size()-1, size);
  cur_size_ = size;
}
void TotalizerBuilder::Build(int i, int l, int r, int size) {
  // std::cout<<"build "<<i<<" "<<l<<" "<<r<<" "<<size<<std::endl;
  assert(size >= 1);
  assert(l<=r);
  while (i >= (int)n_lits_.size()) n_lits_.push_back({});
  if (l == r) {
    if (n_lits_[i].size() == 0) {
      n_lits_[i].push_back(Lit::TrueLit());
    }
    if (n_lits_[i].size() == 1) {
      n_lits_[i].push_back(input_[l]);
    }
    assert(n_lits_[i].size() == 2);
  } else {
    int m = (l+r)/2;
    Build(i*2, l, m, size);
    Build(i*2+1, m+1, r, size);
    for (int x = cur_size_ + 1; x <= std::min(size, r-l+1); x++) {
      BuildNode(i, i*2, i*2+1, x);
    }
  }
}
void TotalizerBuilder::BuildNode(int ti, int li, int ri, int x) {
  // std::cout<<"bn "<<ti<<" "<<li<<" "<<ri<<" "<<x<<std::endl;
  assert(ti>=1&&li>ti&&ri>li);
  if (ti == 1) {
    assert((int)n_lits_[ti].size() >= x+1);
  } else {
    if (x == 0) {
      n_lits_[ti].push_back(Lit::TrueLit());
    } else {
      n_lits_[ti].push_back(solver_->NewVar());
      solver_->FreezeVar(n_lits_[ti].back());
    }
    assert((int)n_lits_[ti].size() == x+1);
  }
  for (int a = 0; a <= x; a++){
    int b = x-a;
    // std::cout<<"asdf "<<a<<" "<<b<<" "<<x<<std::endl;
    if (a >= (int)n_lits_[li].size()) continue;
    if (b >= (int)n_lits_[ri].size()) continue;
    // std::cout<<"asdf2 "<<a<<" "<<b<<" "<<x<<std::endl;
    solver_->AddClause({-n_lits_[li][a], -n_lits_[ri][b], n_lits_[ti][x]});
  }
}
} // namespace triangulator
