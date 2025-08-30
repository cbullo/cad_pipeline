#pragma once

#include <print>
#include <stack>
#include <unordered_map>
#include <variant>

#include "types.h"
#include "visit_helper.h"

using RuntimeType = std::variant<float, GeomId>;
using RuntimeStack = std::stack<RuntimeType>;

template <char Key, typename... Param>
struct Op {
  static const char KeyValue = Key;

  static void ConsumeParams(RuntimeStack& runtime_stack) {
    using ParamsTuple = std::tuple<Param...>;
    using ParamsSequence = std::index_sequence_for<Param...>;

    ParamsTuple params;

    auto indexed_lambda = [&runtime_stack, &params]<size_t... I> {
      (
          [&runtime_stack, &params]() {
            RuntimeType var = runtime_stack.top();
            runtime_stack.pop();
            std::visit(
                overloaded{[&runtime_stack, &params](
                               const std::tuple_element_t<I, ParamsTuple>& op) {
                             std::get<I>(params) = op;
                           },
                           [](const auto& n) {
                             std::println("Incorrect runtime type!");
                           }},
                var);
          }(),
          ...);
    };

    indexed_lambda.template operator()<ParamsSequence>();
  }
};

using CubeOp = Op<'C', Scalar, Scalar, Scalar>;

class Executor {
 public:
  using RuntimeType = ::RuntimeType;
  void Invoke(const char key, RuntimeStack& stack) {}

  template <class Op>
  void Register() {
    auto key = Op::KeyValue;
    const auto& consume = Op::Consume;

    _ops.insert(key, {consume});
  }

 private:
  struct InternalOp {
    std::function<void(RuntimeStack&)> consume;
  };

  std::unordered_map<char, InternalOp> _ops;
};