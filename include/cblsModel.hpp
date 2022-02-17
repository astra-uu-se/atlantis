#pragma once

#include <filesystem>

class CBLSModel {
 private:
  const std::filesystem::path& _modelPath;

 public:
  explicit CBLSModel(const std::filesystem::path& modelPath) : _modelPath(modelPath) {}

  void solve();
};