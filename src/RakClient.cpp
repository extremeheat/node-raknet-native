#ifndef NAPI_VERSION // msvs
#define NAPI_VERSION 6
#endif
#include <napi.h>
#include <thread>
#include <chrono>
#include "RakClient.h"

#include "RakPeerInterface.h"
#include "RakNetDefines.h"
#include "RakNetTypes.h"
#include "MessageIdentifiers.h"
#include "RakSleep.h"
#include "RuntimeVars.h"

//#define printf

// JS Bindings
Napi::Object RakClient::Initialize(Napi::Env& env, Napi::Object& exports) {
    Napi::Function func = DefineClass(env, "RakClient", {
        InstanceMethod("listen", &RakClient::Listen),
        InstanceMethod("connect", &RakClient::Connect),
        InstanceMethod("send", &RakClient::SendEncapsulated),
        InstanceMethod("ping", &RakClient::Ping),
        InstanceMethod("close", &RakClient::Close) 
    });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("RakClient", func);
    return exports;
}

RakClient::RakClient(const Napi::CallbackInfo& info) : Napi::ObjectWrap<RakClient>(info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return;
    } else if (!info[0].IsString() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
        return;
    }

    this->hostname = info[0].As<Napi::String>().Utf8Value();
    this->port = info[1].As<Napi::Number>().Int32Value();
    if (info.Length() == 3) {
        auto connectionType = info[2].As<Napi::String>().Utf8Value();
        if (connectionType == "minecraft") {
            SetRakNetProtocolVersion(10);
        }
    }

    // Validate the hostname + port and save
    if (!this->conAddr.FromStringExplicitPort(this->hostname.c_str(), this->port, 4)) {
        if (!this->conAddr.FromStringExplicitPort(this->hostname.c_str(), this->port, 6)) {
            Napi::Error::New(env, "Invalid connection address " + this->hostname + "/" + std::to_string(this->port)).ThrowAsJavaScriptException();
            return;
        }
    }

    this->Setup();
}

void RakClient::Setup() {
    client = RakNet::RakPeerInterface::GetInstance();
    client->SetOccasionalPing(true);
    client->SetUnreliableTimeout(1000);

    DataStructures::List< RakNet::RakNetSocket2* > sockets;
    client->GetSockets(sockets);
    /*printf("Socket addresses used by RakNet %d : \n", sockets.Size());
    for (unsigned int i = 0; i < sockets.Size(); i++) {
        printf("%i. %s\n", i + 1, sockets[i]->GetBoundAddress().ToString(true));
    }
    printf("\nMy GUID is %s\n", this->rakInterface->GetGuidFromSystemAddress(RakNet::UNASSIGNED_SYSTEM_ADDRESS).ToString());*/

    auto clientPort = 0;
    RakNet::SocketDescriptor socketDescriptor(clientPort, 0);
    socketDescriptor.socketFamily = AF_INET;
    client->Startup(8, &socketDescriptor, 1);
}

void RakClient::RunLoop() {
    // This callback transforms the native addon data (int *data) to JavaScript
    // values. It also receives the treadsafe-function's registered callback, and
    // may choose to call it.
    auto callback = [this](Napi::Env env, Napi::Function jsCallback, RakNet::Packet* data) {
        //hexdump(data->data, data->length);
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
    while (context->running && client->IsActive()) {
        RakSleep(30);
        while (p = client->Receive()) {
            RakNet::Packet* pr = p;
            auto packetIdentifier = GetPacketIdentifier2(p);
            printf("Got packet ID: %d\n", packetIdentifier);
            //hexdump(p->data, p->length);
            auto status = context->tsfn.BlockingCall(pr, callback);
            if (status != napi_ok) {
                fprintf(stderr, "RakClient failed to emit packet to JS: %d\n", status);
            }
        }
    }
    // Release the thread-safe function. This decrements the internal thread
    // count, and will perform finalization since the count will reach 0.
    context->tsfn.Release();
}

Napi::Value RakClient::Listen(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
    auto eventHandler = info[0].As<Napi::Function>();

    // Construct context data
    context = new TsfnContext(env);
    context->rakPeer = client;
    printf("Created ctx\n");
    // Create a new ThreadSafeFunction.
    context->tsfn =
        Napi::ThreadSafeFunction::New(env, // Environment
            eventHandler, // JS function from caller
            "RakClient", // Resource name
            0, // Max queue size (0 = unlimited).
            1, // Initial thread count
            context, // Context,
            FinalizerCallback, // Finalizer
            (void*)nullptr    // Finalizer data
        );
    //printf("tsfn\n");
    context->nativeThread = std::thread(&RakClient::RunLoop, this);
    //printf("All good!\n");
    this->context = context;
    return context->deferred.Promise();
}

void RakClient::Connect(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    auto car = client->Connect(this->hostname.c_str(), this->port, "", 0);
    if (car != RakNet::CONNECTION_ATTEMPT_STARTED) {
        Napi::Error::New(env, "Unable to connect to " + std::to_string(this->port) + " - " + std::to_string(car)).ThrowAsJavaScriptException();
        return;
    }
}

void RakClient::Ping(const Napi::CallbackInfo& info) {
    client->Ping(this->hostname.c_str(), this->port, false);
}

Napi::Value RakClient::SendEncapsulated(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 5) {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    //sendEncapsulated(message: Buffer, priority : PacketPriority, reliability : PacketReliability, orderingChannel : int, broadcast = false) {
    auto buffer = info[0].As<Napi::ArrayBuffer>();
    auto priority = info[1].As<Napi::Number>().Int32Value();
    auto reliability = info[2].As<Napi::Number>().Int32Value();
    auto orderChannel = info[3].As<Napi::Number>().Int32Value();
    bool broadcast = info[4].As<Napi::Boolean>().ToBoolean();

    auto state = client->GetConnectionState(this->conAddr);
    printf("Send con state: %d %d\n", state, buffer.ByteLength());

    if (state != RakNet::IS_CONNECTED) {
        return Napi::Number::New(env, -(int)state);
    }
    //hexdump(buffer.Data(), buffer.ByteLength());
    auto ret = client->Send((char*)buffer.Data(), buffer.ByteLength(), (PacketPriority)priority, (PacketReliability)reliability, (char)orderChannel, this->conAddr, broadcast);
    if (ret == 0)printf("Bad input!\n");
    return Napi::Number::New(env, ret);
}

void RakClient::Close(const Napi::CallbackInfo& info) {
    if (this->client) this->client->Shutdown(600);
}