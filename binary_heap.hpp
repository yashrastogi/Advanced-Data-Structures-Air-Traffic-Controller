#pragma once
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <vector>

template <typename T, typename Compare = std::greater<T>> class BinaryHeap {
private:
  std::vector<T> data_{};
  Compare comp_{};

  int parent(int index) const {
    if (index == 0)
      throw std::out_of_range("Root has no parent");
    return (index - 1) / 2;
  }

  size_t leftChild(int index) const { return 2 * index + 1; }

  size_t rightChild(int index) const { return 2 * index + 2; }

  size_t bubbleUp(size_t i) {
    while (i != 0) {
      if (comp_(data_[i], data_[parent(i)])) {
        swap(&data_[i], &data_[parent(i)]);
        i = parent(i);
      } else {
        break;
      }
    }
    return i;
  }

  void swap(T *a, T *b) { std::swap(*a, *b); }

  size_t bubbleDown(size_t i) {
    size_t most = i;

    if (leftChild(i) < size() && comp_(data_[leftChild(i)], data_[most]))
      most = leftChild(i);

    if (rightChild(i) < size() && comp_(data_[rightChild(i)], data_[most]))
      most = rightChild(i);

    if (most != i) {
      swap(&data_[i], &data_[most]);
      bubbleDown(most);
    }
    return most;
  }

public:
  using value_type = T;
  using size_type = std::size_t;

  BinaryHeap() = default;

  explicit BinaryHeap(Compare comp) : comp_(comp) {}

  template <class It>
  BinaryHeap(It first, It last, Compare comp = Compare{}) : comp_(comp) {
    for (auto it = first; it != last; ++it) {
      push(*it);
    }
  }

  bool eraseOne(T &value) {
    size_t arrIndex = 0;
    bool found = false;
    for (size_t i = 0; i < size(); i++) {
      if (data_[i] == value) {
        arrIndex = i;
        found = true;
        break;
      }
    }
    if (!found)
      return false;
    swap(&data_[arrIndex], &data_[size() - 1]);
    data_.pop_back();
    bubbleDown(arrIndex);
    return true;
  }

  T pop() {
    if (size() == 0)
      throw std::out_of_range("BinaryHeap is empty");
    T value = data_[0];
    data_[0] = data_.back();
    data_.pop_back();
    bubbleDown(0);
    return value;
  }

  bool changeKey(T value, T newValue) {
    for (size_t i = 0; i < size(); i++) {
      if (data_[i] == value) {
        data_[i] = newValue;
        // Determine direction: comp_(a, b) means a > b (higher priority)
        // If newValue > oldValue, priority increased → bubble up
        // If oldValue > newValue, priority decreased → bubble down
        if (comp_(newValue, value)) {
          bubbleUp(i);
        } else if (comp_(value, newValue)) {
          bubbleDown(i);
        }
        // If equal, no movement needed
        return true;
      }
    }
    return false;
  }

  T top() {
    if (size() == 0)
      throw std::out_of_range("BinaryHeap is empty (no top)");
    return data_[0];
  }

  void push(const T &value) {
    data_.emplace_back(value);
    bubbleUp(data_.size() - 1);
  }

  ~BinaryHeap() { clear(); }

  void clear() noexcept { data_.clear(); }

  std::vector<T> data() const { return data_; }

  size_type size() const { return data_.size(); }

  bool empty() const { return size() == 0; }
};
