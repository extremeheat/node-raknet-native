#ifndef NAPI_VERSION  // msvs
#define NAPI_VERSION 6
#endif
#include "RakClient.h"

#include <napi.h>

#include <chrono>
#include <mutex>  // std::mutex
#include <thread>
#include <vector>

#include "MessageIdentifiers.h"
#include "RakNetDefines.h"
#include "RakNetTypes.h"
#include "RakPeerInterface.h"
#include "RakSleep.h"
#include "RuntimeVars.h"

// JS Bindings
Napi::Object RakClient::Initialize(Napi::Env& env, Napi::Object& exports) {
    Napi::Function func =
        DefineClass(env, "RakClient",
                    {InstanceMethod("listen", &RakClient::Listen), InstanceMethod("connect", &RakClient::Connect),
                     InstanceMethod("send", &RakClient::SendEncapsulated), InstanceMethod("ping", &RakClient::Ping),  //
                     InstanceMethod("close", &RakClient::Close)});

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
    auto options = info[2].As<Napi::Object>();
    if (options.Has("protocolVersion")) {
        auto protocolVersion = options.Get("protocolVersion").As<Napi::Number>().Int32Value();
        this->protocolVersion = protocolVersion;
    }

    // Validate the hostname + port and save
    if (!this->conAddr.FromStringExplicitPort(this->hostname.c_str(), this->port, 4)) {
        if (!this->conAddr.FromStringExplicitPort(this->hostname.c_str(), this->port, 6)) {
            Napi::Error::New(env, "Invalid connection address " + this->hostname + "/" + std::to_string(this->port))
                .ThrowAsJavaScriptException();
            return;
        }
    }

    this->Setup();
}

void RakClient::Setup() {
    client = RakNet::RakPeerInterface::GetInstance();
    client->SetOccasionalPing(true);
    client->SetUnreliableTimeout(1000);
    if (this->protocolVersion != -1) client->SetProtocolVersion(this->protocolVersion);

    DataStructures::List<RakNet::RakNetSocket2*> sockets;
    client->GetSockets(sockets);

    auto clientPort = 0;
    RakNet::SocketDescriptor socketDescriptor(clientPort, 0);
    socketDescriptor.socketFamily = AF_INET;
    client->Startup(8, &socketDescriptor, 1);
}

void RakClient::RunLoop() {
    // This callback transforms the native addon data (int *data) to JavaScript
    // values. It also receives the treadsafe-function's registered callback, and
    // may choose to call it.
    auto callback = [this](Napi::Env env, Napi::Function jsCallback, void* datasPtr) {
        Napi::Array packets = Napi::Array::New(env, packet_queue.size());
        packetMutex.lock();
        RakNet::SystemAddress systemAddress;
        RakNet::AddressOrGUID guid;
        for (int i = 0; packet_queue.size() > 0; i++) {
            JSPacket* data = packet_queue.front();
            packet_queue.pop();
            packets[i] = Napi::ArrayBuffer::New(env, data->data, data->length, FreeBuf, data);
            systemAddress = data->systemAddress;
            guid = data->guid;
        }
        packetMutex.unlock();

        jsCallback.Call({packets, Napi::String::From(env, systemAddress.ToString(true, '/')),
                         Napi::String::From(env, guid.ToString())});
    };

    // Holds packets
    RakNet::Packet* p = 0;
    RakNet::SystemAddress clientID;
    while (context->running && client->IsActive()) {
        RakSleep(30);
        while (context->running, p = client->Receive()) {
            packetMutex.lock();
            auto jsp = CreateJSPacket(p);
            client->DeallocatePacket(p);
            packet_queue.push(jsp);
            packetMutex.unlock();
        }
        if (packet_queue.size()) {
            if (!context->running) {
                packetMutex.lock();
                for (int i = 0; packet_queue.size() > 0; i++) {
                    JSPacket* data = packet_queue.front();
                    packet_queue.pop();
                    FreeJSPacket(data);
                }
                packetMutex.unlock();
                break;
            }

            auto status = context->tsfn.NonBlockingCall(&this->packet_queue, callback);
            if (status != napi_ok) {
                fprintf(stderr, "RakClient failed to emit packet to JS: %d\n", status);
            }
        }
    }

    // Release the thread-safe function. This decrements the internal thread
    // count, and will perform finalization since the count will reach 0.
    // auto refCount = this->Ref();  // Force increment the ref count to avoid gc
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
    // Create a new ThreadSafeFunction.
    context->tsfn = Napi::ThreadSafeFunction::New(
        env,           // Environment
        eventHandler,  // JS function from caller
        "RakClient",   // Resource name
        0,             // Max queue size (0 = unlimited).
        1,             // Initial thread count
        context,       // Context,
        [](Napi::Env env, RakClient* thiz, TsfnContext* context) {
            context->running = false;
            // Close the RakNet client
            thiz->Close();
            // Join the thread
            context->nativeThread.join();
            // Resolve the Promise previously returned to JS via the CreateTSFN method.
            context->deferred.Resolve(Napi::Boolean::New(env, true));
            delete context;
        },    // Finalizer
        this  // Finalizer data
    );
    context->nativeThread = std::thread(&RakClient::RunLoop, this);
    return context->deferred.Promise();
}

void RakClient::Connect(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    auto car = client->Connect(this->hostname.c_str(), this->port, "", 0);
    if (car != RakNet::CONNECTION_ATTEMPT_STARTED) {
        Napi::Error::New(env, "Unable to connect to " + std::to_string(this->port) + " - " + std::to_string(car))
            .ThrowAsJavaScriptException();
        return;
    }
}

void RakClient::Ping(const Napi::CallbackInfo& info) { client->Ping(this->hostname.c_str(), this->port, false); }

Napi::Value RakClient::SendEncapsulated(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 5) {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    auto buffer = info[0].As<Napi::ArrayBuffer>();
    auto priority = info[1].As<Napi::Number>().Int32Value();
    auto reliability = info[2].As<Napi::Number>().Int32Value();
    auto orderChannel = info[3].As<Napi::Number>().Int32Value();
    bool broadcast = info[4].As<Napi::Boolean>().ToBoolean();

    auto state = client->GetConnectionState(this->conAddr);

    if (state != RakNet::IS_CONNECTED) {
        return Napi::Number::New(env, -(int)state);
    }
    auto ret = client->Send((char*)buffer.Data(), buffer.ByteLength(), (PacketPriority)priority,
                            (PacketReliability)reliability, (char)orderChannel, this->conAddr, broadcast);
    return Napi::Number::New(env, ret);
}

void RakClient::Close() {
    if (this->context) context->running = false;
    if (this->client) this->client->Shutdown(300);
}

void RakClient::Close(const Napi::CallbackInfo& info) { Close(); }