# xmake

## 安装

```bash
XMAKE_COMMIT_VERSION="304943e79b6e2c67d2c010f053d8cd2a42460ddd"

xmake_install_dir=$(mktemp -d)
cd ${xmake_install_dir}

# clone xmake from gitee
git clone --recursive https://gitee.com/tboox/xmake.git
cd ${xmake_install_dir}/xmake
git checkout "${XMAKE_COMMIT_VERSION}"

# install
./configure
./scripts/get.sh __local__ __install_only__
source ~/.xmake/profile

# check xmake version
xmake --version

# clear
rm -rf ${xmake_install_dir}
```
