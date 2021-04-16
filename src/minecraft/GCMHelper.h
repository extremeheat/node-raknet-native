#pragma once

extern "C" {
#include "gcm.h"
}

struct GCMHelper {
	gcm_context cipherCtx;
    gcm_context decipherCtx;
    const uchar* secretBytes;
    unsigned char cipherIv[12];
    unsigned char decipherIv[12];

    void createCiphers(const uchar* key, int keyLen, unsigned char* iv) {
        gcm_initialize();
        gcm_setkey(&cipherCtx, key, (const uint)keyLen);
        gcm_setkey(&decipherCtx, key, (const uint)keyLen);
        memcpy(cipherIv, iv, 12);
        memcpy(decipherIv, iv, 12);
    }

    unsigned char* decipher(unsigned char* input, int len) {
        auto output = new unsigned char[len];
        size_t tag_len = 0;
        unsigned char* tag_buf = NULL;
        auto ret = gcm_crypt_and_tag(&decipherCtx, DECRYPT, decipherIv, 12, NULL, 0, input, output, len, tag_buf, tag_len);
        return output;
    }

    unsigned char* cipher(unsigned char* input, int len) {
        auto output = new unsigned char[len];
        size_t tag_len = 0;
        unsigned char* tag_buf = NULL;
        auto ret = gcm_crypt_and_tag(&cipherCtx, ENCRYPT, cipherIv, 12, NULL, 0, input, output, len, tag_buf, tag_len);
        return output;
    }
};