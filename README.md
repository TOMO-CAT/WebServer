# WebServer

## 简介

## 内容

* C++ 规范编程
  * cpplint
  * clang-format
  * gtest
* 编译工具
  * makefile
  * blade
  * cmake
  * xmake
* 常用工具
  * docker
  * shell 脚本
  * markdown 文档
  * VSCode
  * 测试覆盖率
* 第三方库
  * jsoncpp

## 搭建环境

```bash
# 搭建 docker 镜像
bash docker.sh build

# 进入 docker
bash docker.sh run
```

## 编译

```bash
# 编译
make build

# 生成 compile_commands.json
make clangd

# 运行全量单测
make test

# 单测覆盖率
make coverage
```

## TODO

1. ./lint.sh 默认格式化所有代码文件, 只通过 exclude 反向选择
2. threadpool 避免饥饿死锁
