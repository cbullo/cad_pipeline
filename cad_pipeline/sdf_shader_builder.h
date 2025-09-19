void BuildShaderSource(const std::string& base_template, const std::string& shape_sdf) {
  std::format(base_template, shape_sdf);
}