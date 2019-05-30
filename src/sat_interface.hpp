#pragma once

// Integrate new SAT solvers by subclassing this.

#include <limits>
#include <vector>

namespace triangulator {

class Lit {
public:
  Lit();
  bool IsDef() const;
  bool IsTrue() const;
  bool IsFalse() const;
  static Lit TrueLit();
  static Lit FalseLit();
  Lit operator-() const;
  bool operator==(const Lit& rhs) const;

  Lit(const Lit& rhs) = default;
  Lit& operator=(const Lit& rhs) = default;
private:
  int value_;
  static constexpr int kTrueVal = std::numeric_limits<int>::max();

  Lit(int value);
  int Value() const;
  
  friend class SatInterface;
  friend class SatHelper;
};

class SatInterface {
public:
  virtual Lit NewVar() = 0;
  virtual void AddClause(std::vector<Lit> clause) = 0;
  virtual bool SolutionValue(Lit lit) = 0;
  virtual bool Solve(std::vector<Lit> assumptions, bool allow_simp) = 0;
  virtual void FreezeVar(Lit lit) = 0;
  virtual void PrintStats(int lvl) = 0;

  SatInterface() {}
  virtual ~SatInterface();

  SatInterface(const SatInterface&) = delete;
  SatInterface& operator=(const SatInterface&) = delete;
protected:
  Lit CreateLit(int value);
  int LitValue(Lit lit);
};

class SatHelper {
public:
  // Processes false and true literals and tautologies. If contains true, { TrueLit } is returned.
  // Also asserts IsDef()
  static std::vector<Lit> ProcessClause(const std::vector<Lit>& clause);
  SatHelper() = delete;
  SatHelper(const SatHelper&) = delete;
  SatHelper& operator=(const SatHelper&) = delete;
};
} // namespace triangulator
