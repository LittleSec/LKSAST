#ifndef LKSAST_COMMON_H
#define LKSAST_COMMON_H

namespace lksast {

#define CLASS_MEMBER_GETTER_PROTO(_rettype, _name, _member)                    \
  _rettype get##_name() { return _member; }

#define PRIVATE_MEMBER_GETTER(_rettype, _name)                                 \
  CLASS_MEMBER_GETTER_PROTO(_rettype, _name, _##_name)

} // namespace lksast

#endif