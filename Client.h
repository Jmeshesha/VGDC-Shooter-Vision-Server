#ifndef CLIENT_H_
#define CLIENT_H_
#include "CameraDetectorUtils.h"
namespace CameraMarkerServer {
    class Client {
    public:
        Client();
        void Run();

    private:
        bool SetupSocket();

        bool isRunning = false;

    }; // Class Client
}
#endif // CLIENT_H_