#pragma once

#include "rx/ReactiveTraits.h"
#include "rx/Rx.h"

namespace rx {

class Signal {
public:
  static uint8_t nextSignalId() {
    return signalId++;
  }
private:
  static uint8_t signalId;
};

uint8_t Signal::signalId = 1;

template <typename T>
class VarNode : public Outputting<T> {
public:
  VarNode(T value) : _value(value) { }

  T now() const {
    return _value;
  }

  void set(T value) {
    if (!(this->_value == value)) {
      this->_value = value;
      this->forwardSignal(Signal::nextSignalId());
    }
  }
private:
  T _value;
};

template <typename T>
class VarT : public Reactive<T> {
public:
  VarT(T value) : Reactive<T>(std::make_shared<VarNode<T>>(value)) { }

  T now() const {
    return this->_node->now();
  }

  void set(T value) {
    std::dynamic_pointer_cast<VarNode<T>>(this->_node)->set(value);
  }

#ifdef DEBUG
  size_t numObservers() {
    this->_node->clean();
    return this->_node->numOutputs();
  }
#endif
};

template <typename T>
VarT<T> Var(T value) {
  return VarT<T>(value);
};

}
