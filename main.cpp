#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/SourceMgr.h"
#include <llvm/Transforms/Utils/Cloning.h>
#include "llvm/Linker/Linker.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Transforms/IPO/Inliner.h"
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/Host.h> 
#include <llvm/Target/TargetMachine.h> 
#include <llvm/Support/TargetRegistry.h> 
#include <llvm/Analysis/TargetTransformInfo.h> 
#include <llvm/Analysis/TargetLibraryInfo.h> 
#include <llvm/Support/FileSystem.h>
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/Transforms/IPO/AlwaysInliner.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"

#include "jit.h"
#include <easy/jit.h>

#include <iostream>
#include <memory>
#include <string>

#define arr_size 100

using namespace llvm;
using namespace llvm::orc;
using namespace std::placeholders;

struct small_block {
  int f1[100];
};

struct block {
  int * int_ptr;
  bool flag;
  int b1[arr_size];
  struct small_block sb;
};

void Optimize(llvm::Module& M, const char* Name, unsigned OptLevel) {
  llvm::PassManagerBuilder Builder;
  Builder.OptLevel = OptLevel;

  llvm::legacy::PassManager MPM;
  MPM.add(createAlwaysInlinerLegacyPass(false));
  //MPM.add(createSROAPass());

  Builder.populateModulePassManager(MPM);

  MPM.run(M);
}

extern "C" {

  int add (struct block * a, int b) {
    // int *dynamic_mem = (int *) malloc(sizeof(int));
    // *dynamic_mem = 100;

    //   int result = *(a->int_ptr);
    //   for (int i = 0; i < arr_size; i++){
    //     result += a->b1[i];
    //   }

    //   for (int i = 0; i < 10; i++){
    //     result += a->sb.f1[i];
    //   }

    //   if (a->flag)
    //     return -1;
    //   else
    //     return result;
    // }

    struct block c;
    int count = 0;
    int result = *(a->int_ptr);

    for (int i = 0; i < result; i++){
      c.b1[i] = i;
      count += c.b1[i];
      c.sb.f1[i] = i;
      count += c.sb.f1[i];
    }

    return count;
  }
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

std::unique_ptr<Module> buildprog(LLVMContext& ctx, StructType * struct_type)
{
    std::unique_ptr<Module> llmod = std::make_unique<Module>("test", ctx);

    Module* m = llmod.get();

    PointerType * struct_ptr_type = PointerType::get(struct_type, 0); 

    Function* add1_fn = Function::Create(
        FunctionType::get(Type::getInt32Ty(ctx), { struct_ptr_type }, false),
        Function::ExternalLinkage, "add1", m);

    BasicBlock* bb = BasicBlock::Create(ctx, "EntryBlock", add1_fn);

    IRBuilder<> builder(bb);

    Value* one = builder.getInt32(1);

    Argument* argx = add1_fn->getArg(0);
    argx->setName("x");

    Value* add = builder.CreateAdd(one, argx);
    
    builder.CreateRet(add);

    return llmod;
}

std::unique_ptr<Module> buildsrc(LLVMContext& Context)
{
    const StringRef add1_src =
        R"(
        define i32 @add1(i32 %x) {
        entry:
            %r = add nsw i32 %x, 1
            ret i32 %r
        }
    )";

    SMDiagnostic err;
    auto llmod = parseIR(MemoryBufferRef(add1_src, "test"), err, Context);
    if (!llmod) {
        std::cout << err.getMessage().str() << std::endl;
        return nullptr;
    }
    else {
        return llmod;
    }
}

int main()
{
    std::unique_ptr<easy::Function> CompiledFunction = _jit(add, _1, 1);
    llvm::Module const & M = CompiledFunction->getLLVMModule();
    std::unique_ptr<llvm::Module> Embed = llvm::CloneModule(M);
    for (Function & func: *Embed){
      func.addFnAttr(Attribute::AlwaysInline);
      func.removeFnAttr(Attribute::OptimizeNone);
    }
    // {
    //   // Create a new pass manager attached to it.
    //   auto TheFPM = std::make_unique<legacy::FunctionPassManager>(Embed.get());

    //   TheFPM->add(createPromoteMemoryToRegisterPass());

    //   TheFPM->doInitialization();

    //   for (Function & func: *Embed){
    //     TheFPM->run(func);
    //   }
    // }
    
    std::vector<StructType *> struct_types = Embed->getIdentifiedStructTypes();
    StructType * block_type = struct_types[0];

    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();

    auto jit = cantFail(TilJIT::Create());
    auto& ctx = jit->getContext();

    auto llmod = buildprog(ctx, block_type);
    //auto llmod = std::move(buildsrc(ctx));
    if (!llmod) return -1;

    // Link the main module with the easy::jit extracted module
    llvm::Linker::linkModules(*llmod, std::move(Embed));

    llvm::Function * add_func = llmod->getFunction("add");
    assert(add_func);
    //add_func->addFnAttr(Attribute::AlwaysInline);

    llvm::Function * add1_func = llmod->getFunction("add1");

    // define struct type 
    // std::vector<Type*> members;
    // members.push_back(IntegerType::get(ctx, 32) );
    // members.push_back(IntegerType::get(ctx, 32) );

    // StructType *const llvm_S = StructType::create( ctx, "block" );
    // llvm_S->setBody( members );

    // Call add in add1
    {
      BasicBlock * entry_block = &(add1_func->getEntryBlock());
      Instruction * ret = entry_block->getTerminator();
      IRBuilder<> builder(ret);

      std::vector<llvm::Value *> llvm_parameters;
      // llvm::Value * block_struct_alloca = builder.CreateAlloca(block_type);

      // llvm::Value * zero_index = llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx), 0, false);
      // llvm::Value * field1_index = llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx), 0, false);
      // llvm::Value * field2_index = llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx), 1, false);

      // std::vector<llvm::Value*> indices1 = {zero_index, field1_index};
      // std::vector<llvm::Value*> indices2 = {zero_index, field2_index};

      // llvm::Value* field1 = builder.CreateGEP(block_struct_alloca, indices1);
      // llvm::Value* field2 = builder.CreateGEP(block_struct_alloca, indices2);
    
      // builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx), 12, true), field1);
      // builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx), 34, true), field2);

      // llvm_parameters.push_back(builder.CreateLoad(block_struct_alloca));

      llvm_parameters.push_back(add1_func->getArg(0));

      llvm::Value * result = builder.CreateCall(add_func, llvm_parameters);
      builder.CreateRet(result);

      ret->eraseFromParent();
    }
 
    Optimize(*llmod, "test", 3);
    
    WriteOptimizedToFile(*llmod);

    cantFail(jit->addModule(std::move(llmod)));

    JITEvaluatedSymbol sym = cantFail(jit->lookup("add1"));

    auto* add1 = (int (*)(struct block *))(intptr_t)sym.getAddress();

    struct block b;

    b.flag = false;
    b.int_ptr = (int *) malloc(sizeof(int));
    *(b.int_ptr) = 100;

    for (int i = 0; i < arr_size; i++){
      b.b1[i] = i;
    }

    for (int i = 0; i < 10; i++){
      b.sb.f1[i] = i;
    }

    std::cout << "Result: " << add1(&b) << std::endl;

    return 0;
}
