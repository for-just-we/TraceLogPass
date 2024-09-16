
添加pass参考[NewPassManager](https://github.com/llvm/llvm-project/blob/llvmorg-15.0.0/llvm/docs/NewPassManager.rst)，如果要将这个pass添加到llvm中作为llvm默认执行的pass，需要：

在编译pass成功后用llvm编译时添加参数 `-fpass-plugin=TraceLogPass.so` 加载pass进行插桩