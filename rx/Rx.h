#pragma once

#include <functional>
#include <memory>

#include "rx/ReactiveTraits.h"

namespace rx {

template<int ...>
struct seq {};

template<int N, int ...S>
struct gen_seq : gen_seq<N-1, N-1, S...> {};

template<int ...S>
struct gen_seq<0, S...>{
  typedef seq<S...> type;
};

class RxException : public std::runtime_error {
public:
  explicit RxException(const char* what_arg) :
    std::runtime_error(what_arg) {}
};

#ifdef DEBUG
  int RX_EVALUATE_COUNT = 0;
#endif

template <typename R, typename... Types>
class RxNode : public Routable<R, Types...> {
public:
  RxNode(std::shared_ptr<Outputting<Types>>... inputs, std::function<R(Types...)> func) :
      Routable<R, Types...>(inputs...),
      _func(func) {
  }

  void receivedSignal() {
    upToDate = false;
  }

  R now() const {
    if (!this->upToDate) {
      #ifdef DEBUG
        RX_EVALUATE_COUNT += 1;
      #endif
      cachedValue = evaluate();
      upToDate = true;
    }
    return cachedValue;
  }

private:
  R evaluate() const {
    return callFunc(typename gen_seq<sizeof...(Types)>::type());
  }

  template<int ...S>
  R callFunc(seq<S...>) const {

    return _func(std::get<S>(this->_inputs)->now() ...);
  }

  mutable bool upToDate = false;
  std::function<R(Types...)> _func;
  mutable R cachedValue;
};

template <typename ReturnType>
class Rx : public Reactive<ReturnType> {
public:
  Rx() { }

  template <typename... Types>
  void create(std::shared_ptr<RxNode<ReturnType, Types...>> node) {
    this->_node = node;
    isCreated = true;
  }

private:
  bool isCreated = false;
};

template <typename... Types>
class ReactiveTuple {
public:
  ReactiveTuple(Reactive<Types>... inputs) : _inputs(std::tie(inputs...)) { }

  template <typename F>
  auto reduce(F func) {
    return reduce(func, typename gen_seq<sizeof...(Types)>::type());
  }

  template <typename F, int ...S>
  auto reduce(F func, seq<S...>) {
    using R = decltype(func(std::get<S>(_inputs).node()->now() ...));
    auto p = std::make_shared<RxNode<R, Types...>>(std::get<S>(_inputs).node() ..., func);

    auto r = Rx<R>();
    r.create(p);
    node = p;
    setOutputs(std::get<S>(_inputs)...);
    return r;
  }

private:
  std::tuple<Reactive<Types>...> _inputs;
  std::shared_ptr<Signallable> node;

  template <typename T>
  void setOutputs(const T& t) {
    t.node()->addOutput(node);
  }

  template <typename First, typename... Rest>
  void setOutputs(const First& first, const Rest&... rest) {
    first.node()->addOutput(node);
    this->setOutputs(rest...);
  }
};

template <typename... Types>
auto reactives(Reactive<Types>... inputs) {
  return ReactiveTuple<Types...>(inputs...);
}

}
