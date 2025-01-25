# EZ-SMTP

A C++ SMTP library built with [bazel](https://bazel.build/).

## Setup

Add the following build_deps to your MODULE.bazel file:

```
bazel_dep(name = "rules_cc", version = "0.1.0")
bazel_dep(name = "bazel_skylib", version = "1.7.1")
bazel_dep(name = "abseil-cpp", version = "20240722.1")
```

Add the http_archive rule:

```
http_archive = use_repo_rule("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
```

Add the http_archive using the latest commit from https://github.com/jimrogerz/ez-smtp/commits/main/:

```
EZ_SMTP_COMMIT = "1ea95355f3d7d58949e7b5306b894301c5d6ed8a"
http_archive(
    name = "ez-smtp",
    strip_prefix = "status_macros-" + EZ_SMTP_COMMIT,
    url = "https://github.com/jimrogerz/ez-smtp/archive/%s.zip" % EZ_SMTP_COMMIT,
)
```

Add `"@ez-smtp//:smtp"` to your BUILD deps.

## Usage

Sending a single email:

```cpp
#include "smtp.h"

// May reuse this instance
Smtp smtp("hostname", /* port= */ 587, "username", "password");

auto status = smtp.NewEmail()
                  .SetSender("someone@gmail.com")
                  .AddRecipient("someone@gmail.com")
                  .AddRecipient("someone@gmail.com", "John Smith", CarbonCopy)
                  .AddRecipient("someone@gmail.com", "Jane Smith", Blind)
                  .SetSubject("Hello from EZ-SMTP!")
                  .SetBody("I'm trying out this amazing SMTP library.")
                  .Send();

if (!status.ok()) {
   std::cerr << status << std::endl;
}
```

The above usage may be repeated with the same Smtp instance to send multiple
emails, however it will connect and disconnect from the receiving server with
each email. If you need to send multiple emails at once, use the batch APIs
(Connect, SendBatch, Disconnect), e.g.:

```cpp
absl::Status SendBatch(Smtp &smtp) {
  RETURN_IF_ERROR(smtp.Connect());
  RETURN_IF_ERROR(smtp.NewEmail()
                      .SetSender("someone@gmail.com")
                      .AddRecipient("someone@gmail.com")
                      .SetSubject("Hello")
                      .SetBody("This is an example.")
                      .SendBatch());
  RETURN_IF_ERROR(smtp.NewEmail()
                      .SetSender("someone@gmail.com")
                      .AddRecipient("someone@gmail.com")
                      .SetSubject("Hello again")
                      .SetBody("This is the second example")
                      .SendBatch());
  return smtp.Disconnect();
}
```

See [status macros](https://github.com/jimrogerz/status_macros) to reduce boilerplate from
[Abseil status](https://abseil.io/docs/cpp/guides/status).

## Contributing

Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html). Update unit tests and confirm passing:

```
bazel test :smtp_test
```

Format the code:

```
clang-format -i *.cc *.h
```