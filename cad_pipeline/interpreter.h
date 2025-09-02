#pragma once

#include <charconv>
#include <iostream>
#include <stack>
#include <string>
#include <variant>
#include <vector>

#include "visit_helper.h"

template <char Mnemonic>
struct Token {
  const static char MnemonicValue = Mnemonic;
  static Token Consume(std::string_view& input) { return Token(); };
};

template <>
struct Token<'N'> {
  const static char MnemonicValue = 'N';
  float value;

  static Token Consume(std::string_view& input) {
    // Variable to store the parsed float
    float f;

    // Use std::from_chars to parse the float from the string_view
    auto result = std::from_chars(input.data(), input.data() + input.size(), f);

    if (result.ec == std::errc()) {
      // Update the string_view to start after the parsed float
      input.remove_prefix(result.ptr - input.data());
    } else {
      // TODO: error handling
    }
    return Token{value : f};
  }
};

template <>
struct Token<'S'> {
  const static char MnemonicValue = 'S';
  char value;

  static Token Consume(std::string_view& input) {
    // Variable to store the parsed float
    char c = input.front();
    input.remove_prefix(1);
    return Token{value : c};
  }
};

using ConstNumberToken = Token<'N'>;
using CubeToken = Token<'C'>;
using WriteToken = Token<'W'>;
using TriangulateToken = Token<'T'>;
using MakeCharacterToken = Token<'L'>;
//TODO: Not really a string, only a single character is supported
using StringToken = Token<'S'>;
using ExtrudeToken = Token<'E'>;

using TokenVariant = std::variant<ConstNumberToken, CubeToken, WriteToken,
                                  TriangulateToken, MakeCharacterToken, StringToken, ExtrudeToken>;

std::vector<TokenVariant> Parse(const std::string& input) {
  std::string_view input_view(input);
  std::vector<TokenVariant> tokens;

  [&input_view]<typename... T>(std::vector<std::variant<T...>>& tokens) {
    auto invoke = [&tokens, &input_view](std::string_view& input) {
      const char key = input.front();
      input.remove_prefix(1);

      bool found = false;
      (
          [key, &input_view, &tokens, &found]() {
            if (key == T::MnemonicValue) {
              std::println("Consuming: {}", key);
              const auto& token = T::Consume(input_view);
              tokens.push_back(TokenVariant(token));
              found = true;
            }
          }(),
          ...);

      if (!found) {
        std::println("Unknown token: {}", key);
      }
    };

    while (!input_view.empty()) {
      invoke(input_view);
    }
  }(tokens);

  return tokens;
}

template <typename Executor>
void Process(Executor& exec, const std::vector<TokenVariant>& tokens,
             Cache& cache) {
  std::stack<typename Executor::RuntimeType> runtime_stack;

  for (const auto& token : tokens) {
    std::visit(overloaded{[&runtime_stack](const ConstNumberToken& n) {
                            runtime_stack.push(n.value);
                          },
                          [&runtime_stack](const StringToken& n) {
                            runtime_stack.push(n.value);
                          },
                          [&exec, &runtime_stack, &cache](const auto& arg) {
                            exec.Invoke(
                                std::decay_t<decltype(arg)>::MnemonicValue,
                                runtime_stack, cache);
                          }},
               token);
  }
}

template <typename Executor>
auto ParseAndProcess(const std::string& input, Executor& executor,
                     Cache& cache) {
  const auto& tokens = Parse(input);
  Process(executor, tokens, cache);
}