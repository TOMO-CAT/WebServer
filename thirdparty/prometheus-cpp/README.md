# prometheus

## 安装

```bash
cd /tmp
git clone https://github.com/jupp0r/prometheus-cpp.git
cd prometheus-cpp

# fetch third-party dependencies
git submodule init
git submodule update

mkdir _build
cd _build

# run cmake
# 1) 编译动态库
# cmake .. -DBUILD_SHARED_LIBS=ON -DENABLE_PUSH=OFF -DENABLE_COMPRESSION=OFF
# 2) 编译静态库
cmake .. -DBUILD_SHARED_LIBS=NO -DENABLE_PUSH=OFF -DENABLE_COMPRESSION=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON

# build
cmake --build . --parallel 4

# run tests
ctest -V

# install the libraries and headers
cmake --install . --prefix ./output
```

## 迁移到 thirdparty

```bash
cp -r /tmp/prometheus-cpp/_build/output/* /webserver/thirdparty/prometheus-cpp/
```
