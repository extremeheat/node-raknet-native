#ifndef NAPI_VERSION // msvs
#define NAPI_VERSION 6
#endif
#include <napi.h>
#include <thread>
#include <chrono>
#include <vector>
#include "MinecraftPacketProcessor.h"

void cleanup(Napi::Env env, void* buf) {
    delete[] buf;
}

// JS Bindings
Napi::Object MinecraftHelper::Initialize(Napi::Env& env, Napi::Object& exports) {
    Napi::Function func = DefineClass(env, "MinecraftHelper", {
        InstanceMethod("createCipher", &MinecraftHelper::CreateCipher),
        InstanceMethod("cipher", &MinecraftHelper::Cipher),
        InstanceMethod("decipher", &MinecraftHelper::Decipher)
    });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("MinecraftHelper", func);
    return exports;
}

void MinecraftHelper::CreateCipher(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return;
    }
    auto alg = info[0].As<Napi::String>().Utf8Value();
    auto secret = info[1].As<Napi::ArrayBuffer>();
    auto iv = info[2].As<Napi::ArrayBuffer>();
    if (alg == "CFB8") {
        helper.createCiphers((u8*)secret.Data(), secret.ByteLength(), (BYTE*)iv.Data());
        mode = 1;
    } else if (alg == "GCM") {
        gcm.createCiphers((unsigned char*)secret.Data(), secret.ByteLength(), (BYTE*)iv.Data());
        mode = 2;
    } else {
        Napi::TypeError::New(env, "Unsupported cipher alg").ThrowAsJavaScriptException();
        return;
    }
}

Napi::Value MinecraftHelper::Cipher(const Napi::CallbackInfo& info) {
    auto data = info[0].As<Napi::Buffer<char>>();
    if (mode == 1) {
        auto enc = helper.cipher((u8*)data.Data(), data.ByteLength());
        auto jsbuffer = Napi::ArrayBuffer::New(info.Env(), enc, data.ByteLength(), cleanup);
        return jsbuffer;
    } else {
        auto enc = gcm.cipher((unsigned char*)data.Data(), data.ByteLength());
        auto jsbuffer = Napi::ArrayBuffer::New(info.Env(), enc, data.ByteLength(), cleanup);
        return jsbuffer;
    }
}

Napi::Value MinecraftHelper::Decipher(const Napi::CallbackInfo& info) {
    auto data = info[0].As<Napi::Buffer<char>>();
    if (mode == 1) {
        auto dec = helper.decipher((u8*)data.Data(), data.ByteLength());
        auto jsbuffer = Napi::ArrayBuffer::New(info.Env(), dec, data.ByteLength(), cleanup);
        return jsbuffer;
    } else {
        auto dec = gcm.decipher((unsigned char*)data.Data(), data.ByteLength());
        auto jsbuffer = Napi::ArrayBuffer::New(info.Env(), dec, data.ByteLength(), cleanup);
        return jsbuffer;
    }
}