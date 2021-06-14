// RUN: %clangxx %cxxflags %include_flags %ld_flags %s -Xclang -load -Xclang %lib_pass -o %t
// RUN: %t > %t.out
// RUN: %FileCheck %s < %t.out

#include <easy/jit.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include <functional>
#include <cstdio>

using namespace std::placeholders;

double add (double a, double b) {
  return a+b;
}

template<class T, class ... Args>
std::unique_ptr<easy::Function> get_function(easy::Context const &C, T &&Fun) {
  auto* FunPtr = easy::meta::get_as_pointer(Fun);
  return easy::Function::Compile(reinterpret_cast<void*>(FunPtr), C);
}

template<class T, class ... Args>
std::unique_ptr<easy::Function> EASY_JIT_COMPILER_INTERFACE _jit(T &&Fun, Args&& ... args) {
  auto C = easy::get_context_for<T, Args...>(std::forward<Args>(args)...);
  return get_function<T, Args...>(C, std::forward<T>(Fun));
}

void WriteOptimizedToFile(llvm::Module const &M) {

  std::error_code Error;
  llvm::raw_fd_ostream Out("bitcode", Error, llvm::sys::fs::F_None);

  Out << M;
}

int main() {
  std::unique_ptr<easy::Function> CompiledFunction = _jit(add, _1, 1);

  llvm::Module const & M = CompiledFunction->getLLVMModule();
  
  std::unique_ptr<llvm::Module> Embed = llvm::CloneModule(M);

  WriteOptimizedToFile(*Embed);

  return 0;
}
