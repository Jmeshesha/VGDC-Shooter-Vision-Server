#ifndef UDP_SERVER_
#define UDP_SERVER_

#include <asio/ip/udp.hpp>
#include <asio/io_service.hpp>
#include <memory>

namespace CameraMarkerServer {

	using asio::ip::udp;

class UDPClient {
 public:
  UDPClient(asio::io_service& io_service)
      : io_service_(io_service),
        is_connected_(false),
        socket_(io_service_, udp::endpoint(udp::v4(), 0)) {}

  bool OpenConnection(const std::string& host, const std::string& port);

  bool CloseConnection();

  ~UDPClient();

  bool Send(const std::string& msg);

 private:
  bool is_connected_;
  asio::io_service& io_service_;
  udp::socket socket_;
  udp::endpoint endpoint_;
};

}
#endif // UDP_SERVER_