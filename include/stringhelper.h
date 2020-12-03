#ifndef LKSAST_STRING_HELPER_H
#define LKSAST_STRING_HELPER_H

#include <string>
#include <vector>

namespace lksast {

std::string Trim(const std::string &s);
void SplitStr2Vec(const std::string &s, std::vector<std::string> &result,
                  const std::string delim);
std::string GetRealPurePath(const std::string &path);

} // namespace lksast

#endif
