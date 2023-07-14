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

#include "smtp.h"
#include "gmock/gmock.h"
#include <gtest/gtest.h>

#include "absl/status/status.h"

namespace ez {
namespace smtp {
namespace {

using testing::Return;

class MockSmtpAdapter : public SmtpAdapter {
public:
  MOCK_METHOD(absl::Status, Connect, (), (override));
  MOCK_METHOD(absl::Status, EnableTls, (), (override));
  MOCK_METHOD(absl::Status, Read, (int expected_return), (override));
  MOCK_METHOD(absl::Status, WriteLine, (absl::string_view message), (override));
  MOCK_METHOD(std::string, Hostname, (), (override));
  MOCK_METHOD(void, Disconnect, (), (override));
  MOCK_METHOD(void, EnableLogging, (), (override));
};

class SmtpTest : public ::testing::Test {
public:
  SmtpTest() : smtp_("username", "password", mock_adapter_) {}

protected:
  MockSmtpAdapter mock_adapter_;
  Smtp smtp_;
};

TEST_F(SmtpTest, SendSingleEmail) {
  EXPECT_CALL(mock_adapter_, Connect()).Times(1);
  EXPECT_CALL(mock_adapter_, Read(220))
      .Times(2)
      .WillRepeatedly(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("STARTTLS"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, EnableTls())
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, Hostname()).Times(1).WillOnce(Return("TestHost"));
  EXPECT_CALL(mock_adapter_, WriteLine("HELO TestHost"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, Read(250))
      .Times(6)
      .WillRepeatedly(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("AUTH PLAIN"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, Read(334))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("AHVzZXJuYW1lAHBhc3N3b3Jk"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, Read(235))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("MAIL FROM: <from@example.com>"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("RCPT TO: <to@example.com>"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("RCPT TO: <cc@example.com>"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("RCPT TO: <bcc@example.com>"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("DATA"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, Read(354))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("From: from@example.com\r\n"
                                       "To: to@example.com\r\n"
                                       "Cc: joe smith <cc@example.com>\r\n"
                                       "Bcc: jane smith <bcc@example.com>\r\n"
                                       "Subject: Subject\r\n\r\n"
                                       "This is the body.\r\n."))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("QUIT"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, Read(221))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, Disconnect()).Times(1).WillOnce(Return());

  auto status = smtp_.NewEmail()
                    .SetSender("from@example.com")
                    .AddRecipient("to@example.com")
                    .AddRecipient("cc@example.com", "joe smith", CarbonCopy)
                    .AddRecipient("bcc@example.com", "jane smith", Blind)
                    .SetSubject("Subject")
                    .SetBody("This is the body.")
                    .Send();

  ASSERT_TRUE(status.ok());
}

TEST_F(SmtpTest, SendMultipleEmails) {
  EXPECT_CALL(mock_adapter_, Connect()).Times(1);
  EXPECT_CALL(mock_adapter_, Read(220))
      .Times(2)
      .WillRepeatedly(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("STARTTLS"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, EnableTls())
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, Hostname()).Times(1).WillOnce(Return("TestHost"));
  EXPECT_CALL(mock_adapter_, WriteLine("HELO TestHost"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, Read(250))
      .Times(11)
      .WillRepeatedly(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("AUTH PLAIN"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, Read(334))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("AHVzZXJuYW1lAHBhc3N3b3Jk"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, Read(235))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("MAIL FROM: <from@example.com>"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("RCPT TO: <to@example.com>"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("RCPT TO: <cc@example.com>"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("RCPT TO: <bcc@example.com>"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("MAIL FROM: <from2@example.com>"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("RCPT TO: <to2@example.com>"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("RCPT TO: <cc2@example.com>"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("RCPT TO: <bcc2@example.com>"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("DATA"))
      .Times(2)
      .WillRepeatedly(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, Read(354))
      .Times(2)
      .WillRepeatedly(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("From: from@example.com\r\n"
                                       "To: to@example.com\r\n"
                                       "Cc: joe smith <cc@example.com>\r\n"
                                       "Bcc: jane smith <bcc@example.com>\r\n"
                                       "Subject: 1st subject\r\n\r\n"
                                       "This is the first body.\r\n."))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("From: from2@example.com\r\n"
                                       "To: to2@example.com\r\n"
                                       "Cc: joe smith <cc2@example.com>\r\n"
                                       "Bcc: jane smith <bcc2@example.com>\r\n"
                                       "Subject: 2nd subject\r\n\r\n"
                                       "This is the second body.\r\n."))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("QUIT"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, Read(221))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, Disconnect()).Times(1).WillOnce(Return());

  ASSERT_TRUE(smtp_.Connect().ok());

  auto status = smtp_.NewEmail()
                    .SetSender("from@example.com")
                    .AddRecipient("to@example.com")
                    .AddRecipient("cc@example.com", "joe smith", CarbonCopy)
                    .AddRecipient("bcc@example.com", "jane smith", Blind)
                    .SetSubject("1st subject")
                    .SetBody("This is the first body.")
                    .SendBatch();
  ASSERT_TRUE(status.ok());

  status = smtp_.NewEmail()
               .SetSender("from2@example.com")
               .AddRecipient("to2@example.com")
               .AddRecipient("cc2@example.com", "joe smith", CarbonCopy)
               .AddRecipient("bcc2@example.com", "jane smith", Blind)
               .SetSubject("2nd subject")
               .SetBody("This is the second body.")
               .SendBatch();
  ASSERT_TRUE(status.ok());

  ASSERT_TRUE(smtp_.Disconnect().ok());
}

TEST_F(SmtpTest, NewEmailResetsState) {
  EXPECT_CALL(mock_adapter_, Connect()).Times(1);
  EXPECT_CALL(mock_adapter_, Read(220))
      .Times(2)
      .WillRepeatedly(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("STARTTLS"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, EnableTls())
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, Hostname()).Times(1).WillOnce(Return("TestHost"));
  EXPECT_CALL(mock_adapter_, WriteLine("HELO TestHost"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, Read(250))
      .Times(8)
      .WillRepeatedly(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("AUTH PLAIN"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, Read(334))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("AHVzZXJuYW1lAHBhc3N3b3Jk"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, Read(235))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("MAIL FROM: <from@example.com>"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("RCPT TO: <to@example.com>"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("RCPT TO: <cc@example.com>"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("RCPT TO: <bcc@example.com>"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("MAIL FROM: <>"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("DATA"))
      .Times(2)
      .WillRepeatedly(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, Read(354))
      .Times(2)
      .WillRepeatedly(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("From: from@example.com\r\n"
                                       "To: to@example.com\r\n"
                                       "Cc: joe smith <cc@example.com>\r\n"
                                       "Bcc: jane smith <bcc@example.com>\r\n"
                                       "Subject: 1st subject\r\n\r\n"
                                       "This is the first body.\r\n."))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("From: \r\nSubject: \r\n\r\n\r\n."))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, WriteLine("QUIT"))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, Read(221))
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, Disconnect()).Times(1).WillOnce(Return());

  ASSERT_TRUE(smtp_.Connect().ok());

  auto status = smtp_.NewEmail()
                    .SetSender("from@example.com")
                    .AddRecipient("to@example.com")
                    .AddRecipient("cc@example.com", "joe smith", CarbonCopy)
                    .AddRecipient("bcc@example.com", "jane smith", Blind)
                    .SetSubject("1st subject")
                    .SetBody("This is the first body.")
                    .SendBatch();
  ASSERT_TRUE(status.ok());

  status = smtp_.NewEmail().SendBatch();
  ASSERT_TRUE(status.ok());

  ASSERT_TRUE(smtp_.Disconnect().ok());
}

TEST_F(SmtpTest, SendDisconnectsOnFailure) {
  EXPECT_CALL(mock_adapter_, Connect())
      .Times(1)
      .WillOnce(Return(absl::OkStatus()));
  EXPECT_CALL(mock_adapter_, Read(220))
      .Times(1)
      .WillRepeatedly(Return(absl::InternalError("Error")));
  EXPECT_CALL(mock_adapter_, Disconnect()).Times(1).WillOnce(Return());

  auto status = smtp_.NewEmail()
                    .SetSender("from@example.com")
                    .AddRecipient("to@example.com")
                    .SetSubject("Subject")
                    .SetBody("Body")
                    .Send();

  ASSERT_FALSE(status.ok());
}

} // namespace
} // namespace smtp
} // namespace ez
