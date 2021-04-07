#pragma once
#define MINECRAFT
#ifdef MINECRAFT

#include <string>
#include "aes.h"
#include "miniz.h"
extern "C" {
#include "sha256.h"
}

#include "../Util.h"

typedef const unsigned char u8;
typedef unsigned long long u64;
typedef std::string String;

// Similar to the zlib compress() and decompress() helpers, but with customizable window type (for mcpe, we need to disable zlib magic)
inline int zlib_compress(BYTE *&pDest, u64 &pDest_len, u8 *pSource, u64 source_len, int compressionLevel, int window = Z_DEFAULT_WINDOW_BITS) {
    int ret;
    auto limit = source_len + 32; // worst case is ~+11 bytes, 32 to be safe �\(�_o)/�
    pDest = new unsigned char[limit]; 
    pDest_len = 0;

    z_stream strm;
    strm.zalloc = 0;
    strm.zfree = 0;
    strm.next_in = pSource;
    strm.avail_in = source_len;
    ret = deflateInit2(&strm, compressionLevel, Z_DEFLATED, window, 8, Z_DEFAULT_STRATEGY);
    assert(ret == Z_OK);
    strm.next_out = pDest;
    strm.avail_out = limit;
    ret = deflate(&strm, Z_FINISH);
    assert(ret == Z_STREAM_END);
    pDest_len = strm.total_out;
    return deflateEnd(&strm);
}

// got lazy, copied from https://github.com/Mojang/leveldb-mcpe/blob/master/db/zlib_compressor.cc#L58
inline int zlib_decompress(String &output, u8* source, u64 source_len, int window = Z_DEFAULT_WINDOW_BITS) {
    const int CHUNK = 64 * 1024;
    int ret;
    size_t have;
    z_stream strm;
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = (uint32_t)source_len;
    strm.next_in = (Bytef*)source;

    ret = inflateInit2(&strm, window);

    if (ret != Z_OK) {
        return ret;
    }

    /* decompress until deflate stream ends or end of file */
    do {
        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = ::inflate(&strm, Z_NO_FLUSH);
            if (ret == Z_NEED_DICT) {
                ret = Z_DATA_ERROR;
            }
            if (ret < 0) {
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
               
            output.append((char*)out, have);
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);

    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

struct MinecraftPacketHelper {
    u8* secretBytes;
    int secretLen;
	int recieveCounter = 0;
	int sendCounter = 0;

    // aes stuff
    aes_context cipherCtx;
    BYTE cipherIv[16];
    BYTE decipherIv[16];
    
    void createCiphers(u8* secretKeyBytes, int secretKeyLen, BYTE* iv) {
        this->secretBytes = secretKeyBytes;
        this->secretLen = secretKeyLen;
        aes_setkey_enc(&cipherCtx, secretKeyBytes, secretKeyLen * 8);
        //aes_setkey_dec(&decipherCtx, secretKeyBytes, secretKeyLen * 8);
        memcpy(cipherIv, iv, 16);
        memcpy(decipherIv, iv, 16);
    }

    BYTE* decipher(u8 *buffer, int len) {
        auto output = new BYTE[len];
        mbedtls_aes_crypt_cfb8(&cipherCtx, AES_DECRYPT, len, decipherIv, buffer, output);
        return output;
    }

    BYTE* cipher(u8* buffer, int len) {
        auto output = new BYTE[len];
        mbedtls_aes_crypt_cfb8(&cipherCtx, AES_ENCRYPT, len, cipherIv, buffer, output);
        return output;
    }

    int encryptMCPE(u8* buffer, int len, BYTE *&outBuf, u64 &outLen) {
        // Compress
        BYTE* comp;
        u64 compLen = 0;
        if (zlib_compress(comp, compLen, buffer, len, 7, -15) != Z_OK) {
            printf("Failed to compress buffer");
            return 1;
        }
        // Add checksum
        auto compWithChecksum = new unsigned char[compLen + 8];
        memcpy(compWithChecksum, comp, compLen);
        char sum[8];
        computeChecksum(comp, compLen, sendCounter, secretBytes, secretLen, sum);
        memcpy(&compWithChecksum[compLen], &sum, 8);
        // Encrypt
        outBuf = cipher(compWithChecksum, compLen + 8);
        outLen = compLen + 8;
        // Cleanup and return
        delete[] comp;
        delete[] compWithChecksum;
        return 0;
    }

    int decryptMCPE(u8 *buffer, int len, String &out) {
        // Decipher
        auto dec = decipher(buffer, len);
        // Check checksum
        char computedSum[8];
        computeChecksum(dec, len - 8, recieveCounter, secretBytes, secretLen, computedSum);
        auto expectedSum = *(u64*)(&dec[len - 8]);
        if (*(u64*)&computedSum != expectedSum) {
            printf("Sum mismatch decrypting: %d != %d\n", *(u64*)&computedSum, expectedSum);
            return 1;
        }
        // Decompress
        if (zlib_decompress(out, dec, len, -15) != Z_OK) {
            printf("Failed to decompress buffer!\n");
            return 2;
        }
        // Cleanup and return
        delete[] dec;
        return 0;
    }

    void computeChecksum(u8 *packetPlaintext, u64 packetPlaintextLen, int counter, u8* secretKeyBytes, u64 secretKeyBytesLen, char out[8]) {
        BYTE buf[SHA256_BLOCK_SIZE] = { 0 };
        SHA256_CTX ctx;
        sha256_init(&ctx);
        sha256_update(&ctx, (u8*)&counter, 4);
        sha256_update(&ctx, packetPlaintext, packetPlaintextLen);
        sha256_update(&ctx, secretKeyBytes, secretKeyBytesLen);
        sha256_final(&ctx, buf);
        memcpy(out, buf, 8);
    }
};

#endif