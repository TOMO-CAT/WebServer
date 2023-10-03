# 所需要的库的列表
_DEPS = [
    'prometheus-cpp-core',
    'prometheus-cpp-pull',
]

# 生成 poco 包中各个库的 blade 描述
[prebuilt_cc_library(
    name=name,
    libpath_pattern='lib',
    hdrs=[],
    export_incs='include') for name in _DEPS
 ]

# 对外接口
cc_library(
    name='prometheus-cpp',
    export_incs=['include'],
    deps=[':' + dep for dep in _DEPS],
    hdrs=[
        'include/prometheus/registry.h',
        'include/prometheus/summary.h',
        'include/prometheus/exposer.h',
        'include/prometheus/detail/utils.h',
        'include/prometheus/counter.h',
    ],
    visibility=['PUBLIC'],
)
