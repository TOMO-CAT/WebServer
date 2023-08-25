# 指定基础镜像
FROM ubuntu:22.04

# 安装依赖
RUN sed -i 's/archive.ubuntu.com/mirrors.aliyun.com/g' /etc/apt/sources.list \
    && apt-get clean && apt-get update && apt-get install -y --fix-missing \
    build-essential \
    git \
    curl \
    python2.7 \
    ninja-build \
    vim \
    sudo \
    bear \
    uuid-dev \
    # 如果需要在 docker 中使用 apt 的话需要注掉这一行
    # && rm -rf /var/lib/apt/lists/* \
    && ln -s /usr/bin/python2.7 /usr/bin/python

# 指定工作目录
WORKDIR /webserver

# 安装 VSCode 相关插件依赖的二进制
RUN apt-get install -y --fix-missing \
    python3-pip \
    clangd \
    clang-format \
    && pip install cpplint
