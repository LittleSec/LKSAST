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

/*
Need to analysis function pointer:
  |- Array gifconf_list --> [register_gifconf 1, ]
  |- struct pernet_operations.init --> [netdev_init, ]
  `-<EOF>
Function Info:
  |- function name 1:
  |    |- callgraph:
  |    |    |- struct net_device_ops.ndo_do_ioctl
  |    |    |- memcpy
  |    |    |- ...
  |    |    `-<End of callgraph>
  |    |- resource:
  |    |    |- struct net_device.netdev_ops
  |    |    |- ...
  |    |    `-<End of resource>
  |    `-<End of Function: function name 1>
  |- ...
  |- ...
  `-<EOF>
*/
void TUAnalyzer::dump(std::ofstream &of) {
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

void TUAnalyzer::dumpJSON(std::ofstream &of, bool ismin) {
  json topj;
  for (auto &tures : TUResult) {
    json funj;
    if (!ismin) {
      funj["declloc"] = tures.declloc;
    }
    funj["callgraph"] = json::array();
    for (auto &cgn : tures._Callees) {
      json cgnodej;
      cgnodej["CallType"] = cgn.CallType2String();
      cgnodej["name"] = cgn.name;
      if (!ismin) {
        cgnodej["declloc"] = cgn.declloc;
      }
      funj["callgraph"].push_back(cgnodej);
    }
    funj["resource access"] = json::array();
    for (auto &reso : tures._Resources) {
      json resj;
      resj["name"] = reso.name;
      resj["ResourceType"] = reso.ResourceType2String();
      resj["AccessType"] = reso.AccessType2String();
      funj["resource access"].push_back(resj);
    }
    topj[tures.funcname] = funj;
  }
  of << std::setw(2) << topj;
}

} // namespace lksast
