#pragma once

#include <napi.h>
#include "RakPeerInterface.h"
#include "Util.h"

class RakClient : public Napi::ObjectWrap<RakClient> {
private:
    RakNet::RakPeerInterface* client = nullptr;
    Napi::Function connectionCallback;
    Napi::Function packetCallback;
    std::string hostname;
    int port = 0;
    RakNet::SystemAddress conAddr;
    TsfnContext* context = nullptr;
public:
    static Napi::Object Initialize(Napi::Env& env, Napi::Object& target);
    // Constructor
    RakClient(const Napi::CallbackInfo& info);

    void Setup();

    // Start connection
    void Connect(const Napi::CallbackInfo& info);

    // Ping
    void Ping(const Napi::CallbackInfo& info);

    // Send an Encapsulated raknet packet
    Napi::Value SendEncapsulated(const Napi::CallbackInfo& info);
    void Close(const Napi::CallbackInfo& info);
    // Called by garbage collector, we don't have to worry about it
    ~RakClient() {
        if (client) {
            client->Shutdown(300);
            RakNet::RakPeerInterface::DestroyInstance(client);
        }
        delete context;
    }
};