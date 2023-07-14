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

#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "status_macros.h"
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/array.hpp>
#include <iostream>

using namespace boost::asio::ip;

namespace ez {
namespace smtp {
namespace {

void WriteRecipient(absl::string_view field, absl::string_view address,
                    absl::string_view name, std::stringstream &output) {
  output << field << ": " << address;
  if (name.size() > 0) {
    output << " <" << name << ">";
  }
  output << "\r\n";
}

void WriteRecipients(const std::vector<Recipient> recipients,
                     absl::string_view field, const int recipient_type,
                     std::stringstream &output) {
  bool started = false;
  for (auto it = recipients.begin(); it != recipients.end(); it++) {
    if (it->recipient_type != recipient_type) {
      continue;
    }
    if (started) {
      output << ", ";
    } else {
      output << field << ": ";
      started = true;
    }
    if (it->name.size() > 0)
      output << it->name << " <" << it->address << ">";
    else
      output << it->address;
    output << "\r\n";
  }
}

std::string Base64Encode(const std::string &data) {
  using namespace boost::archive::iterators;
  typedef base64_from_binary<transform_width<const char *, 6, 8>> base64_text;
  std::string result;
  result.reserve(data.size() * 4 / 3);
  std::copy(base64_text(data.c_str()), base64_text(data.c_str() + data.size()),
            std::back_inserter(result));
  std::string::size_type data_size = data.size();
  while (data_size % 3 != 0) {
    result += '=';
    data_size++;
  }
  return result;
}

absl::Status StartTls(SmtpAdapter &adapter, absl::string_view username,
                      absl::string_view password) {
  RETURN_IF_ERROR(adapter.Read(220));
  RETURN_IF_ERROR(adapter.WriteLine("STARTTLS"));
  RETURN_IF_ERROR(adapter.Read(220));
  RETURN_IF_ERROR(adapter.EnableTls());
  RETURN_IF_ERROR(adapter.WriteLine(absl::StrCat("HELO ", adapter.Hostname())));
  RETURN_IF_ERROR(adapter.Read(250));
  RETURN_IF_ERROR(adapter.WriteLine("AUTH PLAIN"));
  RETURN_IF_ERROR(adapter.Read(334));
  std::string auth_hash = Base64Encode('\000' + std::string(username) + '\000' +
                                       std::string(password));
  RETURN_IF_ERROR(adapter.WriteLine(auth_hash));
  RETURN_IF_ERROR(adapter.Read(235));
  return absl::OkStatus();
}

absl::Status Connect(SmtpAdapter &adapter, absl::string_view username,
                     absl::string_view password) {
  RETURN_IF_ERROR(adapter.Connect());
  auto status = StartTls(adapter, username, password);
  if (!status.ok()) {
    adapter.Disconnect();
  }
  return status;
}

absl::Status Quit(SmtpAdapter &adapter) {
  auto status = adapter.WriteLine("QUIT");
  if (status.ok()) {
    status = adapter.Read(221);
  }
  adapter.Disconnect();
  return status;
}

} // namespace

absl::Status SmtpAdapterImpl::Connect() {
  socket_.reset(new SslSocket(io_service_, ssl_context_));
  tcp::resolver::query query(hostname_, std::to_string(port_));
  tcp::resolver resolver(io_service_);
  tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
  boost::system::error_code error = boost::asio::error::host_not_found;
  tcp::resolver::iterator end;
  while (error && endpoint_iterator != end) {
    socket_->next_layer().close();
    socket_->next_layer().connect(*endpoint_iterator++, error);
  }
  if (error) {
    return absl::UnavailableError(error.message());
  }
  return absl::OkStatus();
}

absl::Status SmtpAdapterImpl::Read(const int expected_return) {
  std::size_t bytes_received;
  boost::array<char, 256> buffer;
  if (enable_tls_)
    bytes_received = socket_->read_some(boost::asio::buffer(buffer));
  else
    bytes_received = socket_->next_layer().receive(boost::asio::buffer(buffer));
  if (bytes_received == 0) {
    return absl::InternalError("The server closed the connection");
  }

  std::string response;
  std::copy(buffer.begin(), buffer.begin() + bytes_received,
            std::back_inserter(response));
  if (log_) {
    std::cout << response;
  }

  int return_value = atoi(response.substr(0, 3).c_str());
  if (return_value != expected_return) {
    return absl::InternalError(absl::StrFormat(
        "Expected status %d, received %d", expected_return, return_value));
  }
  return absl::OkStatus();
}

absl::Status SmtpAdapterImpl::WriteLine(absl::string_view message) {
  boost::system::error_code error;
  const std::string line = std::string(absl::StrCat(message, "\r\n"));
  auto buffer = boost::asio::buffer(line);
  if (enable_tls_)
    socket_->write_some(buffer, error);
  else
    socket_->next_layer().write_some(buffer, error);
  if (log_) {
    std::cout << line;
  }
  return error ? absl::InternalError(error.message()) : absl::OkStatus();
}

absl::Status BuilderImpl::Send() {
  RETURN_IF_ERROR(Connect(adapter_, username_, password_));
  auto status = SendBatch();
  if (!status.ok()) {
    adapter_.Disconnect();
    return status;
  }
  return Quit(adapter_);
}

absl::Status BuilderImpl::SendBatch() {
  RETURN_IF_ERROR(
      adapter_.WriteLine(absl::StrCat("MAIL FROM: <", sender_.address, ">")));
  RETURN_IF_ERROR(adapter_.Read(250));
  for (auto it = recipients_.begin(); it != recipients_.end(); it++) {
    RETURN_IF_ERROR(
        adapter_.WriteLine(absl::StrCat("RCPT TO: <", it->address, ">")));
    RETURN_IF_ERROR(adapter_.Read(250));
  }
  RETURN_IF_ERROR(adapter_.WriteLine("DATA"));
  RETURN_IF_ERROR(adapter_.Read(354));
  std::stringstream data;
  WriteRecipient("From", sender_.address, sender_.name, data);
  WriteRecipients(recipients_, "To", Primary, data);
  WriteRecipients(recipients_, "Cc", CarbonCopy, data);
  WriteRecipients(recipients_, "Bcc", Blind, data);
  if (content_type_ != "") {
    data << "MIME-Version: 1.0\r\n";
    data << "Content-Type: " << content_type_ << "\r\n";
  }
  data << "Subject: " << subject_ << "\r\n\r\n";
  data << body_ << "\r\n.";
  RETURN_IF_ERROR(adapter_.WriteLine(data.str()));
  RETURN_IF_ERROR(adapter_.Read(250));
  return absl::OkStatus();
}

absl::Status Smtp::Connect() {
  return ez::smtp::Connect(adapter_, username_, password_);
}

absl::Status Smtp::Disconnect() { return Quit(adapter_); }

} // namespace smtp
} // namespace ez
