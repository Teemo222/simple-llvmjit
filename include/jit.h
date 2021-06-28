#ifndef TIL_TILJIT
#define TIL_TILJIT

#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/ExecutionEngine/Orc/IRTransformLayer.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/IRReader/IRReader.h"

#include <memory>
#include <iostream>

using namespace llvm;
using namespace llvm::orc;

class TilJIT
{
private:
    ExecutionSession es;
    RTDyldObjectLinkingLayer linker;
    IRCompileLayer compiler;
    IRTransformLayer optimizer;

    DataLayout dl;
    MangleAndInterner mangle;
    ThreadSafeContext ctx;

    JITDylib& jd;

public:
    TilJIT(JITTargetMachineBuilder jtmb, DataLayout dl) :
        linker(es, []() { return std::make_unique<SectionMemoryManager>(); }),
        compiler(es, linker, std::make_unique<ConcurrentIRCompiler>(std::move(jtmb))),
        optimizer(es, compiler, optimizeModule),
        dl(std::move(dl)), mangle(es, this->dl), ctx(std::make_unique<LLVMContext>()),
        jd(es.createBareJITDylib("__til_lib"))
    {
        jd.addGenerator(cantFail(
            DynamicLibrarySearchGenerator::GetForCurrentProcess(dl.getGlobalPrefix())));
    }

    static Expected<std::unique_ptr<TilJIT>> Create()
    {
        auto jtmb = JITTargetMachineBuilder::detectHost();
        if (!jtmb) return jtmb.takeError();

        auto dl = jtmb->getDefaultDataLayoutForTarget();
        if (!dl) return dl.takeError();

        return std::make_unique<TilJIT>(std::move(*jtmb), std::move(*dl));
    }

    const DataLayout& getDataLayout() const { return dl; }

    LLVMContext& getContext() { return *(ctx.getContext()); }

    Error addModule(std::unique_ptr<Module> M)
    {
        //return compiler.add(jd, ThreadSafeModule(std::move(M), ctx));
        return optimizer.add(jd, ThreadSafeModule(std::move(M), ctx));
    }

    Expected<JITEvaluatedSymbol> lookup(StringRef name)
    {
        return es.lookup({ &jd }, mangle(name.str()));
    }

private:
    static Expected<ThreadSafeModule>
    optimizeModule(ThreadSafeModule TSM, const MaterializationResponsibility &R) {
        TSM.withModuleDo([](Module &M) {

            llvm::PassManagerBuilder Builder;
            Builder.OptLevel = 3;
            Builder.Inliner = createFunctionInliningPass(3, 0, false);

            llvm::legacy::PassManager MPM;
            Builder.populateModulePassManager(MPM);

            std::error_code Error2;
            llvm::raw_fd_ostream Out2("before", Error2, llvm::sys::fs::F_None);
            Out2 << M;

            MPM.run(M);

            std::cout << "run optimization" << std::endl;

            std::error_code Error;
            llvm::raw_fd_ostream Out("after", Error, llvm::sys::fs::F_None);

            Out << M;
        });

    return std::move(TSM);
  }
};

#endif // TIL_TILJIT