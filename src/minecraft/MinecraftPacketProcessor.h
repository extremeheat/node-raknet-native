#pragma once

#include <napi.h>
#include "GCMHelper.h"
#include "MinecraftPacketHelper.h"

class MinecraftHelper : public Napi::ObjectWrap<MinecraftHelper> {
private:
    GCMHelper gcm;
    MinecraftPacketHelper helper;
    int mode = 0;
public:
    MinecraftHelper(const Napi::CallbackInfo& info) : Napi::ObjectWrap<MinecraftHelper>(info) { }

    static Napi::Object Initialize(Napi::Env& env, Napi::Object& target);
    void CreateCipher(const Napi::CallbackInfo& info);
    Napi::Value Cipher(const Napi::CallbackInfo& info);
    Napi::Value Decipher(const Napi::CallbackInfo& info);
};