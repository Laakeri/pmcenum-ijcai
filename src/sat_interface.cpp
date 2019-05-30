#include "sat_interface.hpp"

#include <vector>
#include <set>
#include <cassert>

#include "utils.hpp"

namespace triangulator {
Lit::Lit() {
  value_ = 0;
}
Lit::Lit(int value) : value_(value) {}
Lit Lit::TrueLit() {
  return Lit(kTrueVal);
}
Lit Lit::FalseLit() {
  return Lit(-kTrueVal);
}
int Lit::Value() const {
  return value_;
}
bool Lit::IsDef() const {
  return value_ != 0;
}
bool Lit::IsTrue() const {
  return value_ == kTrueVal;
}
bool Lit::IsFalse() const {
  return value_ == -kTrueVal;
}
Lit Lit::operator-() const {
  return Lit(-value_);
}
bool Lit::operator==(const Lit& rhs) const {
  return value_ == rhs.value_;
}
Lit SatInterface::CreateLit(int value) {
  return Lit(value);
}
int SatInterface::LitValue(Lit lit) {
  return lit.Value();
}
SatInterface::~SatInterface() { }
std::vector<Lit> SatHelper::ProcessClause(const std::vector<Lit>& clause) {
  std::vector<Lit> pro_clause;
  std::set<int> lit_values;
  for (Lit lit : clause) {
    assert(lit.IsDef());
    if (lit.IsTrue()) {
      return { Lit::TrueLit() };
    } else if (!lit.IsFalse()) {
      pro_clause.push_back(lit);
      if (lit_values.count(-lit.Value())) return { Lit::TrueLit() };
      lit_values.insert(lit.Value());
    }
  }
  return pro_clause;
}
} // namespace triangulator
