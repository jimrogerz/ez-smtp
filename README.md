# EZ-SMTP

A C++ SMTP library built with [bazel](https://bazel.build/).

## Setup

Add the following to your WORKSPACE file:


```
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "ez-smtp",
    commit = "78b23f01ab36da2bcd1f20d4d5e3639d95bb77fa",
    remote = "https://github.com/jimrogerz/ez-smtp.git",
)
```

Add `"@ez-smtp//:smtp"` to your BUILD deps.

## Usage

Sending a single email:

```cpp
#include "smtp.h"

// May reuse this instance
Smtp smtp("smtp.sendgrid.net", /* port= */ 587, "username", "password", adapter);

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