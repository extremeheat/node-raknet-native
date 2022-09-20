#ifndef NAPI_VERSION  // msvs
#define NAPI_VERSION 6
#endif
#include "RakServer.h"

#include <napi.h>

#include <chrono>
#include <thread>

#include "MessageIdentifiers.h"
#include "RakNetDefines.h"
#include "RakNetTypes.h"
#include "RakPeerInterface.h"
#include "RakSleep.h"
#include "RuntimeVars.h"

#define printf

Napi::Object RakServer::Initialize(Napi::Env& env, Napi::Object& exports) {
    Napi::Function func =
        DefineClass(env, "RakServer",
                    {InstanceMethod("listen", &RakServer::Listen), InstanceMethod("send", &RakServer::SendEncapsulated),
                     InstanceMethod("kick", &RakServer::Kick), InstanceMethod("close", &RakServer::Close),
                     InstanceMethod("setPongResponse", &RakServer::SetPongResponse)});

    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("RakServer", func);
    return exports;
}

RakServer::RakServer(const Napi::CallbackInfo& info) : Napi::ObjectWrap<RakServer>(info) {
    Napi::Env env = info.Env();
    if (info.Length() < 3) {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return;
    } else if (!info[0].IsString() || !info[1].IsNumber() || !info[2].IsObject()) {
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
        return;
    }

    this->hostname = info[0].As<Napi::String>().Utf8Value();
    this->port = info[1].As<Napi::Number>().Int32Value();
    auto options = info[2].As<Napi::Object>();

    if (options.Has("maxConnections")) this->options.maxConnections = options.Get("maxConnections").As<Napi::Number>();
    if (options.Has("protocolVersion")) {
        auto protocolVersion = options.Get("protocolVersion").As<Napi::Number>().Int32Value();
        this->protocolVersion = protocolVersion;
    }

    server = RakNet::RakPeerInterface::GetInstance();
}

void RakServer::RunLoop() {
    auto client = ctx->rakPeer;
    // This callback transforms the native addon data (int *data) to JavaScript
    // values. It also receives the treadsafe-function's registered callback, and
    // may choose to call it.
    auto callback = [&](Napi::Env env, Napi::Function jsCallback, void* datasPtr) {
        Napi::Array packets = Napi::Array::New(env, packet_queue.size());
        packetMutex.lock();
        for (int i = 0; packet_queue.size() > 0; i++) {
            JSPacket* data = packet_queue.front();
            packet_queue.pop();
            Napi::Array fields = Napi::Array::New(env, 3);
            int j = 0;
            fields[j++] = Napi::ArrayBuffer::New(env, data->data, data->length, &FreeBuf, data);
            fields[j++] = Napi::String::From(env, data->systemAddress.ToString(true, '/'));
            fields[j++] = Napi::String::From(env, data->guid.ToString());
            packets[i] = fields;
        }
        packetMutex.unlock();
        jsCallback.Call({packets});
    };

    // Holds packets
    RakNet::Packet* p = 0;
    RakNet::SystemAddress clientID;
    while (ctx->running && client->IsActive()) {
        RakSleep(50);
        while (p = client->Receive()) {
            packetMutex.lock();
            auto jsp = CreateJSPacket(p);
            this->packet_queue.push(jsp);
            client->DeallocatePacket(p);
            packetMutex.unlock();
        }
        if (packet_queue.size()) {
            if (!ctx->running) {
                packetMutex.lock();
                for (int i = 0; packet_queue.size() > 0; i++) {
                    JSPacket* data = packet_queue.front();
                    packet_queue.pop();
                    FreeJSPacket(data);
                }
                packetMutex.unlock();
                break;
            }
            auto status = ctx->tsfn.NonBlockingCall(&this->packet_queue, callback);
            if (status != napi_ok) {
                fprintf(stderr, "RakServer failed to emit packet to JS: %d\n", status);
            }
        }
    }
    printf("server RELEASING\n");
    // Release the thread-safe function. This decrements the internal thread
    // count, and will perform finalization since the count will reach 0.
    ctx->tsfn.Release();
}

Napi::Value RakServer::Listen(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
    auto eventHandler = info[0].As<Napi::Function>();

    server->SetTimeoutTime(30000, RakNet::UNASSIGNED_SYSTEM_ADDRESS);
    server->SetMaximumIncomingConnections(this->options.maxConnections);
    server->allowClientsWithOlderVersion = true;
    if (this->protocolVersion != -1) server->SetProtocolVersion(this->protocolVersion);

    DataStructures::List<RakNet::RakNetSocket2*> sockets;
    server->GetSockets(sockets);
    printf("Socket addresses used by RakNet %d : \n", sockets.Size());
    for (unsigned int i = 0; i < sockets.Size(); i++) {
        printf("%i. %s\n", i + 1, sockets[i]->GetBoundAddress().ToString(true));
    }
    printf("\nMy GUID is %s\n", server->GetGuidFromSystemAddress(RakNet::UNASSIGNED_SYSTEM_ADDRESS).ToString());

    RakNet::SocketDescriptor socketDescriptors[2];
    socketDescriptors[0] = RakNet::SocketDescriptor(this->port, this->hostname.size() ? this->hostname.c_str() : 0);
    socketDescriptors[1].port = this->port;
    socketDescriptors[1].socketFamily = AF_INET6;
    // TODO: fix ipv6
    bool b = server->Startup(this->options.maxConnections, socketDescriptors, 2) == RakNet::RAKNET_STARTED;
    if (!b) {
        printf("Failed to start dual IPV4 and IPV6 ports. Trying IPV4 only.\n");

        // Try again, but leave out IPV6
        b = server->Startup(this->options.maxConnections, socketDescriptors, 1) == RakNet::RAKNET_STARTED;
        if (!b) {
            Napi::TypeError::New(env, "Server failed to start").ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }
    }

    // Construct context data
    ctx = new TsfnContext(env);
    ctx->rakPeer = server;
    printf("Server Created ctx\n");
    // Create a new ThreadSafeFunction.
    ctx->tsfn = Napi::ThreadSafeFunction::New(env,                // Environment
                                              eventHandler,       // JS function from caller
                                              "RakServer",        // Resource name
                                              0,                  // Max queue size (0 = unlimited).
                                              1,                  // Initial thread count
                                              ctx,                // Context,
                                              FinalizerCallback,  // Finalizer
                                              (void*)nullptr      // Finalizer data
    );
    // printf("tsfn\n");
    ctx->nativeThread = std::thread(&RakServer::RunLoop, this);
    printf("server All good!\n");

    // auto refCount = this->Ref(); // Force increment the ref count to avoid gc
    // printf("Ref count:%d\n", refCount);
    return ctx->deferred.Promise();
}

Napi::Value RakServer::SendEncapsulated(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 5) {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    auto clientAddress = info[0].As<Napi::String>().Utf8Value();
    auto clientPort = info[1].As<Napi::Number>().Int32Value();
    auto buffer = info[2].As<Napi::ArrayBuffer>();
    auto priority = info[3].As<Napi::Number>().Int32Value();
    auto reliability = info[4].As<Napi::Number>().Int32Value();
    auto orderChannel = info[5].As<Napi::Number>().Int32Value();
    bool broadcast = info[6].As<Napi::Boolean>().ToBoolean();

    RakNet::SystemAddress addr;
    if (!addr.FromStringExplicitPort(clientAddress.c_str(), clientPort, 4)) {
        if (!addr.FromStringExplicitPort(clientAddress.c_str(), clientPort, 6)) {
            Napi::RangeError::New(env,
                                  "Invalid connection address " + this->hostname + "/" + std::to_string(this->port))
                .ThrowAsJavaScriptException();
            return Napi::Number::New(env, 0);
        }
    }

    auto state = server->GetConnectionState(addr);

    if (state != RakNet::IS_CONNECTED) {
        return Napi::Number::New(env, -(int)state);
    }

    auto ret = server->Send((char*)buffer.Data(), buffer.ByteLength(), (PacketPriority)priority,
                            (PacketReliability)reliability, (char)orderChannel, addr, broadcast);
    return Napi::Number::New(env, ret);
}

void RakServer::SetPongResponse(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !server) {
        Napi::TypeError::New(env, "Wrong number of arguments or needs init").ThrowAsJavaScriptException();
        return;
    }

    auto buffer = info[0].As<Napi::Buffer<char>>();
    server->SetOfflinePingResponse((const char*)buffer.Data(), buffer.ByteLength());
}

void RakServer::Kick(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return;
    }

    auto clientGuid = info[0].As<Napi::String>().Utf8Value();
    auto silent = info[1].As<Napi::Boolean>().ToBoolean();

    RakNet::RakNetGUID clientAddress;
    clientAddress.FromString(clientGuid.c_str());
    server->CloseConnection(clientAddress, !silent);
}

void RakServer::Close(const Napi::CallbackInfo& info) {
    if (ctx) ctx->running = false;
    if (server) server->Shutdown(300);
}