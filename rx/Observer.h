// #pragma once
//
// #include <memory>
// #include <functional>
//
// #include "rx/ReactiveTraits.h"
//
// namespace rx {
//
// template <class T>
// class ObserverNode : public Signallable {
// public:
//   ObserverNode(
//     std::function<void(T)>&& func,
//     std::shared_ptr<Outputting<T>> input) : evaluate(func), input(input) { }
//
//   void signal(uint8_t signalId) override {
//     evaluate(input->now());
//   }
// private:
//   std::function<void(T)> evaluate;
//   std::shared_ptr<Outputting<T>> input;
// };
//
// template <class T>
// class Observer {
// public:
//   Observer() { }
//
//   Observer(std::function<void(T)>&& func, Reactive<T> input) :
//       _node(std::make_shared<ObserverNode<T>>(std::move(func), input.node())) {
//
//     observe(input);
//   }
// private:
//   void observe(Reactive<T> reactive) {
//     reactive.node()->addStickyOutput(_node);
//   }
//
//   std::shared_ptr<ObserverNode<T>> _node;
// };
//
// }
