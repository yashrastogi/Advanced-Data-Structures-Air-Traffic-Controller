#pragma once
#include <functional>
#include <stdexcept>
#include <vector>

template <typename T> struct PairingHeapNode {
  T value;
  PairingHeapNode<T> *child{nullptr};
  PairingHeapNode<T> *leftSibling{nullptr};
  PairingHeapNode<T> *rightSibling{nullptr};
};

template <typename T, typename Compare = std::greater<T>> class PairingHeap {
public:
  Compare comp_{};
  std::size_t totalNodes{0};
  PairingHeapNode<T> *root_{nullptr};

  void detachNode(PairingHeapNode<T> *theNode) {
    if (!theNode || theNode == root_)
      return;

    if (theNode->leftSibling->child == theNode) {
      PairingHeapNode<T> *parent = theNode->leftSibling;
      if (theNode->rightSibling) {
        theNode->rightSibling->leftSibling = parent;
      }
      parent->child = theNode->rightSibling;
    }

    else {
      theNode->leftSibling->rightSibling = theNode->rightSibling;
      if (theNode->rightSibling) {
        theNode->rightSibling->leftSibling = theNode->leftSibling;
      }
    }

    theNode->leftSibling = nullptr;
    theNode->rightSibling = nullptr;
  }

  PairingHeapNode<T> *meld(PairingHeapNode<T> *a, PairingHeapNode<T> *b) {
    if (!a)
      return b;
    if (!b)
      return a;
    a->leftSibling = nullptr;
    a->rightSibling = nullptr;
    b->leftSibling = nullptr;
    b->rightSibling = nullptr;
    if (comp_(a->value, b->value)) {
      PairingHeapNode<T> *currentChild = a->child;
      a->child = b;
      b->leftSibling = a;
      b->rightSibling = currentChild;
      if (currentChild)
        currentChild->leftSibling = b;
      return a;
    } else {
      PairingHeapNode<T> *currentChild = b->child;
      b->child = a;
      a->leftSibling = b;
      a->rightSibling = currentChild;
      if (currentChild)
        currentChild->leftSibling = a;
      return b;
    }
  }

  using value_type = T;
  using size_type = std::size_t;

  PairingHeap() = default;

  explicit PairingHeap(Compare comp) : comp_(comp) {}

  template <class It>
  PairingHeap(It first, It last, Compare comp = Compare{}) : comp_(comp) {
    for (auto it = first; it != last; ++it) {
      push(*it);
    }
  }

  PairingHeapNode<T> *push(const T &value) {
    PairingHeapNode<T> *newNode =
        new PairingHeapNode<T>{value, nullptr, nullptr, nullptr};
    root_ = meld(root_, newNode);
    totalNodes++;
    return newNode;
  }

  // Only allowed to increase or keep same the priority
  PairingHeapNode<T> * changeKey(PairingHeapNode<T> *theNode, T newValue) {
    if (!theNode)
      throw std::runtime_error("Node to change doesn't exist");

    T value = theNode->value;

    // For max heap: comp_(a, b) means a > b (a has higher priority)
    // If old value has higher priority than new, that's a decrease,
    // delete and reinsert element instead.
    if (comp_(value, newValue)) {
      eraseOne(theNode);
      return push(newValue);
      // throw std::runtime_error("Only increasing priority allowed");
    }

    theNode->value = newValue;

    // If priority increased (newValue > value), we need to restructure
    // If priority stayed same (newValue == value), no restructuring needed
    if (!comp_(newValue, value)) {
      // Priority didn't increase, no restructuring needed
      return theNode;
    }

    // Priority increased - if it's the root, it's already at the top
    if (theNode == root_)
      return theNode;

    // Detach and meld to maintain heap property
    detachNode(theNode);
    root_ = meld(root_, theNode);
    return theNode;
  }

  // Only allowed to increase or keep same the priority
  void changeKey(T value, T newValue) { changeKey(findNode(value), newValue); }

  bool eraseOne(T value) { return eraseOne(findNode(value)); }

  bool eraseOne(PairingHeapNode<T> *theNode) {
    if (!theNode || !root_)
      return false;
    if (theNode == root_) {
      pop();
      return true;
    }
    PairingHeapNode<T> *childPointer = theNode->child;
    detachNode(theNode);
    while (childPointer) {
      PairingHeapNode<T> *next = childPointer->rightSibling;
      childPointer->leftSibling = nullptr;
      childPointer->rightSibling = nullptr;
      root_ = meld(root_, childPointer);
      childPointer = next;
    }
    totalNodes--;
    delete theNode;
    return true;
  }

  const T &top() const {
    if (!root_)
      throw std::runtime_error("PairingHeap is empty (no top)");
    return root_->value;
  }

  T pop() {
    if (!root_)
      throw std::runtime_error("PairingHeap is empty");

    T value = root_->value;
    PairingHeapNode<T> *childPointer = root_->child;
    std::vector<PairingHeapNode<T> *> meldStack;
    while (childPointer && childPointer->rightSibling) {
      PairingHeapNode<T> *first = childPointer;
      PairingHeapNode<T> *second = childPointer->rightSibling;
      childPointer = second->rightSibling;
      meldStack.push_back(meld(first, second));
    }

    if (childPointer) {
      if (!meldStack.empty()) {
        PairingHeapNode<T> *temp = meldStack.back();
        meldStack.pop_back();
        meldStack.push_back(meld(temp, childPointer));
      } else
        meldStack.push_back(childPointer);
    }

    PairingHeapNode<T> *second = nullptr;
    while (!meldStack.empty()) {
      PairingHeapNode<T> *first = meldStack.back();
      meldStack.pop_back();
      second = meld(first, second);
    }
    delete root_;
    root_ = second;
    if (root_) {
      root_->rightSibling = root_->leftSibling = nullptr;
    }
    totalNodes--;
    return value;
  }

  bool empty() const noexcept { return root_ == nullptr; }

  size_type size() const noexcept { return totalNodes; }

  ~PairingHeap() { clear(); }

  void clear() noexcept {
    if (!root_)
      return;
    std::vector<PairingHeapNode<T> *> stk{root_};
    while (!stk.empty()) {
      PairingHeapNode<T> *n = stk.back();
      stk.pop_back();
      for (PairingHeapNode<T> *c = n->child; c; c = c->rightSibling)
        stk.push_back(c);
      delete n;
    }
    root_ = nullptr;
    totalNodes = 0;
  }

  PairingHeapNode<T> *findNode(const T &value) {
    if (!root_)
      return nullptr;
    std::vector<PairingHeapNode<T> *> stk{root_};
    while (!stk.empty()) {
      PairingHeapNode<T> *n = stk.back();
      stk.pop_back();
      if (n->value == value)
        return n;
      for (PairingHeapNode<T> *c = n->child; c; c = c->rightSibling)
        stk.push_back(c);
    }
    return nullptr;
  }

  PairingHeap(const PairingHeap &) = delete;
  PairingHeap &operator=(const PairingHeap &) = delete;
  PairingHeap(PairingHeap &&) = delete;
  PairingHeap &operator=(PairingHeap &&) = delete;
};
