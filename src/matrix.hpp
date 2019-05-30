#pragma once

#include <vector>
#include <cstdlib>

namespace triangulator {
// Interface
template<typename T>
class Matrix {
public:
  Matrix();
  explicit Matrix(size_t rows, size_t columns);
  void Resize(size_t rows, size_t columns);
  T* operator[](size_t pos);
  const T* operator[](size_t pos) const;
private:
  size_t rows_, columns_;
  std::vector<T> data_;
};

// Implementation
template<typename T>
Matrix<T>::Matrix(size_t rows, size_t columns) : rows_(rows), columns_(columns), data_(rows * columns) { }

template<typename T>
Matrix<T>::Matrix() : Matrix(0, 0) {}

template<typename T>
void Matrix<T>::Resize(size_t rows, size_t columns) {
  rows_ = rows;
  columns_ = columns;
  data_.resize(rows * columns);
}

template<typename T>
T* Matrix<T>::operator[](size_t pos) {
  return data_.data() + pos * columns_;
}

template<typename T>
const T* Matrix<T>::operator[](size_t pos) const {
  return data_.data() + pos * columns_;
}

} // namespace triangulator
