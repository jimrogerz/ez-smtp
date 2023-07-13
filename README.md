# ez-smtp

An SMTP library built with [bazel](https://bazel.build/).

## Setup

Add the following to your WORKSPACE file:


```
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "ez-smtp",
    commit = "ba8316a07559cb7deca40c307bf3cd571eb62d29",
    remote = "https://github.com/jimrogerz/ez-smtp.git",
)
```

Add `"@ez-smtp//:smtp"` to your BUILD deps, then `#include "smtp.h"` to your source.

## Usage

Construct the Smtp class as follows (sendgrid is used as an example, but any SMTP server should work):

```
std::string smtp_host = "smtp.sendgrid.net";
int smtp_port = 587;
SmtpAdapterImpl adapter(smtp_host, smtp_port);
Smtp smtp("username", "password", adapter);
```

Now you can send an email like this:

```
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

```
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
```

The above example uses [status macros](https://github.com/jimrogerz/status_macros) to reduce boilerplate from
[Abseil status](https://abseil.io/docs/cpp/guides/status).
