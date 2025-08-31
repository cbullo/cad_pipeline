#pragma once


class Planner {
  void ProcessRequestsStack(std::stack<Request>& request_stack) {
    while(!request_stack.empty()) {
      auto op = request_stack.top();
      request_stack.pop();

    }
  }
};