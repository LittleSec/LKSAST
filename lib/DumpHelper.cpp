#include "DumpHelper.h"
#include "Handle1AST.h"

#include <iomanip>

#include "nlohmann/json.hpp"

using json = nlohmann::json;

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

void DumpPtrInfo2json(const std::string &filename, const Ptr2InfoType &ptrinfo,
                      JsonLogV V) {
  std::ofstream outfile(filename);
  DumpPtrInfo2json(outfile, ptrinfo, V);
  outfile.close();
}

void DumpPtrInfo2json(std::ofstream &of, const Ptr2InfoType &ptrinfo,
                      JsonLogV V) {
  json topj;
  for (auto &info : ptrinfo) {
    json pteesj;
    // FIXME: maybe consider logV for topj
    if (V == JsonLogV::FLAT_STRING) {
      pteesj = json::array();
      for (auto &ptee : info.second) {
        pteesj.push_back(ptee.name);
      }
    } else if (V == JsonLogV::ALL_DETAIL) {
      pteesj = json::array();
      for (auto &ptee : info.second) {
        json pteej;
        pteej["name"] = ptee.name;
        pteej["declloc"] = ptee.declloc;
        pteej["CallType"] = ptee.CallType2String();
        pteesj.push_back(pteej);
      }
    } else if (V == JsonLogV::NORMAL) {
      for (auto &ptee : info.second) {
        pteesj[ptee.name] = ptee.CallType2String();
      }
    } else {
      llvm::errs() << "[-] Err, Forget fill DumpPtrInfo2json() funciton when "
                      "add a new enum JsonLogV type\n";
    }
    topj[info.first.name] = pteesj;
  }
  of << std::setw(2) << topj << "\n";
}

void Dumpfun2json2txt(const std::string &filename,
                      const Fun2JsonType &mapinfo) {
  std::ofstream outfile(filename);
  Dumpfun2json2txt(outfile, mapinfo);
  outfile.close();
}

void Dumpfun2json2txt(std::ofstream &of, const Fun2JsonType &mapinfo) {
  for (auto &kv : mapinfo) {
    of << kv.first << " " << *(kv.second) << "\n";
  }
}

void Dumpfun2json2json(const std::string &filename,
                       const Fun2JsonType &mapinfo) {
  std::ofstream outfile(filename);
  Dumpfun2json2json(outfile, mapinfo);
  outfile.close();
}

void Dumpfun2json2json(std::ofstream &of, const Fun2JsonType &mapinfo) {
  json j;
  for (auto &kv : mapinfo) {
    j[kv.first] = *(kv.second);
  }
  of << std::setw(2) << j << "\n";
}

void TUAnalyzer::dumpTree(const std::string &filename) {
  std::ofstream outfile(filename);
  dumpTree(outfile);
  outfile.close();
}

void TUAnalyzer::dumpTree(std::ofstream &of) {
  of << "Function Info:\n";
  for (auto &tures : TUResult) {
    of << "  |- " << tures.funcname << ":\n";
    of << "  |    |- callgraph:\n";
    for (auto &cgn : tures._Callees) {
      of << "  |    |    |- " << cgn.name << "\n";
    }
    of << "  |    |    `-<End of callgraph>\n";
    of << "  |    |- resource:\n";
    for (auto &reso : tures._Resources) {
      of << "  |    |    |- " << reso.name << "\n";
    }
    of << "  |    |    `-<End of resource>\n";
    of << "  |    `-<End of Function: " << tures.funcname << ">\n";
  }
  of << "  `-<EOF>\n";
  of << "\n";
}

void TUAnalyzer::dumpJSON(const std::string &filename, JsonLogV V) {
  std::ofstream outfile(filename);
  dumpJSON(outfile);
  outfile.close();
}

void TUAnalyzer::dumpJSON(std::ofstream &of, JsonLogV V) {
  if (V == JsonLogV::FLAT_STRING) {
    llvm::errs()
        << "[!] TUAnalyzer::dumpJSON() does not have LogV FLAT_STRING,\n"
        << "    and here use logV NORMAL.\n";
    V = JsonLogV::NORMAL;
  }
  json topj;
  for (auto &tures : TUResult) {
    json funj;
    if (V == JsonLogV::ALL_DETAIL) {
      funj["declloc"] = tures.declloc;
      funj["callgraph"] = json::array();
      funj["resource_access"] = json::array();
      for (auto &cgn : tures._Callees) {
        json cgnodej;
        cgnodej["CallType"] = cgn.CallType2String();
        cgnodej["name"] = cgn.name;
        cgnodej["declloc"] = cgn.declloc;
        funj["callgraph"].push_back(cgnodej);
      }
      for (auto &reso : tures._Resources) {
        json resj;
        resj["name"] = reso.name;
        resj["ResourceType"] = reso.ResourceType2String();
        resj["AccessType"] = reso.AccessType2String();
        funj["resource_access"].push_back(resj);
      }
    } else if (V == JsonLogV::NORMAL) {
      json cgnodej;
      json resj;
      for (auto &cgn : tures._Callees) {
        cgnodej[cgn.name] = cgn.CallType2String();
      }
      for (auto &reso : tures._Resources) {
        if (resj.contains(reso.name)) {
          const std::string &srcAT = resj[reso.name].get<std::string>();
          const std::string &tgrAT = reso.AccessType2String();
          if ((srcAT == "W" && tgrAT == "R") ||
              (srcAT == "R" && tgrAT == "W")) {
            resj[reso.name] = "R/W";
          }
        } else {
          resj[reso.name] = reso.AccessType2String();
        }
      }
      funj["callgraph"] = cgnodej;
      funj["resource_access"] = resj;
    } else {
      llvm::errs() << "[-] Err, Forget fill TUAnalyzer::dumpJSON() funciton "
                      "when add a new enum JsonLogV type\n";
    }
    topj[tures.funcname] = funj;
  }
  of << std::setw(2) << topj << "\n";
}

} // namespace lksast
