#include "Utils.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>

#include <llvm/Transforms/Utils/ModuleUtils.h>

using namespace llvm;

Function* GetCtor(Module &M, Twine Name, unsigned Priority) {
  LLVMContext &C = M.getContext();
  Type* Void = Type::getVoidTy(C);
  FunctionType* VoidFun = FunctionType::get(Void, false);
  Function* Ctor = Function::Create(VoidFun, Function::PrivateLinkage, Name, &M);
  BasicBlock* Entry = BasicBlock::Create(C, "entry", Ctor);
  ReturnInst::Create(C, Entry);

  llvm::appendToGlobalCtors(M, Ctor, Priority);

  return Ctor;
}
