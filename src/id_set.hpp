#pragma once

#include <map>
#include <vector>

namespace triangulator {
// Interface
template<typename T>
class IdSet {
public:
  int Insert(T);
  const T& Get(int) const;
  int IdOf(T) const;
  
  IdSet() = default;
  IdSet(const IdSet& rhs) = default;
  IdSet& operator=(const IdSet& rhs) = default;
private:
  std::map<T, int> ids_;
  std::vector<T> elements_;
};


// Implementation
template<typename T>
int IdSet<T>::Insert(T element) {
  if (ids_.count(element)) {
    return ids_[element];
  } else {
    ids_[element] = elements_.size();
    elements_.push_back(element);
    return elements_.size() - 1;
  }
}

template<typename T>
const T& IdSet<T>::Get(int i) const {
  return elements_[i];
}

template<typename T>
int IdSet<T>::IdOf(T element) const {
  return ids_.find(element)->second;
}
} // namespace triangulator
