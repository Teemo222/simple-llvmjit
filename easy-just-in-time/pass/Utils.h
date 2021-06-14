#ifndef UTILS_H
#define UTILS_H

#include <llvm/ADT/Twine.h>

namespace llvm {
  class Module;
  class Function;
}

llvm::Function* GetCtor(llvm::Module &M, llvm::Twine Name, unsigned Priority = 65535);

#endif // UTILS_H
