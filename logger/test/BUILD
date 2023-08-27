gen_rule(
    name='logger_test_gen',
    srcs=[
        'conf/logger.conf',
    ],
    outs=[
        'conf/logger.conf',
    ],
    # 在 BLADE_ROOT 目录执行
    cmd='cp logger/test/conf/logger.conf $FIRST_OUT',
)

cc_binary(
    name='logger_test',
    srcs=[
        'logger_test.cpp',
    ],
    deps=[
        '//logger:logger',
        ':logger_test_gen',
    ],
    visibility=['PUBLIC'],
    extra_cppflags=[
        '-rdynamic',
    ],
)


cc_binary(
    name='signal8_test',
    srcs=[
        'signal8_test.cc',
    ],
    deps=[
        '//logger:logger',
    ]
)
