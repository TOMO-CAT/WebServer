cc_library(
    name='logger',
    srcs=[
        'file_appender.cc',
        'log_capture.cc',
        'log_kv.cc',
        'logger.cc',
        'log_backtrace.cc',
        'sync_file_appender.cc',
        'async_file_appender.cc',
    ],
    hdrs=[
        'file_appender.h',
        'log_capture.h',
        'log.h',
        'log_kv.h',
        'logger.h',
        'log_backtrace.h',
        'sync_file_appender.h',
        'async_file_appender.h',
    ],
    deps=[
        '//util/toml:toml',
        '//util/macros:macros',
        '//thirdparty/cpptoml:cpptoml',
        '#backtrace',
        '#uuid',
        '#pthread',
    ],
    visibility=['PUBLIC'],
)
