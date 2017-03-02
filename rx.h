#pragma once

#include <functional>
#include <memory>
#include <vector>

namespace rx {

template <typename T>
class Observable {
public:
  virtual T now() const = 0;

  virtual ~Observable() { }
};

class Signallable {
public:
  virtual ~Signallable() { }
  virtual void signal(uint8_t signalId) = 0;
private:

};


template <typename T>
class Outputting {
public:

  void addOutput(std::weak_ptr<Signallable> r) {
    _outputs.push_back(r);
  }

  void addStickyOutput(std::shared_ptr<Signallable> r) {
    _stickyOutputs.push_back(r);
  }

  virtual ~Outputting() { }

  virtual T now() const = 0;

#ifdef DEBUG
  void clean() {
    _clean();
  }

  size_t numOutputs() {
    return _outputs.size();
  }
#endif


protected:

  void forwardSignal(uint8_t signalId) const {
    auto cleanup = false;
    for (auto observer : _outputs) {
      if (auto tmp = observer.lock()) {
        tmp->signal(signalId);
      } else {
        cleanup = true;
      }
    }
    for (auto observer : _stickyOutputs) {
      observer->signal(signalId);
    }
    if (cleanup) {
      _clean();
    }
  }

private:
  void _clean() const {
    _outputs.erase(
      std::remove_if(
        begin(_outputs),
        end(_outputs),
        [](auto ptr) { return ptr.expired(); }),
      end(_outputs)
    );
  }

  mutable std::vector<std::weak_ptr<Signallable>> _outputs;
  mutable std::vector<std::shared_ptr<Signallable>> _stickyOutputs;
};

template <typename R, typename... Types>
class Routable :
  public Signallable,
  public Outputting<R> {
public:
  Routable(std::shared_ptr<Outputting<Types>>... inputs) : _inputs(std::tie(inputs...)) { }

  virtual ~Routable() { }

  virtual void receivedSignal() = 0;

  void signal(uint8_t signalId) {
    if (signalId != lastReceivedSignalId) {
      lastReceivedSignalId = signalId;
      receivedSignal();
      this->forwardSignal(signalId);
    }
  }

protected:
  uint8_t lastReceivedSignalId = 0;
  std::tuple<std::shared_ptr<Outputting<Types>>...> _inputs;
};

template <class T>
class Observer;

template <typename T>
class Reactive {
public:
  Reactive() { }
  Reactive(std::shared_ptr<Outputting<T>> node) : _node(node) {}

  std::shared_ptr<Outputting<T>> node() const {
    return _node;
  }

  template <typename F>
  auto map(F func) {
    return reactives(*this).reduce(func);
  }

  template <typename F>
  void observe(F func) {
    Observer<T>(func, *this);
  }

  T now() const {
    return this->_node->now();
  }

  virtual ~Reactive() { }

protected:
  std::shared_ptr<Outputting<T>> _node;
};

template <class T>
class ObserverNode : public Signallable {
public:
  ObserverNode(
    std::function<void(T)>&& func,
    std::shared_ptr<Outputting<T>> input) : evaluate(func), input(input) { }

  void signal(uint8_t signalId) override {
    if (auto tmp = input.lock()) {
      evaluate(tmp->now());
    }
  }
private:
  std::function<void(T)> evaluate;
  std::weak_ptr<Outputting<T>> input;
};

template <class T>
class Observer {
public:
  Observer() { }

  Observer(std::function<void(T)>&& func, Reactive<T> input) :
      _node(std::make_shared<ObserverNode<T>>(std::move(func), input.node())) {

    observe(input);
  }
private:
  void observe(Reactive<T> reactive) {
    reactive.node()->addStickyOutput(_node);
  }

  std::shared_ptr<ObserverNode<T>> _node;
};

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

class Signal {
public:
  static Signal& instance() {
    static Signal _instance;
    return _instance;
  }

  Signal(Signal const&) = delete;
  void operator=(Signal const&) = delete;

  uint8_t nextSignalId() {
    if (signalId == 0) {
      signalId = 1;
    }
    return signalId++;
  }
private:
  Signal() { }
  uint8_t signalId = 1;
};

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
      this->forwardSignal(Signal::instance().nextSignalId());
    }
  }
private:
  T _value;
};

template <typename T>
class VarT : public Reactive<T> {
public:
  VarT(T value) : Reactive<T>(std::make_shared<VarNode<T>>(value)) { }

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
