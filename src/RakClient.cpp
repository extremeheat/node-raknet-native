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

// hexdump
#include <ctype.h>
#include <stdio.h>

void hexdump(void* ptr, int buflen) {
    unsigned char* buf = (unsigned char*)ptr;
    int i, j;
    for (i = 0; i < buflen; i += 16) {
        printf("%06x: ", i);
        for (j = 0; j < 16; j++)
            if (i + j < buflen)
                printf("%02x ", buf[i + j]);
            else
                printf("   ");
        printf(" ");
        for (j = 0; j < 16; j++)
            if (i + j < buflen)
                printf("%c", isprint(buf[i + j]) ? buf[i + j] : '.');
        printf("\n");
    }
}
// end hexdump

// RakNet thread
struct TsfnContext {
    TsfnContext(Napi::Env env) {};
    // Native Promise returned to JavaScript
    //Napi::Promise::Deferred deferred;
    // Native thread
    bool running = true;
    RakNet::RakPeerInterface* rakPeer = nullptr;
    std::thread nativeThread;
    Napi::ThreadSafeFunction tsfn;
};

void FinalizerCallback(Napi::Env env, void* finalizeData, TsfnContext* context) {
    context->running = false;
    // Join the thread
    context->nativeThread.join();
    delete context;
}

unsigned char GetPacketIdentifier2(RakNet::Packet* p) {
    if (p == 0) return 255;

    if ((unsigned char)p->data[0] == ID_TIMESTAMP) {
        //assert(p->length > sizeof(RakNet::MessageID) + sizeof(RakNet::Time));
        return (unsigned char)p->data[sizeof(RakNet::MessageID) + sizeof(RakNet::Time)];
    } else {
        return (unsigned char)p->data[0];
    }
}

void RakClientLoop(TsfnContext *ctx) {

    // This callback transforms the native addon data (int *data) to JavaScript
    // values. It also receives the treadsafe-function's registered callback, and
    // may choose to call it.
    auto callback = [](Napi::Env env, Napi::Function jsCallback, RakNet::Packet* data) {
        auto buffer = Napi::ArrayBuffer::New(env, data->data, data->length);
        auto addr = Napi::String::From(env, data->systemAddress.ToString(true, '/'));
        jsCallback.Call({
            Napi::ArrayBuffer::New(env, data->data, data->length),
            Napi::String::From(env, data->systemAddress.ToString(true, '/')),
            Napi::String::From(env, data->guid.ToString())
        });

        /*jsCallback.Call({
            Napi::Number::New(env, 22)
        });*/
    };

    auto client = ctx->rakPeer;
    unsigned char packetIdentifier;
    // Holds packets
    RakNet::Packet* p = 0;
    RakNet::SystemAddress clientID ;
    while (ctx->running) {
        RakSleep(30);
        for (p = client->Receive(); p; client->DeallocatePacket(p), p = client->Receive()) {
            // We got a packet, get the identifier with our handy function
            packetIdentifier = GetPacketIdentifier2(p);
            printf("Got packet ID: %d\n", packetIdentifier);

            auto status = ctx->tsfn.BlockingCall(p, callback);
            return;
            // Check if this is a network message packet
            switch (packetIdentifier)
            {
            case ID_DISCONNECTION_NOTIFICATION:
                // Connection lost normally
                printf("ID_DISCONNECTION_NOTIFICATION\n");
                break;
            case ID_ALREADY_CONNECTED:
                // Connection lost normally
                printf("ID_ALREADY_CONNECTED with guid %" PRINTF_64_BIT_MODIFIER "u\n", p->guid);
                break;
            case ID_INCOMPATIBLE_PROTOCOL_VERSION:
                printf("ID_INCOMPATIBLE_PROTOCOL_VERSION\n");
                break;
            case ID_REMOTE_DISCONNECTION_NOTIFICATION: // Server telling the clients of another client disconnecting gracefully.  You can manually broadcast this in a peer to peer enviroment if you want.
                printf("ID_REMOTE_DISCONNECTION_NOTIFICATION\n");
                break;
            case ID_REMOTE_CONNECTION_LOST: // Server telling the clients of another client disconnecting forcefully.  You can manually broadcast this in a peer to peer enviroment if you want.
                printf("ID_REMOTE_CONNECTION_LOST\n");
                break;
            case ID_REMOTE_NEW_INCOMING_CONNECTION: // Server telling the clients of another client connecting.  You can manually broadcast this in a peer to peer enviroment if you want.
                printf("ID_REMOTE_NEW_INCOMING_CONNECTION\n");
                break;
            case ID_CONNECTION_BANNED: // Banned from this server
                printf("We are banned from this server.\n");
                break;
            case ID_CONNECTION_ATTEMPT_FAILED:
                printf("Connection attempt failed\n");
                break;
            case ID_NO_FREE_INCOMING_CONNECTIONS:
                // Sorry, the server is full.  I don't do anything here but
                // A real app should tell the user
                printf("ID_NO_FREE_INCOMING_CONNECTIONS\n");
                break;

            case ID_INVALID_PASSWORD:
                printf("ID_INVALID_PASSWORD\n");
                break;

            case ID_CONNECTION_LOST:
                // Couldn't deliver a reliable packet - i.e. the other system was abnormally
                // terminated
                printf("ID_CONNECTION_LOST\n");
                break;

            case ID_CONNECTION_REQUEST_ACCEPTED:
                // This tells the client they have connected
                printf("ID_CONNECTION_REQUEST_ACCEPTED to %s with GUID %s\n", p->systemAddress.ToString(true), p->guid.ToString());
                printf("My external address is %s\n", client->GetExternalID(p->systemAddress).ToString(true));
                break;
            case ID_CONNECTED_PING:
            case ID_UNCONNECTED_PING:
                printf("Ping from %s\n", p->systemAddress.ToString(true));
                break;
            default:
                // It's a client, so just show the message
                //printf("%s\n", p->data);
                hexdump(p->data, p->length);
                break;
            }
        }
    }
}

// JS Bindings
Napi::Object RakClient::Initialize(Napi::Env& env, Napi::Object& exports) {
    Napi::Function func = DefineClass(env, "RakClient", { 
        InstanceMethod("connect", &RakClient::Connect),
        InstanceMethod("send", &RakClient::SendEncapsulated),
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
    }

    if (!info[0].IsString() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
        return;
    }

    //constructor(hostname: string, port : number, onPacket : (encapsulated) = > void)

    this->hostname = info[0].As<Napi::String>().Utf8Value();
    this->port = info[1].As<Napi::Number>().Int32Value();
    if (info.Length() == 3) {
        auto connectionType = info[2].As<Napi::String>().Utf8Value();
        if (connectionType == "minecraft") {
            SetRakNetProtocolVersion(10);
        }
    }
}

void RakClient::Connect(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return;
    }
    auto eventHandler = info[0].As<Napi::Function>();

    this->rakInterface = RakNet::RakPeerInterface::GetInstance();
    this->rakInterface->SetOccasionalPing(true);
    this->rakInterface->SetUnreliableTimeout(1000);

    /*RakNet::SocketDescriptor socketDescriptors[2];
    socketDescriptors[0].port = this->port;
    socketDescriptors[0].socketFamily = AF_INET; // Test out IPV4
    //TODO: IPv6
    //socketDescriptors[1].port = this->port;
    //socketDescriptors[1].socketFamily = AF_INET6; // Test out IPV6
    int b = (int)this->rakInterface->Startup(4, socketDescriptors, 2);
    if (b != RakNet::RAKNET_STARTED) {
        Napi::Error::New(env, "Unable to bind connection with IPs at " + std::to_string(this->port) + " - " + std::to_string(b)).ThrowAsJavaScriptException();
        return;
    }*/

    DataStructures::List< RakNet::RakNetSocket2* > sockets;
    this->rakInterface->GetSockets(sockets);
    printf("Socket addresses used by RakNet %d : \n", sockets.Size());
    for (unsigned int i = 0; i < sockets.Size(); i++) {
        printf("%i. %s\n", i + 1, sockets[i]->GetBoundAddress().ToString(true));
    }
    printf("\nMy GUID is %s\n", this->rakInterface->GetGuidFromSystemAddress(RakNet::UNASSIGNED_SYSTEM_ADDRESS).ToString());

    auto clientPort = 0;
    RakNet::SocketDescriptor socketDescriptor(clientPort, 0);
    socketDescriptor.socketFamily = AF_INET;
    this->rakInterface->Startup(8, &socketDescriptor, 1);

    // Validate the hostname + port and save
    if (!this->conAddr.FromStringExplicitPort(this->hostname.c_str(), this->port, 4)) {
        if (!this->conAddr.FromStringExplicitPort(this->hostname.c_str(), this->port, 6)) {
            Napi::Error::New(env, "Invalid connection address " + this->hostname + "/" + std::to_string(this->port)).ThrowAsJavaScriptException();
            return;
        }
    }

    auto car = rakInterface->Connect(this->hostname.c_str(), this->port, "", 0);
    if (car != RakNet::CONNECTION_ATTEMPT_STARTED) {
        Napi::Error::New(env, "Unable to connect to " + std::to_string(this->port) + " - " + std::to_string(car)).ThrowAsJavaScriptException();
        return;
    }

    // Construct context data
    auto context = new TsfnContext(env);
    context->rakPeer = this->rakInterface;
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
    printf("tsfn\n");
    context->nativeThread = std::thread(RakClientLoop, context);
    printf("All good!\n");

    return;
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

    auto state = rakInterface->GetConnectionState(this->conAddr);
    printf("CURRENT CONNECTION STATE: %d\n", state);

    if (state != RakNet::IS_CONNECTED) {
        return Napi::Number::New(env, -(int)state);
    }

    auto ret = this->rakInterface->Send((char*)buffer.Data(), buffer.ByteLength(), (PacketPriority)priority, (PacketReliability)reliability, (char)orderChannel, RakNet::UNASSIGNED_SYSTEM_ADDRESS, broadcast);
    return Napi::Number::New(env, ret);
}

void RakClient::Close(const Napi::CallbackInfo& info) {
    std::this_thread::sleep_for(std::chrono::seconds(200));
    auto cb = info[0].As<Napi::Function>();

    cb.Call(info.This(), { Napi::String::New(info.Env(), "Hello world!") });
}