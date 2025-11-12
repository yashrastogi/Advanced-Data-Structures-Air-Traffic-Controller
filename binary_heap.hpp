#pragma once
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <vector>

// Binary heap implementation with customizable comparison
template <typename T, typename Compare = std::greater<T>> class BinaryHeap {
private:
  std::vector<T> data_{};
  Compare comp_{};

  int parent(int index) const {
    if (index == 0)
      throw std::out_of_range("Root has no parent");
    return (index - 1) / 2;
  }

  // Returns the left child index
  size_t leftChild(int index) const { return 2 * index + 1; }

  // Returns the right child index
  size_t rightChild(int index) const { return 2 * index + 2; }

  // Moves element up the heap to maintain heap property
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

  // Swaps two elements
  void swap(T *a, T *b) { std::swap(*a, *b); }

  // Moves element down the heap to maintain heap property
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

  // Removes the first occurrence of a value from the heap
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

  // Removes and returns the top element
  T pop() {
    if (size() == 0)
      throw std::out_of_range("BinaryHeap is empty");
    T value = data_[0];
    data_[0] = data_.back();
    data_.pop_back();
    bubbleDown(0);
    return value;
  }

  // Changes the key of an element and repositions it
  bool changeKey(T value, T newValue) {
    for (size_t i = 0; i < size(); i++) {
      if (data_[i] == value) {
        data_[i] = newValue;
        // Bubble up if priority increased, bubble down if decreased
        if (comp_(newValue, value)) {
          bubbleUp(i);
        } else if (comp_(value, newValue)) {
          bubbleDown(i);
        }
        return true;
      }
    }
    return false;
  }

  // Returns the top element without removing it
  T top() {
    if (size() == 0)
      throw std::out_of_range("BinaryHeap is empty (no top)");
    return data_[0];
  }

  // Inserts a new element into the heap
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
