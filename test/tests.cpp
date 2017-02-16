#define CATCH_CONFIG_MAIN
#include "test/catch.hpp"

#define DEBUG
#include "rx.h"

using namespace rx;

TEST_CASE( "Vars can be set", "[Var]" ) {
  VarT<int> a = Var(0);
  REQUIRE( a.now() == 0 );

  a.set(5);
  REQUIRE( a.now() == 5 );

  auto b = Var("before");
  REQUIRE( b.now() == "before" );

  b.set("after");
  REQUIRE( b.now() == "after" );
}

TEST_CASE( "Var can be mapped to reactive variable", "[Var]" ) {
  VarT<int> input = Var(10);

  Rx<float> mapped = input.map([] (int value) {
    return value / 2.0f;
  });

  REQUIRE( mapped.now() == 5.0f);

  input.set(20);

  REQUIRE( mapped.now() == 10.0f);
}

TEST_CASE( "Observers out of scope get cleaned up", "[Var]") {
  VarT<int> a = Var(0);

  {
    Rx<int> r = a.map([] (int value) {
      return value * 2;
    });

    REQUIRE(a.numObservers() == 1);
  }

  REQUIRE(a.numObservers() == 0);
}

TEST_CASE( "Rx values react to Var changes", "[Rx]" ) {
  VarT<int> input = Var(1);

  Rx<int> r1 = input.map([] (int value) {
    return value * 10;
  });

  Rx<int> r2 = input.map([] (int value) {
    return value * 100;
  });

  REQUIRE( r1.now() == 10 );
  REQUIRE( r2.now() == 100 );

  input.set(2);

  REQUIRE( r1.now() == 20 );
  REQUIRE( r2.now() == 200 );

  input.set(1);

  REQUIRE( r1.now() == 10 );
  REQUIRE( r2.now() == 100 );
}

TEST_CASE( "Rx values react to multiple Var changes", "[Rx]") {
  VarT<int> input1 = Var(1);
  VarT<int> input2 = Var(100);

  Rx<int> r1 = reactives(input1, input2).reduce([] (int in1, int in2) {
    return in1 * in2;
  });

  REQUIRE( r1.now() == 100 );

  input1.set(2);

  REQUIRE( r1.now() == 200 );

  input2.set(1000);

  REQUIRE( r1.now() == 2000 );

  input1.set(4);
  input2.set(10000);

  REQUIRE( r1.now() == 40000 );
}

TEST_CASE( "Var changes propogate through multiple Rx levels", "[Rx]") {
  VarT<int> input = Var(1);

  Rx<int> r1 = input.map([] (int in) {
    return in * 2;
  });

  REQUIRE( r1.now() == 2 );

  Rx<int> r2 = r1.map([] (int in) {
    return in * 3;
  });

  REQUIRE( r2.now() == 6 );

  input.set(10);

  REQUIRE( r2.now() == 60 );
  REQUIRE( r1.now() == 20 );
}

TEST_CASE( "Changes are only calculated upon reading value", "[Rx]") {
  VarT<int> input = Var(1);

  const int evaluateCount = RX_EVALUATE_COUNT;

  Rx<int> r = input.map([] (int in) {
    return in * 2;
  });

  input.set(2);
  input.set(3);

  REQUIRE( RX_EVALUATE_COUNT == evaluateCount );

  r.now();

  REQUIRE( RX_EVALUATE_COUNT == evaluateCount + 1 );

}

TEST_CASE( "Vars can be observed", "[Var]" ) {
  VarT<int> input = Var(0);

  int signalCount = 0;

  input.observe([&] (int observedValue) {
    signalCount++;
  });

  REQUIRE( signalCount == 0 );

  input.set(10);

  REQUIRE( signalCount == 1 );

  input.set(1);
  input.set(10);

  REQUIRE( signalCount == 3 );
}

TEST_CASE( "Rxs can be observed", "[Rx]" ) {
  VarT<int> input = Var(0);

  Rx<int> r = input.map([] (int in) {
    return in * 10;
  });

  int signalCount = 0;

  r.observe([&] (int observedValue) {
    signalCount++;
  });

  REQUIRE( signalCount == 0 );

  input.set(10);

  REQUIRE( signalCount == 1 );

  input.set(1);
  input.set(10);

  REQUIRE( signalCount == 3 );
}

TEST_CASE( "Send signal only if variable changes", "[Var]" ) {
  VarT<int> input = Var(0);

  int signalCount = 0;

  input.observe([&] (int observedValue) {
    signalCount++;
  });

  REQUIRE( signalCount == 0 );

  input.set(0);

  REQUIRE( signalCount == 0 );
}

TEST_CASE( "Stop observing when reactive no longer in scope", "[Observer]" ) {
  int counter = 0;
  VarT<int> input = Var(0);
  {
    Rx<int> r = input.map([] (int in) {
      return in * 2;
    });

    r.observe([&] (int observedValue) {
      counter++;
    });

    input.set(1);
  }

  REQUIRE( counter == 1 );

  input.set(2);

  REQUIRE( counter == 1 );
}

TEST_CASE( "A signal should pass through a Reactive only once", "[Rx]" ) {
  VarT<float> time = 0.0f;

  auto x = time.map([] (float time) {
    return time * 10.0f;
  });

  auto y = time.map([] (float time) {
    return time * 1.0f;
  });

  auto xy = reactives(x, y).reduce([] (float x, float y) {
    return x * y;
  });

  int counter = 0;

  xy.observe([&](float input) {
    counter++;
  });

  REQUIRE( counter == 0 );

  time.set(1.0f);

  REQUIRE( counter == 1 );
}
