#pragma once

#include <napi.h>
#include "RakPeerInterface.h"

struct ServerOptions {
    int maxConnections = 4;

    std::string serverMessage;
};

class RakServer : public Napi::ObjectWrap<RakServer> {
private:
    RakNet::RakPeerInterface* server = nullptr;
    Napi::Function connectionCallback;
    Napi::Function packetCallback;
    std::string hostname;
    int port = -1;
    RakNet::SystemAddress conAddr;

    ServerOptions options;
public:
    static Napi::Object Initialize(Napi::Env& env, Napi::Object& target);
    // Constructor
    RakServer(const Napi::CallbackInfo& info);
    // Start connection
    void Listen(const Napi::CallbackInfo& info);
    // Send an Encapsulated raknet packet
    Napi::Value SendEncapsulated(const Napi::CallbackInfo& info);

    //Napi::Value GetConnectionsCount(const Napi::CallbackInfo& info);

    //void Kick(const Napi::CallbackInfo& info);
    void Close(const Napi::CallbackInfo& info);
    // Called by garbage collector, we don't have to worry about it
    ~RakServer() {
        if (server) {
            server->Shutdown(300);
            RakNet::RakPeerInterface::DestroyInstance(server);
        }
    }
};
