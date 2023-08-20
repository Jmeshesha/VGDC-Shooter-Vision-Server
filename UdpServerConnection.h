#ifndef UDP_SERVER_
#define UDP_SERVER_

#include <asio.hpp>
namespace CameraMarkerServer {
	class UDPServerConnection {
	public:
          UDPServerConnection() : isConnected(false), socket(nullptr) {}

		void SetTarget(std::string address, int port);
		bool ConnectToServer();

		bool DisconnectFromServer();

		bool SendString(std::string bytes);

		~UDPServerConnection();


	private:
		bool isConnected;
		asio::ip::udp::socket *socket;
	};
}
#endif // UDP_SERVER_