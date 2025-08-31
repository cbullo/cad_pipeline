#pragma once

#include <format>
#include <print>
#include <stack>
#include <unordered_map>
#include <variant>

#include "types.h"
#include "visit_helper.h"

using RuntimeType = std::variant<float, GeomId>;
using RuntimeStack = std::stack<RuntimeType>;

std::string NormalizeKey(const GeomId& g) { return g; }

std::string NormalizeKey(float v) { return std::format("N{:.8f}", v); }

template <char Key, typename F, typename... Param>
struct Op {
  static const char keyValue = Key;
  using ParamsTuple = std::tuple<Param...>;
  using Process = F;

  static std::string ConsumeParams(RuntimeStack& runtime_stack) {
    using ParamsSequence = std::index_sequence_for<Param...>;

    ParamsTuple params;

    auto indexed_lambda = [&runtime_stack,
                           &params]<size_t... I>(std::index_sequence<I...>) {
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
                             std::println("Incorrect runtime type: {} {}", n, I);
                           }},
                var);
          }(),
          ...);
    };

    indexed_lambda.template operator()<>(ParamsSequence{});

    std::string out;
    std::apply(
        [&out](Param const&... tupleArgs) {
          ((out.append(NormalizeKey(tupleArgs))), ...);
        },
        params);
    return out;
  }

  static void GetNormalizedOp(Param... params) {}
};

class Executor {
 public:
  using RuntimeType = ::RuntimeType;

  void Invoke(const char mnemonic, RuntimeStack& runtime_stack) {
    auto& op = GetOp(mnemonic);
    
    GeomId key = std::format("{}{}", op.consume_params(runtime_stack), mnemonic);
    _request_stack.push(Request{key, params, &op::Process});
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
    std::function<std::string(RuntimeStack&)> consume_params;
  };

  InternalOp& GetOp(const char key) { return _ops[key]; }

  std::unordered_map<char, InternalOp> _ops;
  std::stack<Request> _request_stack; 
};