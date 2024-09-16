#include "library.h"

#include <sys/syscall.h>
#include <iostream>
#include <sys/unistd.h>

using namespace std;
using namespace llvm;

Constant* TraceLogPass::getStrConstant(const string& str, IRBuilder<> &builder) const {
    if (strConstants.count(str) > 0)
        return strConstants[str];
    Constant* constant = builder.CreateGlobalStringPtr(str);
    strConstants[str] = constant;
    return constant;
}

void TraceLogPass::declareFunctions(Module &M) const{
    LLVMContext &C = M.getContext();
    // 需要插入的声明包括
    // declare ptr @fopen(ptr noundef, ptr noundef)
    // declare i64 @syscall(i64 noundef, ...) #2
    // declare i32 @fprintf(ptr noundef, ptr noundef, ...) #1
    // declare i32 @fclose(ptr noundef) #1
    Type *IntTy32 = Type::getInt32Ty(C); // i32
    Type *IntTy64 = Type::getInt64Ty(C); // i64
    Type *PointerType = Type::getInt8PtrTy(C); //ptr

    // ptr @fopen(ptr, ptr)
    FunctionType* fopenType = FunctionType::get(PointerType, {PointerType, PointerType}, false);
    M.getOrInsertFunction(_fopen, fopenType);
    // i64 @syscall(i64, ...)
    FunctionType* syscallType = FunctionType::get(IntTy64, {IntTy64}, true);
    M.getOrInsertFunction(_syscall, syscallType);
    // i32 @fprintf(ptr, ptr, ...)
    FunctionType* fprintfType = FunctionType::get(IntTy32, {PointerType, PointerType}, true);
    M.getOrInsertFunction(_fprintf, fprintfType);
    // i32 @fclose(ptr)
    FunctionType* fcloseType = FunctionType::get(IntTy32, {PointerType}, false);
    M.getOrInsertFunction(_fclose, fcloseType);
}

void TraceLogPass::insertLogFuncName(Function &F) const {
    // 需要插入
    // %fid = call ptr @fopen(ptr @.str, ptr @.str.1)
    // %pid = call i64 (i64, ...) @syscall(i64 sys_pid)
    // %tid = call i64 (i64, ...) @syscall(i64 sys_tid)
    // %n = call i32 (ptr, ptr, ...) @fprintf(ptr %fid, ptr @.str.3, ptr @str, i64 %pid, i64 %fid)
    // %flag = call i32 @fclose(ptr %fid)
    if (F.isDeclaration())
        return;
    string funcName = F.getName().str();
    DISubprogram *debugInfo = F.getSubprogram();

    if (!debugInfo)
        return;
    // Get the starting line number of the function.
    unsigned line = debugInfo->getLine();
    // Print the line number.
    string directory = debugInfo->getDirectory().str();
    string filename = debugInfo->getFile()->getFilename().str();
    string srcFileName = directory + "/" + filename;

    string funcKey = srcFileName + "<" + to_string(line) + "<" + funcName;

    const char* fileToDump = "/log/funcs.txt";
    FILE* fd = fopen(fileToDump, "a");
    fprintf(fd, "%s\n", funcKey.c_str());
    fclose(fd);

    LLVMContext &context = F.getParent()->getContext();
    BasicBlock &bb = F.getEntryBlock();
    BasicBlock::iterator IP = bb.getFirstInsertionPt();

    IRBuilder<> builder(&*IP);
    // call ptr @fopen(ptr @.str, ptr @.str.1)
    Function* fopenFunc = F.getParent()->getFunction(_fopen);
    // fileName, mode
    ArrayRef<Value*> fopenArgs = {
        getStrConstant(fileName, builder), // icall_log.txt
        getStrConstant(mode, builder) // a
    };
    CallInst* fid = CallInst::Create(fopenFunc, fopenArgs, "", &*IP);

    // call i64 (i64, ...) @syscall(i64 sys_pid) 以及 call i64 (i64, ...) @syscall(i64 sys_tid)
    Function* syscallFunc = F.getParent()->getFunction(_syscall);
    CallInst* pid = CallInst::Create(syscallFunc, {builder.getInt64(SYS_getpid)}, "", &*IP);

    // %n = call i32 (ptr, ptr, ...) @fprintf(ptr %fid, ptr @.str.3, ptr @str, ptr %func_pointer, i64 %pid)
    Function* fprintfFunc = F.getParent()->getFunction(_fprintf);
    ArrayRef<Value*> fprintfArgs = {
            fid, getStrConstant(funcKeyLogContent, builder), getStrConstant(funcKey, builder), &F, pid
    };
    CallInst::Create(fprintfFunc, fprintfArgs, "", &*IP);

    // %flag = call i32 @fclose(ptr %fid)
    Function* fcloseFunc = F.getParent()->getFunction(_fclose);
    CallInst::Create(fcloseFunc, {fid}, "", &*IP);
}

void TraceLogPass::dumpICallToFile(string fileName, unsigned line, unsigned col) const {
    const char* fileToDump = "/log/icalls.txt";
    FILE* fd = fopen(fileToDump, "a");
    fprintf(fd, "%s:%d:%d\n", fileName.c_str(), line, col);
    fclose(fd);
}

void TraceLogPass::insertLogIndirectCall(CallInst *CI) const {
    // 需要插入
    // %fid = call ptr @fopen(ptr @.str, ptr @.str.1)
    // %pid = call i64 (i64, ...) @syscall(i64 sys_pid)
    // %tid = call i64 (i64, ...) @syscall(i64 sys_tid)
    // %n = call i32 (ptr, ptr, ...) @fprintf(ptr %fid, ptr @.str.3, ptr @str, i32 %line, i32 %col, i64 %pid, i64 %fid)
    // %flag = call i32 @fclose(ptr %fid)
    if (!CI->isIndirectCall())
        return;

    DebugLoc debugLoc = CI->getDebugLoc();
    if (!debugLoc)
        return;
    // 没有debug信息的通常是插入的call指令
    string directory = debugLoc->getDirectory().str();
    string filename = debugLoc->getFile()->getFilename().str();
    string srcFileName = directory + "/" + filename;

    unsigned line = debugLoc->getLine();
    unsigned col = debugLoc->getColumn();

    dumpICallToFile(srcFileName, line, col);

    LLVMContext &context = CI->getContext();
    IRBuilder<> builder(CI->getParent());
    // call ptr @fopen(ptr @.str, ptr @.str.1)
    Function* fopenFunc = CI->getModule()->getFunction(_fopen);
    // fileName, mode
    ArrayRef<Value*> fopenArgs = {
            getStrConstant(fileName, builder), // icall_log.txt
            getStrConstant(mode, builder) // a
    };
    CallInst* fid = CallInst::Create(fopenFunc, fopenArgs, "", CI);

    // call i64 (i64, ...) @syscall(i64 sys_pid)
    Function* syscallFunc = CI->getModule()->getFunction(_syscall);
    CallInst* pid = CallInst::Create(syscallFunc, {builder.getInt64(SYS_getpid)}, "", CI);

    // %n = call i32 (ptr, ptr, ...) @fprintf(ptr %fid, ptr @.str.3, ptr @str, i64 %pid, i64 %fid)
    Function* fprintfFunc = CI->getModule()->getFunction(_fprintf);
    // %s:%d:%d:%p:%ld
    ArrayRef<Value*> fprintfArgs = {
            fid, getStrConstant(icallLogContent, builder), getStrConstant(srcFileName, builder),
            builder.getInt32(line), builder.getInt32(col), CI->getCalledOperand(), pid
    };
    CallInst::Create(fprintfFunc, fprintfArgs, "", CI);

    // %flag = call i32 @fclose(ptr %fid)
    Function* fcloseFunc = CI->getModule()->getFunction(_fclose);
    CallInst::Create(fcloseFunc, {fid}, "", CI);
}

bool TraceLogPass::insertForAllFunc(Function &F) const {
    insertLogFuncName(F);

    for (BasicBlock &BB : F)
        for (Instruction &I : BB)
            if (CallInst *CI = dyn_cast<CallInst>(&I))
                insertLogIndirectCall(CI);
    return true;
}

PreservedAnalyses TraceLogPass::run(Module &M, ModuleAnalysisManager &MAM) {
    // 将插桩执行的函数声明先插入
    declareFunctions(M);

    // 遍历每个函数
    for (Module::iterator mi = M.begin(); mi != M.end(); ++mi) {
        Function &F = *mi;
        insertForAllFunc(F);
    }
    return PreservedAnalyses::all();
}

// 用clang -g -fpass-plugin=xx 可以成功运行这个pass，opt未知
PassPluginLibraryInfo getTraceLogCallBack() {
    const auto callback = [](PassBuilder &PB) {
        PB.registerPipelineEarlySimplificationEPCallback(
                [&](ModulePassManager &MPM, auto) {
                    MPM.addPass(TraceLogPass());
                    return true;
                });
    };
    return {
            LLVM_PLUGIN_API_VERSION, "TraceLogPass", "v0.1", callback
    };
}

// This part is the new way of registering your pass
extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
    return getTraceLogCallBack();
}