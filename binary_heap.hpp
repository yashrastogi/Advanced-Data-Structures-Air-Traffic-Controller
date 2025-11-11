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

  void bubbleUp(int i) {
    while (i != 0) {
      if (comp_(data_[i], data_[parent(i)])) {
        swap(&data_[i], &data_[parent(i)]);
        i = parent(i);
      } else {
        break;
      }
    }
  }

  void swap(T *a, T *b) {
    T temp = *a;
    *a = *b;
    *b = temp;
  }

  void bubbleDown(int i) {
    int most = i;

    if (leftChild(i) < size() && comp_(data_[leftChild(i)], data_[most]))
      most = leftChild(i);

    if (rightChild(i) < size() && comp_(data_[rightChild(i)], data_[most]))
      most = rightChild(i);

    if (most != i) {
      swap(&data_[i], &data_[most]);
      bubbleDown(most);
    }
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

  bool eraseOne(T value) {
    for (size_t i = 0; i < size(); i++) {
      if (data_[i] == value) {
        swap(&data_[i], &data_[size() - 1]);
        data_.pop_back();
        bubbleDown(i);
      }
    }
    return false;
  }

  T pop() {
    if (size() == 0)
      throw std::out_of_range("Heap is empty");
    T value = data_[0];
    data_[0] = data_.back();
    data_.pop_back();
    bubbleDown(0);
    // for (size_t i = 0; i < size_; i++)
    //   std::cout << data_[i] << " ";
    // std::cout << std::endl;
    return value;
  }

  bool changeKey(T value, T newValue) {
    for (size_t i = 0; i < size(); i++) {
      if (data_[i] == value) {
        data_[i] = newValue;
        swap(&data_[i], &data_[size() - 1]);
        bubbleUp(size() - 1);
        return true;
      }
    }
    return false;
  }

  T top() {
    if (size() == 0)
      throw std::out_of_range("Heap is empty");
    return data_[0];
  }

  void push(const T &value) {
    data_.emplace_back(value);
    bubbleUp(data_.size() - 1);
    // for (size_t i = 0; i < size(); i++)
    //   std::cout << data_[i] << " ";
    // std::cout << std::endl;
  }

  size_type size() const { return data_.size(); }

  bool empty() const { return size() == 0; }
};
