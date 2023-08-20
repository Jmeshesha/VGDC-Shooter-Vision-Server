#include "Client.h"

#include "client.h"
#include <iostream>
namespace CameraMarkerServer {
    Client::Client() {

    }


    void Client::Run() {
        isRunning = true;

        while (isRunning) {
            std::cout << "Hello!" << std::endl;
        }
    }
}
