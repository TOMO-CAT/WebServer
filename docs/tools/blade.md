# blade

## 单测覆盖率

构建和运行测试时，加上 `--coverage` 参数，blade 就会加入覆盖率相关的编译选项，并在运行时收集测试覆盖率数据，目前仅支持 C++、Java 和 Scala。

C/C++ 测试覆盖率是通过 gcc 的 gcov 实现的, 测试完成后需要自己执行 gcov 或者 lcov 之类的第三方工具。

## 查看单测覆盖率

一种是通过 python 启动一个 http server：

```bash
blade test ... --full-test --run-unrepaired-tests --coverage
gcovr -r . --html --html-details -o coverage_reports/coverage_report.html
cd coverage_reports && python3 -m http.server 8080
```

我已经集成到 Makefile 里面了：

```bash
make coverage-server
```

另一种方式是使用 VSCode 插件 `Live Server`，它可以通过 Http 的方式访问当前项目的所有文件。

## Reference

[1] <https://github.com/chen3feng/blade-build/blob/master/doc/zh_CN/test.md>
