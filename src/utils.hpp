#pragma once

#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <chrono>

namespace triangulator {
// Interface

namespace utils {
template<typename T>
bool IsSorted(const std::vector<T>& vec);

template<typename T>
void SortAndDedup(std::vector<T>& vec);

template<typename T>
void InitZero(std::vector<T>& vec, size_t size);

template<typename T>
std::vector<std::pair<T, T> > CompleteEdges(const std::vector<T>& vertices);

template<typename T>
std::vector<T> PermInverse(const std::vector<T>& perm);

template<typename... Args>
void Warning(Args... message);

template<typename... Args>
void ErrorDie(Args... message);
} // namespace utils

class Log {
public:
  template<typename T, typename... Args>
  static void P(T first_message, Args... message);
  
  template<typename T>
  static void Write(int lvl, T message);

  template<typename T, typename... Args>
  static void Write(int lvl, T first_message, Args... message);

  static void SetLogLevel(int lvl);
private:
  template<typename T>
  static void WriteImpl(T message);
  static int log_level_;
};

class Timer {
private:
  bool timing;
  std::chrono::duration<double> elapsedTime;
  std::chrono::time_point<std::chrono::steady_clock> startTime;
public:
  Timer();
  void start();
  void stop();
  std::chrono::duration<double> getTime();
};


// Implementation

namespace utils {
template<typename T>
bool IsSorted(const std::vector<T>& vec) {
  for (int i = 1; i < (int)vec.size(); i++) {
    if (vec[i] < vec[i-1]) return false;
  }
  return true;
}

template<typename T>
void SortAndDedup(std::vector<T>& vec) {
  std::sort(vec.begin(), vec.end());
  vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
}

template<typename T>
void InitZero(std::vector<T>& vec, size_t size) {
  vec.resize(size);
  std::fill(vec.begin(), vec.begin() + size, 0);
}

template<typename T>
std::vector<std::pair<T, T> > CompleteEdges(const std::vector<T>& vertices) {
  if (vertices.size() < 2) return { };
  std::vector<std::pair<T, T> > edges;
  edges.reserve(vertices.size() * (vertices.size() - 1) / 2); // care with unsigned
  for (int i = 0; i < vertices.size(); i++) {
    for (int ii = i + 1; ii < vertices.size(); ii++) {
      edges.push_back({vertices[i], vertices[ii]});
    }
  }
  return edges;
}

template<typename T>
std::vector<T> PermInverse(const std::vector<T>& perm) {
  std::vector<T> ret(perm.size());
  for (int i = 0; i < perm.size(); i++) {
    ret[perm[i]] = i;
  }
  return ret;
}

template<typename... Args>
void Warning(Args... message) {
  Log::Write(5, "[WARNING] ", message...);
}
// TODO: this is bad idea because assert shows line number
template<typename... Args>
void ErrorDie(Args... message) {
  Log::Write(1, "\033[0;31m[FATAL ERROR]\033[0m ", message...);
  std::exit(EXIT_FAILURE);
}
} // namespace utils
template<typename T>
void Log::WriteImpl(T message) {
  std::cerr<<message;
}
template<>
inline void Log::WriteImpl(std::vector<char> message) {
  std::cerr<<"{ ";
  for (char e : message) {
    std::cerr<<(int)e<<" ";
  }
  std::cerr<<"}";
}
template<>
inline void Log::WriteImpl(std::vector<int> message) {
  std::cerr<<"{ ";
  for (int e : message) {
    std::cerr<<e<<" ";
  }
  std::cerr<<"}";
}
template<typename T>
void Log::Write(int lvl, T message) {
  if (lvl > log_level_) return;
  WriteImpl(message);
  std::cerr<<std::endl;
}
template<typename T, typename... Args>
void Log::Write(int lvl, T first_message, Args... message) {
  if (lvl > log_level_) return;
  WriteImpl(first_message);
  Write(lvl, message...);
}
template<typename T, typename... Args>
void Log::P(T first_message, Args... message) {
  Write(0, first_message, message...);
}
} // namespace triangulator
