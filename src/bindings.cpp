#include <napi.h>

#include "RakClient.h"
#include "RakServer.h"

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    RakClient::Initialize(env, exports);
    RakServer::Initialize(env, exports);
    return exports;
}

NODE_API_MODULE(raknet, InitAll)