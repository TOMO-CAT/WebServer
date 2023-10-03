#!/bin/bash

# 遇到非零的返回值时直接退出
set -e

# 蓝色 info 日志
function log_info() {
    printf "\e[34m\e[1m[INFO]\e[0m $*\n"
}

# 橙色 warn 日志
function log_warn() {
    printf "\033[0;33m[WARN]\e[0m $*\n"
}

# 红色 error 日志
function log_error() {
    printf "\033[0;31m[ERRO]\e[0m $*\n"
}

# 绿色 ok 日志
function log_ok() {
    printf "\e[32m\e[1m[ OK ]\e[0m $*\n"
}

# lint 单个目录
function lint_single_directory() {
    log_info "start lint $1"
    [ -d $1 ] || {
        log_error "directory '$1' don't exist"
        exit 1
    }

    found_files=`find ./logger -name "*.h" -o -name "*.cc" -o -name "*.hpp" -o -name "*.cpp"`
    if [ -n "$found_files" ]; then
        # 如果找到至少一个文件，则执行 cpplint
        res=$(echo "${found_files}" | xargs cpplint)
        set +e
        echo "$res" | grep -v "Done processing"
        set -e
    fi
}

# 需要排除的相对路径文件夹, 后面会通过 readlink -f 保证相对路径一致性
EXCLUDE_FOLDER_LIST="
thirdparty
util/toml
build64_release
docs
.cache
.git
.vscode
coverage_reports
"

function lint_all_directory() {
    find . -mindepth 1 -type d  | while read -r folder; do
        # 检查文件所在路径是否包含在排除列表中
        # 通过 * 号跳过所有 EXCLUDE_FOLDER_LIST 指定的文件夹和子文件夹
        exclude=false
        for exclude_folder in `echo "${EXCLUDE_FOLDER_LIST}"`; do
            if [[ `readlink -f "${folder}"` == `readlink -f "${exclude_folder}"`* ]]; then
                exclude=true
                break
            fi
        done

        # 如果不在排除列表中，则运行 CPPLINT
        if ! ${exclude}; then
            lint_single_directory "${folder}"
        fi
    done
}

if [ $# -ge 1 ]; then
    lint_single_directory "$1"
else
    lint_all_directory
fi

[ $? == 0 ] && log_ok
