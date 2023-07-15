// EZ-SMTP Copyright 2023 Jim Rogers (jimrogerz@gmail.com).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "smtp.h"

ABSL_FLAG(std::string, smtp_server, "", "SMTP server");
ABSL_FLAG(int, smtp_port, 587, "SMTP port");
ABSL_FLAG(std::string, smtp_username, "", "SMTP username");
ABSL_FLAG(std::string, smtp_password, "", "SMTP password");
ABSL_FLAG(std::string, from, "", "Sender email");
ABSL_FLAG(std::string, to, "", "Recipient email");
ABSL_FLAG(bool, batch, false, "Batch mode");

using ez::smtp::Blind;
using ez::smtp::CarbonCopy;
using ez::smtp::Smtp;
using ez::smtp::SmtpAdapterImpl;

absl::Status SendSingle(Smtp &smtp) {
  return smtp.NewEmail()
      .SetSender(absl::GetFlag(FLAGS_from))
      .AddRecipient(absl::GetFlag(FLAGS_to))
      .SetSubject("subject1")
      .SetBody("body1")
      .Send();
}

absl::Status SendBatch(Smtp &smtp) {
  RETURN_IF_ERROR(smtp.Connect());
  RETURN_IF_ERROR(smtp.NewEmail()
                      .SetSender(absl::GetFlag(FLAGS_from))
                      .AddRecipient(absl::GetFlag(FLAGS_to))
                      .SetSubject("subject1")
                      .SetBody("body1")
                      .SendBatch());
  RETURN_IF_ERROR(smtp.NewEmail()
                      .SetSender(absl::GetFlag(FLAGS_from))
                      .AddRecipient(absl::GetFlag(FLAGS_to))
                      .SetSubject("subject2")
                      .SetBody("body2")
                      .SendBatch());
  return smtp.Disconnect();
}

/** Sends a test email. Example usage:
 *
 * bazel run :send_mail -- \
 *  --server_address="smtp.sendgrid.net" \
 *  --server_port=587 \
 *  --smtp_username="<username>" \
 *  --smtp_password="<password" \
 *  --from="<email>" \
 *  --to="<email>"
 *  --batch=true
 */
int main(int argc, char **argv) {
  absl::ParseCommandLine(argc, argv);

  Smtp smtp(absl::GetFlag(FLAGS_smtp_server), absl::GetFlag(FLAGS_smtp_port),
            absl::GetFlag(FLAGS_smtp_username),
            absl::GetFlag(FLAGS_smtp_password));
  smtp.EnableLogging();

  auto status = absl::GetFlag(FLAGS_batch) ? SendBatch(smtp) : SendSingle(smtp);
  if (!status.ok()) {
    std::cerr << status << std::endl;
    return 1;
  }

  return 0;
}
