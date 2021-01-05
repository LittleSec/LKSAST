# third party

## [nlohmann/json.hpp](https://github.com/nlohmann/json/releases/tag/v3.9.1)
1. test code
```cpp
#include "nlohmann/json.hpp"
#include <iomanip>
#include <iostream>
#include <vector>
#include <string>

using json = nlohmann::json;

json testJson(const std::vector<int> &vec, const std::string &s) {
  // create an empty structure (null)
  json j;
  // add a number that is stored as double (note the implicit conversion of j to
  // an object)
  j["pi"] = 3.141;
  // add a Boolean that is stored as bool
  j["happy"] = true;
  // add a string that is stored as std::string
  j["name"] = "Niels";
  // add another null object by passing nullptr
  j["nothing"] = nullptr;
  // add an object inside the object
  j["answer"]["everything"] = 42;
  // add an array that is stored as std::vector (using an initializer list)
  j["list"] = {1, 0, 2};
  // add another object (using an initializer list of pairs)
  j["object"] = {{"currency", "USD"}, {"value", 42.99}};
  std::cout << std::setw(2) << j << std::endl;

  // instead, you could also write (which looks very similar to the JSON above)
  json j2 = {{"pi", 3.141},
             {"happy", true},
             {"name", s},
             {"nothing", vec},
             {"answer", {{"everything", 42}}},
             {"object", {{"currency", "USD"}, {"value", 42.99}}}};
  // j2["list"] = vec;
  std::cout << std::setw(2) << j2 << std::endl;

  return 0;
}

int main() {
  json j = testJson({10, 30, 20}, ts);
  std::ofstream of("test.json");
  of << j;
  of.close();
  return 0;
}
```
