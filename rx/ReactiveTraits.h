#pragma once

#include <memory>
#include <vector>
#include <functional>

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

}
