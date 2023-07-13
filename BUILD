# EZ-SMTP (C) 2023 Jim Rogers.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "smtp",
    srcs = ["smtp.cc"],
    hdrs = ["smtp.h"],
    deps = [
        "@status_macros//:status_macros",
		"@boost//:asio",
        "@boost//:asio_ssl",
        "@boost//:serialization",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/status:statusor",
   ],
)

cc_test(
  name = "smtp_test",
  srcs = ["smtp_test.cc"],
  deps = [
    ":smtp",
    "@com_google_absl//absl/status",
    "@com_google_absl//absl/status:statusor",
    "@com_google_googletest//:gtest_main"
  ],
)

cc_binary(
    name = "send_mail",
    srcs = ["send_mail.cc"],
    deps = [
        ":smtp",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
    ],
)
