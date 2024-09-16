#ifndef TRACELOGPASS_LIBRARY_H
#define TRACELOGPASS_LIBRARY_H

#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Casting.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include<map>

using namespace llvm;
using namespace std;


namespace {
    struct TraceLogPass : public PassInfoMixin<TraceLogPass> {
        const char *_fopen = "fopen";
        const char *_fclose = "fclose";
        const char *_fprintf = "fprintf";
        const char *_syscall = "syscall";

        string mode = "a";
        string fileName = "icall_log.txt";

        string icallLogContent = "I:%s:%d:%d:%p:%ld\n"; // file:line:col:func_address:pid
        string funcKeyLogContent = "F:%s:%p:%ld\n"; // func_key:func_address:pid

        mutable map<string, Constant*> strConstants;

        void dumpICallToFile(string fileName, unsigned line, unsigned col) const;

        Constant* getStrConstant(const string& str, IRBuilder<> &builder) const;

        void declareFunctions(Module &M) const;

        void insertLogFuncName(Function &F) const;

        void insertLogIndirectCall(CallInst *CI) const;

        bool insertForAllFunc(Function &F) const;

        PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
    };
}

#endif //TRACELOGPASS_LIBRARY_H
