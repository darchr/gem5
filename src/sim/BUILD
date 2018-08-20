cc_library(
    name = "core",
    srcs = ["core.cc"],
    hdrs = ["core.hh"],
    deps = [
        "//base:base",
        "eventq",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "eventq",
    srcs = ["eventq.cc"],
    hdrs = ["eventq.hh"],
    deps = [
        "//base:flags",
        "//base:types",
        "//debug:Event",
        "//serialize",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "serialize",
    srcs = ["serialize.cc"],
    hdrs = ["serialize.hh"],
    deps = [
        "//base:base"
    ],
    visibility = ["//visibility:public"],
)
