#pragma once

#include <napi.h>

#include <queue>

#include "RakPeerInterface.h"
#include "Util.h"

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
    TsfnContext* ctx = nullptr;
    std::queue<JSPacket*> packet_queue;
    std::mutex packetMutex;
    int protocolVersion = -1;

   public:
    static Napi::Object Initialize(Napi::Env& env, Napi::Object& target);
    // Constructor
    RakServer(const Napi::CallbackInfo& info);
    // RAKNET LOOP
    void RunLoop();
    // Start connection
    Napi::Value Listen(const Napi::CallbackInfo& info);
    // Send an Encapsulated raknet packet
    Napi::Value SendEncapsulated(const Napi::CallbackInfo& info);

    void SetPongResponse(const Napi::CallbackInfo& info);

    // Napi::Value GetConnectionsCount(const Napi::CallbackInfo& info);

    void Kick(const Napi::CallbackInfo& info);
    void Close(const Napi::CallbackInfo& info);
    // Called by garbage collector, we don't have to worry about it
    ~RakServer() {
        // printf("raknet server freeing\n");
        if (server) {
            server->Shutdown(500);
            RakNet::RakPeerInterface::DestroyInstance(server);
            server = nullptr;
        }
    }
};
