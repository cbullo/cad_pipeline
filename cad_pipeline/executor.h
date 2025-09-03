#pragma once

#include <format>
#include <print>
#include <stack>
#include <unordered_map>
#include <variant>

#include "cache.h"
#include "types.h"
#include "visit_helper.h"

using RuntimeType = std::variant<float, char, GeomId>;
using RuntimeStack = std::stack<RuntimeType>;

std::string NormalizeKey(const GeomId& g) { return g; }
std::string NormalizeKey(float v) { return std::format("N{:.8f}", v); }
std::string NormalizeKey(char v) { return std::format("S{}", v); }

// Your target type
struct MyType {
  std::string value;
};

// Customization functor: special-case GeomId, identity for others
struct Convert {
  // TODO: doesn't work with a reference, why?
  AnyGeometry operator()(/*const*/ GeomId /*&*/ id, Cache& cache) const {
    return cache.at(id);
  }

  template <class T>
  constexpr T operator()(T&& x, Cache&) const noexcept {
    return std::forward<T>(x);  // identity
  }
};

// Generic tuple transform (C++17)
template <class F, class Tuple>
auto tuple_transform(Tuple&& tup, F&& f, Cache& cache) {
  return std::apply(
      [&](auto&&... xs) {
        return std::tuple<std::decay_t<decltype(f(
            std::forward<decltype(xs)>(xs), cache))>...>{
            f(std::forward<decltype(xs)>(xs), cache)...};
      },
      std::forward<Tuple>(tup));
}

template <char Key, bool Cachable, auto F, typename... Param>
struct Op {
  static const char keyValue = Key;
  using ParamsTuple = std::tuple<Param...>;

  static std::string ConsumeParams(RuntimeStack& runtime_stack, Cache& cache) {
    using ParamsSequence = std::index_sequence_for<Param...>;

    ParamsTuple params;

    auto indexed_lambda = [&runtime_stack,
                           &params]<size_t... I>(std::index_sequence<I...>) {
      (
          [&runtime_stack, &params]() {
            if (runtime_stack.empty()) {
              std::println("Runtime stack is empty! Invalid program.");
              return;
            }
            RuntimeType var = runtime_stack.top();
            runtime_stack.pop();
            std::visit(
                overloaded{
                    [&runtime_stack, &params](
                        const std::tuple_element_t<I, ParamsTuple>& param) {
                      std::get<I>(params) = param;
                    },
                    [](const auto& n) {
                      std::println(
                          "Incorrect runtime type: {} {}", typeid(n).name(),
                          typeid(std::tuple_element_t<I, ParamsTuple>).name());
                      std::flush(std::cout);
                    }},
                var);
          }(),
          ...);
    };

    indexed_lambda.template operator()<>(ParamsSequence{});

    std::string params_key;

    std::apply(
        [&params_key](Param const&... tupleArgs) {
          ((params_key.append(NormalizeKey(tupleArgs))), ...);
        },
        params);

    GeomId cache_key = std::format("{}{}", params_key, Key);

    // []<typename ...T>(std::tuple<T...>&& p) {
    //   F(p...);
    // }(params);

    Convert c{};
    auto callable_params = tuple_transform(params, c, cache);

    // TODO: Don't call operations here - they should be processed by Planner
    // and Scheduler.
    auto geometry = std::apply(F, callable_params);
    cache[cache_key] = geometry;

    return cache_key;
  }

  static void GetNormalizedOp(Param... params) {}
};

class Executor {
 public:
  using RuntimeType = ::RuntimeType;

  void Invoke(const char mnemonic, RuntimeStack& runtime_stack, Cache& cache) {
    auto& op = GetOp(mnemonic);
    auto key = op.consume_params(runtime_stack, cache);
    _request_stack.push(key);
    runtime_stack.push(key);
  }

  template <class Op>
  void Register() {
    auto key = Op::keyValue;
    auto consume_params = Op::ConsumeParams;

    _ops.insert({key, {consume_params}});
  }

 private:
  struct InternalOp {
    std::function<std::string(RuntimeStack&, Cache&)> consume_params;
  };

  InternalOp& GetOp(const char key) { return _ops.at(key); }

  std::unordered_map<char, InternalOp> _ops;
  RuntimeStack _request_stack;
};