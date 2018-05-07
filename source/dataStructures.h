/* dataStructures.h
A file defining various data structures and management operations on those data structures. Used for the Deep Space File Transfer Protocol.
*/

#pragma once
#include <iostream>

using namespace std;

#define MESSAGE_IS_DATA (0x01 << 0)
#define MESSAGE_IS_CONTROL (0x01 << 1)
#define MESSAGE_IS_CODEX (0x01 << 2)
#define MESSAGE_IS_TEARDOWN (0x01 << 3)

#define ACK_POSITIVE (0x01 << 0)
#define ACK_NEGATIVE (0x01 << 1)

#define MB_MESSAGE_SIZE 1000

typedef struct AckBlock
{
    unsigned int checksum;
    unsigned int flags;
    int numAcks;
    int AckBuf[256];
} ACKBLOCK;

typedef struct MessageBlock
{
    unsigned int checksum;
    unsigned int flags;
    int sequenceNumber;
    int messageSize;
    char message[MB_MESSAGE_SIZE];
} MESSAGEBLOCK;

bool checkMessageFinal(MESSAGEBLOCK *m);
void setMessageFinal(MESSAGEBLOCK *m);
unsigned int computeChecksum(MESSAGEBLOCK m);
MESSAGEBLOCK setBlockHash(MESSAGEBLOCK m);
bool checkBlockHash(MESSAGEBLOCK m);