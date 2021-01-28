#ifndef LKSAST_DUMP_HELPER_H
#define LKSAST_DUMP_HELPER_H

#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>

#include "ConfigManager.h"
#include "Handle1AST.h"

namespace lksast {

void DumpPtrInfo2txt(const std::string &filename, const Ptr2InfoType &ptrinfo,
                     bool istree = false);
void DumpPtrInfo2txt(std::ofstream &of, const Ptr2InfoType &ptrinfo,
                     bool istree = false);
void DumpPtrInfo2json(const std::string &filename, const Ptr2InfoType &ptrinfo,
                      JsonLogV V = JsonLogV::NORMAL);
void DumpPtrInfo2json(std::ofstream &of, const Ptr2InfoType &ptrinfo,
                      JsonLogV V = JsonLogV::NORMAL);

using Fun2JsonType = unordered_map<std::string, shared_ptr<std::string>>;

void Dumpfun2json2txt(const std::string &filename, const Fun2JsonType &mapinfo);
void Dumpfun2json2txt(std::ofstream &of, const Fun2JsonType &mapinfo);
void Dumpfun2json2json(const std::string &filename,
                       const Fun2JsonType &mapinfo);
void Dumpfun2json2json(std::ofstream &of, const Fun2JsonType &mapinfo);

} // namespace lksast

#endif
