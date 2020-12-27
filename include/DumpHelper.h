#ifndef LKSAST_DUMP_HELPER_H
#define LKSAST_DUMP_HELPER_H

#include <fstream>
#include <string>

#include "Handle1AST.h"

namespace lksast {

void DumpPtrInfo2txt(const std::string &filename, const Ptr2InfoType &ptrinfo,
                     bool istree = false);
void DumpPtrInfo2txt(std::ofstream &of, const Ptr2InfoType &ptrinfo,
                     bool istree = false);

} // namespace lksast

#endif
