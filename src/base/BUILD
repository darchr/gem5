
cc_library(
    name = "base",
    srcs = [
        "bitfield.cc",
        "cprintf.cc",
        "debug.cc",
        "logging.cc",
        "hostinfo.cc",
        "inifile.cc",
        "match.cc",
        "output.cc",
        "str.cc",
        "trace.cc",
        "types.cc",
    ],
    hdrs = [
        "bitfield.hh",
        "bitunion.hh",
        "compiler.hh",
        "cprintf.hh",
        "debug.hh",
        "logging.hh",
        "hostinfo.hh",
        "inifile.hh",
        "match.hh",
        "output.hh",
        "refcnt.hh",
        "str.hh",
        "trace.hh",
        "types.hh",
    ],
    deps = [
        "cprintf_formats",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "cprintf_formats",
    hdrs = ["cprintf_formats.hh"],
    visibility = ["//visibility:private"],
)

cc_test(
    name = "bitunion-test",
    srcs = ["bitunion.test.cc"],
    deps = [
        "base",
        "@gtest//:gtest",
        "@gtest//:gtest_main",
    ],
)

cc_test(
    name = "cprintf-test",
    srcs = ["cprintf.test.cc"],
    deps = [
        "base",
        "@gtest//:gtest",
        "@gtest//:gtest_main",
    ],
)
