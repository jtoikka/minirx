# minirx

Minirx is a small library for reactive programming with C++14.

## Usage

Drop `rx.h` and the `rx` folder into your project.

```cpp

#include "rx.h"

VarT<int> foo = Var(1);

VarT<float> bar = Var(2.0f);

Rx<float> baz = bar.map([] (int value) {
  return value * 2.5f;
});

Rx<float> fooBaz = reactives(foo, baz).reduce([] (int foo, float baz) {
  return foo * baz;
});

std::cout << fooBaz.now() << std::endl; // 5.0f

foo.set(2);

std::cout << fooBaz.now() << std::endl; // 10.0f

bar.set(4.0f);

std::cout << fooBaz.now() << std::endl; // 20.0f

```

See `test/tests.cpp` for more examples.

## Run tests

```sh
$ ./run_tests.sh
```

Requires [meson](https://github.com/mesonbuild/meson) build tool.
