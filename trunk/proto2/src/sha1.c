/*
 *  sha1.c
 *
 *  Description:
 *      This file implements the Secure Hashing Algorithm 1 as
 *      defined in FIPS PUB 180-1 published April 17, 1995.
 *
 *      The SHA-1, produces a 160-bit message digest for a given
 *      data stream.  It should take about 2**n steps to find a
 *      message with the same digest as a given message and
 *      2**(n/2) to find any two messages with the same digest,
 *      when n is the digest size in bits.  Therefore, this
 *      algorithm can serve as a means of providing a
 *      "fingerprint" for a message.
 *
 *  Portability Issues:
 *      SHA-1 is defined in terms of 32-bit "words".  This code
 *      uses <stdint.h> (included via "sha1.h" to define 32 and 8
 *      bit unsigned integer types.  If your C compiler does not
 *      support 32 bit unsigned integers, this code is not
 *      appropriate.
 *
 *  Caveats:
 *      SHA-1 is designed to work with messages less than 2^64 bits
 *      long.  Although SHA-1 allows a message digest to be generated
 *      for messages of any number of bits less than 2^64, this
 *      implementation only works with messages with a length that is
 *      a multiple of the size of an 8-bit character.
 *
 */  
    
#include "config.h"
#include "db.h"
#include "externs.h"
#include "strings.h"
#include "sha1.h"
    
/*
 *  Define the SHA1 circular left shift macro
 */ 
#define SHA1CircularShift(bits,word) \
    ((((word) << (bits)) & 0xFFFFFFFF) | ((word) >> (32 - (bits))))  
/* Local Function Prototyptes */ 
void SHA1PadMessage(SHA1Context * context);

void SHA1ProcessMessageBlock(SHA1Context * context);


/*
 *  SHA1Reset
 *
 *  Description:
 *      This function will initialize the SHA1Context in preparation
 *      for computing a new SHA1 message digest.
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to reset.
 *
 *  Returns:
 *      1 on success, 0 on failure.
 *
 */ 
    int
SHA1Reset(SHA1Context * context)
{
    if (!context)
        return 0;
    context->Length_Low = 0;
    context->Length_High = 0;
    context->Message_Block_Index = 0;
    context->Intermediate_Hash[0] = 0x67452301;
    context->Intermediate_Hash[1] = 0xEFCDAB89;
    context->Intermediate_Hash[2] = 0x98BADCFE;
    context->Intermediate_Hash[3] = 0x10325476;
    context->Intermediate_Hash[4] = 0xC3D2E1F0;
    context->Computed = 0;
    context->Corrupted = 0;
    return 1;
}


/*
 *  SHA1Result
 *
 *  Description:
 *      This function will return the 160-bit message digest into the
 *      Message_Digest array  provided by the caller.
 *      NOTE: The first octet of hash is stored in the 0th element,
 *            the last octet of hash in the 19th element.
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to use to calculate the SHA-1 hash.
 *      Message_Digest: [out]
 *          Where the digest is returned.
 *
 *  Returns:
 *      1 on success, 0 on failure.
 *
 */ 
    int
SHA1Result(SHA1Context * context, uint8_t * Message_Digest)
{
    int i;

    if (!context || !Message_Digest)
        return 0;
    if (context->Corrupted)
        return context->Corrupted;
    if (!context->Computed) {
        SHA1PadMessage(context);
        
            /* message may be sensitive, clear it out */ 
            for (i = 0; i < 64; ++i)
            context->Message_Block[i] = 0;
        context->Length_Low = 0; /* and clear length */
        context->Length_High = 0;
        context->Computed = 1;
    }
    for (i = 0; i < SHA1HashSize; ++i)
        Message_Digest[i] =
            context->Intermediate_Hash[i >> 2] >> 8 * (3 - (i & 0x03));
    return 1;
}


/*
 *  SHA1Input
 *
 *  Description:
 *      This function accepts an array of octets as the next portion
 *      of the message.
 *
 *  Parameters:
 *      context: [in/out]
 *          The SHA context to update
 *      message_array: [in]
 *          An array of characters representing the next portion of
 *          the message.
 *      length: [in]
 *          The length of the message in message_array
 *
 *  Returns:
 *      1 on success, 0 on failure.
 *
 */ 
    int
SHA1Input(SHA1Context * context, const uint8_t * message_array, unsigned length)
{
    if (!context || !message_array)
        return 0;
    if (!length)
        return 1;
    if (context->Computed) {
        context->Corrupted = 1;
        return 0;
    }
    if (context->Corrupted)
        return 0;
    while (length-- && !context->Corrupted) {
        context->Message_Block[context->Message_Block_Index++] =
            (*message_array & 0xFF);
        context->Length_Low += 8;
        context->Length_Low &= 0xFFFFFFFF;
        if (context->Length_Low == 0) {
            context->Length_High++;
            context->Length_High &= 0xFFFFFFFF;
            if (context->Length_High == 0)
                context->Corrupted = 1; /* Message is too long */
        }
        if (context->Message_Block_Index == 64)
            SHA1ProcessMessageBlock(context);
        message_array++;
    }
    return 1;
}


/*
 *  SHA1ProcessMessageBlock
 *
 *  Description:
 *      This function will process the next 512 bits of the message
 *      stored in the Message_Block array.
 *
 *  Parameters:
 *      None.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *      Many of the variable names in this code, especially the
 *      single character names, were used because those were the
 *      names used in the publication.
 *
 *
 */ 
    void
SHA1ProcessMessageBlock(SHA1Context * context)
{
    const uint32_t K[] = { /* Constants defined in SHA-1   */ 
        0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6 
    };
    int t;                     /* Loop counter                */

    uint32_t temp;             /* Temporary word value        */
    uint32_t W[80];            /* Word sequence               */
    uint32_t A, B, C, D, E;    /* Word buffers                */
    
        /* Initialize the first 16 words in the array W */ 
        for (t = 0; t < 16; t++) {
        W[t] = (uint32_t) (context->Message_Block[t * 4] << 24) & 0xFF000000;
        W[t] |=
            (uint32_t) (context->Message_Block[t * 4 + 1] << 16) & 0x00FF0000;
        W[t] |= (context->Message_Block[t * 4 + 2] << 8) & 0x0000FF00;
        W[t] |= (context->Message_Block[t * 4 + 3]) & 0x000000FF;
    }
    for (t = 16; t < 80; t++) {
        W[t] =
            SHA1CircularShift(1, W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16]);
        W[t] &= 0xFFFFFFFF;
    }
    A = context->Intermediate_Hash[0];
    B = context->Intermediate_Hash[1];
    C = context->Intermediate_Hash[2];
    D = context->Intermediate_Hash[3];
    E = context->Intermediate_Hash[4];
    for (t = 0; t < 20; t++) {
        temp = SHA1CircularShift(5, A) + 
            ((B & C) | ((~B) & D)) + E + W[t] + K[0];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30, B);
        C &= 0xFFFFFFFF;
        B = A;
        A = temp;
    }
    for (t = 20; t < 40; t++) {
        temp = SHA1CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K[1];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30, B);
        C &= 0xFFFFFFFF;
        B = A;
        A = temp;
    }
    for (t = 40; t < 60; t++) {
        temp = SHA1CircularShift(5, A) + 
            ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30, B);
        C &= 0xFFFFFFFF;
        B = A;
        A = temp;
    }
    for (t = 60; t < 80; t++) {
        temp = SHA1CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K[3];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30, B);
        C &= 0xFFFFFFFF;
        B = A;
        A = temp;
    }
    context->Intermediate_Hash[0] =
        (context->Intermediate_Hash[0] + A) & 0xFFFFFFFF;
    context->Intermediate_Hash[1] =
        (context->Intermediate_Hash[1] + B) & 0xFFFFFFFF;
    context->Intermediate_Hash[2] =
        (context->Intermediate_Hash[2] + C) & 0xFFFFFFFF;
    context->Intermediate_Hash[3] =
        (context->Intermediate_Hash[3] + D) & 0xFFFFFFFF;
    context->Intermediate_Hash[4] =
        (context->Intermediate_Hash[4] + E) & 0xFFFFFFFF;
    context->Message_Block_Index = 0;
}


/*
 *  SHA1PadMessage
 *
 
 *  Description:
 *      According to the standard, the message must be padded to an even
 *      512 bits.  The first padding bit must be a '1'.  The last 64
 *      bits represent the length of the original message.  All bits in
 *      between should be 0.  This function will pad the message
 *      according to those rules by filling the Message_Block array
 *      accordingly.  It will also call the ProcessMessageBlock function
 *      provided appropriately.  When it returns, it can be assumed that
 *      the message digest has been computed.
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to pad
 *      ProcessMessageBlock: [in]
 *          The appropriate SHA*ProcessMessageBlock function
 *  Returns:
 *      Nothing.
 *
 */ 
    void
SHA1PadMessage(SHA1Context * context)
{
    
        /*
         *  Check to see if the current message block is too small to hold
         *  the initial padding bits and length.  If so, we will pad the
         *  block, process it, and then continue padding into a second
         *  block.
         */ 
        if (context->Message_Block_Index > 55) {
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while (context->Message_Block_Index < 64)
            context->Message_Block[context->Message_Block_Index++] = 0;
        SHA1ProcessMessageBlock(context);
        while (context->Message_Block_Index < 56)
            context->Message_Block[context->Message_Block_Index++] = 0;
    } else {
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while (context->Message_Block_Index < 56)
            context->Message_Block[context->Message_Block_Index++] = 0;
    }
    
        /* Store the message length as the last 8 octets */ 
        context->Message_Block[56] = (context->Length_High >> 24) & 0xFF;
    context->Message_Block[57] = (context->Length_High >> 16) & 0xFF;
    context->Message_Block[58] = (context->Length_High >> 8) & 0xFF;
    context->Message_Block[59] = (context->Length_High) & 0xFF;
    context->Message_Block[60] = (context->Length_Low >> 24) & 0xFF;
    context->Message_Block[61] = (context->Length_Low >> 16) & 0xFF;
    context->Message_Block[62] = (context->Length_Low >> 8) & 0xFF;
    context->Message_Block[63] = (context->Length_Low) & 0xFF;
    SHA1ProcessMessageBlock(context);
}


// ProtoMUCK specific code
void 
SHA1hash(void *dest, const void *orig, int len) 
{
    struct SHA1Context context;

    SHA1Reset(&context);
    SHA1Input(&context, (const unsigned char *) orig, len);
    SHA1Result(&context, (uint8_t *) dest);
} 

/* dest buffer MUST be at least 41 chars long. */ 
void 
SHA1hex(void *dest, const void *orig, int len)
{
    unsigned char *tmp = (unsigned char *) malloc(20);

    SHA1hash(tmp, orig, len);
    unsigned char i;

    int j;

    for (j = 0; j <= 19; ++j) {
        i = (unsigned char) tmp[j];
        sprintf((char *) dest + (j * 2), "%.2X", (unsigned char) i);
    } free((void *) tmp);
} 

/* dest buffer MUST be at least 31 chars long. */ 
void 
SHA1base64(char *dest, const void *orig, int len) 
{
    void *tmp = (void *) malloc(20);

    SHA1hash(tmp, orig, len);
    Base64Encode(dest, tmp, 20);
    free(tmp);
}
