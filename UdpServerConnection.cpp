#include "UdpServerConnection.h"
#include <asio/ip/udp.hpp>
#include <asio/io_service.hpp>
namespace CameraMarkerServer {
using ::asio::ip::udp;

bool UDPClient::OpenConnection(const std::string& host, const std::string& port) {
  if (is_connected_) {
    return false;
  }
  socket_ = udp::socket(io_service_, udp::endpoint(udp::v4(), 0));
  udp::resolver resolver(io_service_);
  udp::resolver::query query(udp::v4(), host, port);
  endpoint_ = *resolver.resolve(query);
  is_connected_ = true;
}
bool UDPClient::CloseConnection() {
  if (!is_connected_) {
    return false;
  }
  socket_.close();
  is_connected_ = false;
}
UDPClient::~UDPClient() { CloseConnection(); }
bool UDPClient::Send(const std::string& msg) {
  size_t bytes_sent = socket_.send_to(asio::buffer(msg, msg.size()), endpoint_);
  return bytes_sent > 0;
}
}  // namespace CameraMarkerServer