package(default_visibility = ["//visibility:public"])

load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_binary",
    "envoy_cc_library"
)

envoy_cc_binary(
    name = "envoy",
    repository = "@envoy",
    deps = [
        "@envoy//source/exe:envoy_main_entry_lib",
        ":cache_filter_config",
    ],
)

envoy_cc_library(
    name = "cache_filter_lib",
    repository = "@envoy",
    srcs = ["cache_filter.cc", "cache.h", "request_coalescer.h"],
    hdrs = ["cache_filter.h", "cache.h", "request_coalescer.h"],
    deps = [
        "@envoy//source/extensions/filters/http/common:pass_through_filter_lib",
    	"@envoy//source/common/common:thread_lib",
        "@envoy//envoy/http:filter_interface",
        "@envoy//source/common/buffer:buffer_lib",
        "@envoy//source/common/http:header_map_lib",
    ],
)

envoy_cc_library(
    name = "cache_filter_config",
    repository = "@envoy",
    srcs = ["cache_filter_config.cc"],
    deps = [
        ":cache_filter_lib",
        "@envoy//source/extensions/filters/http/common:factory_base_lib",
        "@envoy//envoy/server:filter_config_interface",
    ],
)

