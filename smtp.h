// EZ-SMTP (C) 2023 Jim Rogers.
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

#ifndef EZ_SMTP_H
#define EZ_SMTP_H

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "status_macros.h"
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <map>
#include <queue>
#include <sstream>

namespace ez {
namespace smtp {

/** Interface to connect, read and write SMTP messages over a tls socket. */
class SmtpAdapter {
public:
  // Connects to the SMTP server.
  virtual absl::Status Connect() = 0;

  // Upgrades the connection to tls.
  virtual absl::Status EnableTls() = 0;

  // Reads from the socket confirming the SMTP expected value.
  virtual absl::Status Read(const int expected_return) = 0;

  // Writes the message appending a newline.
  virtual absl::Status WriteLine(absl::string_view message) = 0;

  // Returns the connected hostname.
  virtual std::string Hostname() = 0;

  // Disconnects from the SMTP server.
  virtual void Disconnect() = 0;

  // Enables logging to cout.
  virtual void EnableLogging() = 0;
};

/** Concrete implementation of SocketAdapter. */
class SmtpAdapterImpl : public SmtpAdapter {
public:
  typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> SslSocket;

  SmtpAdapterImpl(absl::string_view hostname, const int port)
      : hostname_(std::string(hostname)), port_(port),
        ssl_context_(boost::asio::ssl::context::tlsv12), enable_tls_(false) {}

  absl::Status Connect() override;

  absl::Status EnableTls() override {
    boost::system::error_code error;
    socket_->handshake(SslSocket::client, error);
    if (error) {
      return absl::InternalError(error.message());
    }
    enable_tls_ = true;
    return absl::OkStatus();
  }

  absl::Status Read(int expected_return) override;

  absl::Status WriteLine(absl::string_view message) override;

  std::string Hostname() override {
    return socket_->next_layer().remote_endpoint().address().to_string();
  }

  void Disconnect() override {
    enable_tls_ = false;
    socket_.reset();
  }

  void EnableLogging() override { log_ = true; }

private:
  std::string hostname_;
  const int port_;
  boost::asio::io_service io_service_;
  boost::asio::ssl::context ssl_context_;
  boost::shared_ptr<SslSocket> socket_;
  bool enable_tls_;
  bool log_;
};

enum RecipientType { Primary = 0, CarbonCopy = 1, Blind = 2 };

struct Recipient {
  Recipient() : recipient_type(Primary) {}
  std::string address;
  std::string name;
  RecipientType recipient_type;
};

struct Sender {
  std::string address;
  std::string name;
};

/** Interface for building and sending an email. */
class Builder {
public:
  /** Sets the subject of the email. */
  virtual Builder &SetSubject(absl::string_view subject) = 0;

  /** Sets the body of the email. */
  virtual Builder &SetBody(absl::string_view body) = 0;

  /** Sets the sender of the email. The name is optional. */
  virtual Builder &SetSender(absl::string_view address,
                             absl::string_view name = "") = 0;

  /** Adds a recipient of the email. The recipient's name is optional. By
   * default, a recipient will be the primary. For cc or bcc, specify a
   * RecipientType of CarbonCopy or Blind. */
  virtual Builder &AddRecipient(absl::string_view address,
                                absl::string_view name = "",
                                RecipientType recipient_type = Primary) = 0;

  /** Sets the content type of the email. Leave unset for plain text. */
  virtual Builder &SetContentType(absl::string_view content_type) = 0;

  /** Connects to the receiving server, sends the email, and disconnects. */
  virtual absl::Status Send() = 0;

  /** Sends the email over an existing connection. See Smtp class docs below
   * for usage. */
  virtual absl::Status SendBatch() = 0;
};

/** Concrete implementation of Builder. */
class BuilderImpl : public Builder {
public:
  BuilderImpl(SmtpAdapter &adapter, absl::string_view username,
              absl::string_view password)
      : username_(std::string(username)), password_(std::string(password)),
        adapter_(adapter) {}

  BuilderImpl &SetSubject(absl::string_view subject) override {
    subject_ = subject;
    return *this;
  }

  BuilderImpl &SetBody(absl::string_view body) override {
    body_ = body;
    return *this;
  }

  BuilderImpl &SetSender(absl::string_view address,
                         absl::string_view name = "") override {
    sender_.address = address;
    sender_.name = name;
    return *this;
  }

  BuilderImpl &SetContentType(absl::string_view content_type) override {
    content_type_ = content_type;
    return *this;
  }

  BuilderImpl &AddRecipient(absl::string_view address,
                            absl::string_view name = "",
                            RecipientType recipient_type = Primary) override {
    Recipient &recipient = recipients_.emplace_back(Recipient());
    recipient.address = address;
    recipient.name = name;
    recipient.recipient_type = recipient_type;
    return *this;
  }

  absl::Status Send() override;

  absl::Status SendBatch() override;

  BuilderImpl &Reset() {
    subject_ = "";
    body_ = "";
    sender_.address = "";
    sender_.name = "";
    content_type_ = "";
    recipients_.clear();
    return *this;
  }

private:
  std::vector<Recipient> recipients_;
  Sender sender_;
  std::string subject_;
  std::string body_;
  std::string content_type_;
  std::string username_;
  std::string password_;
  SmtpAdapter &adapter_;
};

/** Sends email(s) using SMTP.
 *
 *  Example construction:
 *
 *   SmtpAdapterImpl adapter("smtp.sendgrid.net", 587)
 *   Smtp smtp("username", "password", adapter);
 *
 *  Example usage - sending a single email:
 *
 *    auto status = smtp.NewEmail()
 *                  .SetSender("someone@gmail.com")
 *                  .AddRecipient("someone@gmail.com")
 *                  .AddRecipient("someone@gmail.com", "John Smith", CarbonCopy)
 *                  .AddRecipient("someone@gmail.com", "Jane Smith", Blind)
 *                  .SetSubject("Subject")
 *                  .SetBody("Body")
 *                  .Send();
 *    if (!status.ok()) {
 *      std::cerr << status << std::endl;
 *    }
 *
 * The above usage may be repeated with the same Smtp instance to send multiple
 * emails, however it will connect and disconnect from the receiving server with
 * each email. If you need to send multiple emails at once, use the batch APIs
 * (Connect, SendBatch, Disconnect), e.g.:
 *
 *   absl::Status SendBatch(Smtp &smtp) {
 *     RETURN_IF_ERROR(smtp.Connect());
 *     RETURN_IF_ERROR(smtp.NewEmail()
 *                         .SetSender("someone@gmail.com")
 *                         .AddRecipient("someone@gmail.com")
 *                         .SetSubject("Hello")
 *                         .SetBody("This is an example.")
 *                         .SendBatch());
 *     RETURN_IF_ERROR(smtp.NewEmail()
 *                         .SetSender("someone@gmail.com")
 *                         .AddRecipient("someone@gmail.com")
 *                         .SetSubject("Hello again")
 *                         .SetBody("This is the second example")
 *                         .SendBatch());
 *     return smtp.Disconnect();
 *   }
 */
class Smtp {
public:
  Smtp(absl::string_view username, absl::string_view password,
       SmtpAdapter &adapter)
      : username_(std::string(username)), password_(std::string(password)),
        adapter_(adapter), builder_(adapter_, username_, password) {}

  /** Returns a Builder that may be used to construct and send an email. */
  Builder &NewEmail() { return builder_.Reset(); }

  /** Connects and authorizes with the SMTP server. Only call this in batch
   * mode prior to sending emails. If there is a failure, it will
   * automatically disconnect. */
  absl::Status Connect();

  /** Disconnects from the SMTP server. Only use this in batch mode. */
  absl::Status Disconnect();

  /** Enables logging to stdout which will show the SMTP traffic, intended
   * only for debugging. */
  void EnableLogging() { adapter_.EnableLogging(); }

private:
  std::string username_;
  std::string password_;
  SmtpAdapter &adapter_;
  BuilderImpl builder_;
};

} // namespace smtp
} // namespace ez

#endif // EZ_SMTP_H