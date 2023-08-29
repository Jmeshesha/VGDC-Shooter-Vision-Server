#include "Client.h"

#include <iostream>
#include <asio/io_service.hpp>
#include <chrono>
#include <thread>

namespace CameraMarkerServer {

bool Client::SetupSocket() {}

void Client::Run() {
  isRunning = true;
  asio::io_service io_service;
  UDPClient client(io_service);
  const std::string address = "localhost";
  const std::string port = "7777";
  if (!client.OpenConnection(address, port)) {
    return;
  }
  while (isRunning) {
    client.Send("test");
  }
}
}  // namespace CameraMarkerServer
