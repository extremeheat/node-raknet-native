#ifndef NAPI_VERSION // msvs
#define NAPI_VERSION 6
#endif
#include <napi.h>
#include <thread>
#include <chrono>
#include "RakServer.h"

#include "RakPeerInterface.h"
#include "RakNetDefines.h"
#include "RakNetTypes.h"
#include "MessageIdentifiers.h"
#include "RakSleep.h"
#include "RuntimeVars.h"

#include "Util.h"

void RakServerLoop(TsfnContext* ctx) {
    auto client = ctx->rakPeer;
    // This callback transforms the native addon data (int *data) to JavaScript
    // values. It also receives the treadsafe-function's registered callback, and
    // may choose to call it.
    auto callback = [client](Napi::Env env, Napi::Function jsCallback, RakNet::Packet* data) {
        auto buffer = Napi::ArrayBuffer::New(env, data->data, data->length);
        auto addr = Napi::String::From(env, data->systemAddress.ToString(true, '/'));
        jsCallback.Call({
            Napi::ArrayBuffer::New(env, data->data, data->length),
            Napi::String::From(env, data->systemAddress.ToString(true, '/')),
            Napi::String::From(env, data->guid.ToString())
        });
        client->DeallocatePacket(data);
    };

    // Holds packets
    RakNet::Packet* p = 0;
    RakNet::SystemAddress clientID;
    while (ctx->running) {
        RakSleep(30);
        for (p = client->Receive(); p; p = client->Receive()) {
            // We got a packet, get the identifier with our handy function
            auto packetIdentifier = GetPacketIdentifier2(p);
            printf("Got packet ID: %d\n", packetIdentifier);

            auto status = ctx->tsfn.BlockingCall(p, callback);
            if (status != napi_ok) {
                fprintf(stderr, "RakClient failed to emit packet to JS: %d\n", status);
            }
        }
    }
}

Napi::Object RakServer::Initialize(Napi::Env& env, Napi::Object& exports) {
    Napi::Function func = DefineClass(env, "RakServer", {
        InstanceMethod("listen", &RakServer::Listen),
        InstanceMethod("send", &RakServer::SendEncapsulated),
        InstanceMethod("close", &RakServer::Close)
    });

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
    if (options.Has("minecraft")) {
        auto mc = options.Get("minecraft").As<Napi::Object>();
        SetRakNetProtocolVersion(10);
        this->options.serverMessage = mc.Get("message").As<Napi::String>();
    }
}

void RakServer::Listen(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return;
    }
    auto eventHandler = info[0].As<Napi::Function>();

    server = RakNet::RakPeerInterface::GetInstance();
    server->SetTimeoutTime(30000, RakNet::UNASSIGNED_SYSTEM_ADDRESS);
    server->SetMaximumIncomingConnections(this->options.maxConnections);
    server->SetOfflinePingResponse(this->options.serverMessage.c_str(), this->options.serverMessage.length());

    DataStructures::List< RakNet::RakNetSocket2* > sockets;
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
    //TODO: fix ipv6
    bool b = server->Startup(4, socketDescriptors, 2) == RakNet::RAKNET_STARTED;
    server->SetMaximumIncomingConnections(4);
    if (!b) {
        printf("Failed to start dual IPV4 and IPV6 ports. Trying IPV4 only.\n");

        // Try again, but leave out IPV6
        b = server->Startup(4, socketDescriptors, 1) == RakNet::RAKNET_STARTED;
        if (!b) {
            puts("Server failed to start.  Terminating.");
            exit(1);
        }
    }

    // Construct context data
    auto context = new TsfnContext(env);
    context->rakPeer = server;
    printf("Created ctx\n");
    // Create a new ThreadSafeFunction.
    context->tsfn =
        Napi::ThreadSafeFunction::New(env, // Environment
            eventHandler, // JS function from caller
            "RakServer", // Resource name
            0, // Max queue size (0 = unlimited).
            1, // Initial thread count
            context, // Context,
            FinalizerCallback, // Finalizer
            (void*)nullptr    // Finalizer data
        );
    //printf("tsfn\n");
    context->nativeThread = std::thread(RakServerLoop, context);
    //printf("All good!\n");
    return;
}

Napi::Value RakServer::SendEncapsulated(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 5) {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    //sendEncapsulated(message: Buffer, priority : PacketPriority, reliability : PacketReliability, orderingChannel : int, broadcast = false) {
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
            Napi::RangeError::New(env, "Invalid connection address " + this->hostname + "/" + std::to_string(this->port)).ThrowAsJavaScriptException();
            return Napi::Number::New(env, 0);
        }
    }

    auto state = server->GetConnectionState(addr);
    printf("CURRENT CONNECTION STATE: %d\n", state);

    if (state != RakNet::IS_CONNECTED) {
        return Napi::Number::New(env, -(int)state);
    }

    auto ret = server->Send((char*)buffer.Data(), buffer.ByteLength(), (PacketPriority)priority, (PacketReliability)reliability, (char)orderChannel, addr, broadcast);
    return Napi::Number::New(env, ret);
}

void RakServer::Close(const Napi::CallbackInfo& info) {
    std::this_thread::sleep_for(std::chrono::seconds(200));
    auto cb = info[0].As<Napi::Function>();

    cb.Call(info.This(), { Napi::String::New(info.Env(), "Hello world!") });
}