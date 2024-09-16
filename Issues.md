插桩可参考 [LLVM_Instrumentation_Pass](https://github.com/imdea-software/LLVM_Instrumentation_Pass/)。

pass编写可参考：[llvm-tutor](https://github.com/banach-space/llvm-tutor), [clangpass](https://www.cs.cornell.edu/~asampson/blog/clangpass.html), [mlta](https://github.com/umnsec/mlta/blob/main/irgen.sh)。

存在的问题：[D77704](https://reviews.llvm.org/D77704)，在OSS-Fuzz下编译pass最好重新编译一个LLVM，编译时 `LLVM_LINK_LLVM_DYLIB` 打开，动态链接LLVM，防止出现 `'non-global-value-max-name-size' registered more than once!` 类似错误。

```
opt: CommandLine Error: Option 'non-global-value-max-name-size' registered more than once!

LLVM ERROR: inconsistency in registered CommandLine options
```




运行pass方式

- `opt`运行:

    * `clang -emit-llvm -c -g test.c`

    * `opt -verify-debuginfo-preserve -load-pass-plugin=$Path/NewICallInstPass.so -passes="icall-inst" test.bc`

    * `clang test1.bc -o test -L$Path1 -lLyFunc`

- `clang`: `clang -g -fpass-plugin=$Path/NewICallInstPass.so test.c -o test -L$PassLibPath/lyFuncs -lLyFunc`

`export PATH=/extra/llvm-15.0.0/bin:$PATH`

`CFLAGS="$CFLAGS -fpass-plugin=/extra/libs/NewICallInstPass.so" LDFLAGS="$LDFLAGS -L/extra/libs -lLyFunc" ./configure`

但是这里将pass用作plugin时，`configure`，`meson` 等构建工具可能无法识别对应的编译和链接选项报错，因此我修改插桩代码，使得无需链接新的文件。
同时我参考wllvm构建wclang，`wclang = clang -fpass-plugin=xxx`，编译的时候直接设置`CC=wclang`就行，
这样可以避免手动添加 `-fpass-plugin` 参数，从而绕开 `configure`, `meson` 对该编译选项的限制。

参考issue:

- [clang-run-concrete-llvm-pass-with-new-pass-manager-registry](https://stackoverflow.com/questions/72981340/clang-run-concrete-llvm-pass-with-new-pass-manager-registry)

- [how-to-automatically-register-and-load-modern-pass-in-clang](https://stackoverflow.com/questions/54447985/how-to-automatically-register-and-load-modern-pass-in-clang)

