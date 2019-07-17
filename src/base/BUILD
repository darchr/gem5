# Note: To deal with the circular dependencies, we can declare all of these
# files in a single library called "base"

cc_library(
    name = "bitfield",
    srcs = ["bitfield.cc"],
    hdrs = ["bitfield.hh"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "bitunion",
    hdrs = ["bitunion.hh"],
    deps = [
        "bitfield",
    ],
)

cc_library(
    name = "compiler",
    hdrs = ["compiler.hh"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "cprintf_formats",
    hdrs = ["cprintf_formats.hh"],
    visibility = ["//visibility:private"],
)

cc_library(
    name = "cprintf",
    srcs = ["cprintf.cc"],
    hdrs = ["cprintf.hh"],
    visibility = ["//visibility:public"],
    deps = [
        "cprintf_formats",
        "compiler",
    ],
)

cc_library(
    name = "logging",
    srcs = ["logging.cc"],
    hdrs = ["logging.hh"],
    visibility = ["//visibility:public"],
    deps = [
        "hostinfo",
        "compiler",
        "cprintf",
    ]
)

cc_library(
    name = "hostinfo",
    srcs = ["hostinfo.cc"],
    hdrs = ["hostinfo.hh"],
    deps = [
        "types",
        "logging",
        "string"
    ]
)

cc_test(
    name = "bitunion-test",
    srcs = ["bitunion.test.cc"],
    deps = [
        "bitunion",
        "@gtest//:gtest",
        "@gtest//:gtest_main",
    ],
)

cc_test(
    name = "cprintf-test",
    srcs = ["cprintf.test.cc"],
    deps = [
        "cprintf",
        "@gtest//:gtest",
        "@gtest//:gtest_main",
    ],
)
