#include "DumpHelper.h"

namespace lksast {

void DumpPtrInfo2txt(const std::string &filename, const Ptr2InfoType &ptrinfo,
                     bool istree) {
  std::ofstream outfile(filename);
  DumpPtrInfo2txt(outfile, ptrinfo, istree);
  outfile.close();
}

void DumpPtrInfo2txt(std::ofstream &of, const Ptr2InfoType &ptrinfo,
                     bool istree) {
  if (istree) {
    for (auto &info : ptrinfo) {
      of << info.first.name << " --> ";
      for (auto &ptee : info.second) {
        of << ptee.name << " # ";
      }
      of << "\n";
    }
  } else {
    of << "Need to analysis function pointer:\n";
    for (auto &info : ptrinfo) {
      of << "  |- " << info.first.name << " --> [";
      for (auto &ptee : info.second) {
        of << ptee.name << ", ";
      }
      of << "]\n";
    }
    of << "  `-<EOF>\n";
  }
}

} // namespace lksast
