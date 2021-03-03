#include <napi.h>
#include "RakClient.h"

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  return RakClient::Initialize(env, exports);
}

NODE_API_MODULE(raknet, InitAll)