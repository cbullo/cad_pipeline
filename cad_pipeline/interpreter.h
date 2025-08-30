#pragma once

#include <charconv>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

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

using ConstNumber = Token<'N'>;
using Cube = Token<'C'>;

using Tokens = std::variant<ConstNumber, Cube>;

std::vector<Tokens> Parse(const std::string& input) {
  std::string_view input_view(input);
  std::vector<Tokens> tokens;

  [&input_view]<typename... T>(std::vector<std::variant<T...>>& tokens) {
    auto invoke = [&tokens, &input_view](std::string_view& input) {
      const char key = input.front();
      input.remove_prefix(1);

      (
          [key, &input_view, &tokens]() {
            if (key == T::MnemonicValue) {
              const auto& token = T::Consume(input_view);
              tokens.push_back(Tokens(token));
              std::cout << key;
            }
          }(),
          ...);
    };

    while (!input_view.empty()) {
      invoke(input_view);
    }
  }(tokens);

  return tokens;
}

// template <typename Executor,
//           typename RuntimeValue = typename Executor::RuntimeValue>
auto ParseAndExecute(const std::string& input) {
  const auto& tokens = Parse(input);
}