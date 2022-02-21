#include "Util.h"

#include "MessageIdentifiers.h"

void FinalizerCallback(Napi::Env env, void* finalizeData, TsfnContext* context) {
    context->running = false;
    // Join the thread
    context->nativeThread.join();
    // Resolve the Promise previously returned to JS via the CreateTSFN method.
    context->deferred.Resolve(Napi::Boolean::New(env, true));
    delete context;
}

unsigned char GetPacketIdentifier2(RakNet::Packet* p) {
    if (p == 0) return 255;

    if ((unsigned char)p->data[0] == ID_TIMESTAMP) {
        // assert(p->length > sizeof(RakNet::MessageID) + sizeof(RakNet::Time));
        return (unsigned char)p->data[sizeof(RakNet::MessageID) + sizeof(RakNet::Time)];
    } else {
        return (unsigned char)p->data[0];
    }
}

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
            if (i + j < buflen) printf("%c", isprint(buf[i + j]) ? buf[i + j] : '.');
        printf("\n");
    }
}