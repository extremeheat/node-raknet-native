#pragma once
#include <thread>
#include <ctype.h>
#include <stdio.h>
#include "napi.h"
#include "RakPeerInterface.h"

// Holds the context for our RakNet thread
struct TsfnContext {
    TsfnContext(Napi::Env env) {};
    bool running = true;
    RakNet::RakPeerInterface* rakPeer = nullptr;
    std::thread nativeThread;
    Napi::ThreadSafeFunction tsfn;
};

// Called when our RakNet thread needs to close
void FinalizerCallback(Napi::Env env, void* finalizeData, TsfnContext* context);

// Hex tools

unsigned char GetPacketIdentifier2(RakNet::Packet* p); // get first byte of packet
void hexdump(void* ptr, int buflen); // dump packet