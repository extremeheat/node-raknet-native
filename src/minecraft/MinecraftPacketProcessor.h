#pragma once

#include <napi.h>
#include "MinecraftPacketHelper.h"

class MinecraftHelper : public Napi::ObjectWrap<MinecraftHelper> {
private:
    MinecraftPacketHelper helper;
public:
    MinecraftHelper(const Napi::CallbackInfo& info) : Napi::ObjectWrap<MinecraftHelper>(info) { }

    static Napi::Object Initialize(Napi::Env& env, Napi::Object& target);
    void CreateCipher(const Napi::CallbackInfo& info);
    Napi::Value Cipher(const Napi::CallbackInfo& info);
    Napi::Value Decipher(const Napi::CallbackInfo& info);
};