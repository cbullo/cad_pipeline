class Planner {



  void Investigate(const Node& node) {
    // auto inputs_version = node->GetInputsVersion(this, version_cache);
  
    // if (!node->HasVersion(inputs_version)) {
    //   auto inputs = node->GetInputs();
    //   operations.push(node, inputs);
    //   node->FollowInputs<Output>(this);
    // }

    std::stack nodes;
    nodes.push(&node);
    while (!nodes.empty()) {
      
    }
  }
};