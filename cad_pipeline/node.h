using Version = unsigned int;

class Node {
  private:
    Version GetVersion<Output>() const;

    std::vector<Version> GetInputsVersion(Planner& planner, VersionCache cache) const;
}