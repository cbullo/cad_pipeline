#pragma once

struct Request {
  GeomId id;
  std::vector<RuntimeType> params;
  std::function<void()> op;
};
