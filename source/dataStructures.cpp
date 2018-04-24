//Structure for storing space messages

#include <iostream>

using namespace std;

#define MESSAGE_IS_DATA (0x01 << 0)
#define MESSAGE_IS_CONTROL (0x01 << 1)
#define MESSAGE_IS_CODEX (0x01 << 2)
#define MESSAGE_IS_TEARDOWN (0x01 << 3)

typedef struct AckBlock
{
    unsigned int checksum;
    int numAcks;
    int AckBuf[256];
} ACKBLOCK;

typedef struct MessageBlock
{
    unsigned int checksum;
    unsigned int flags;
    int sequenceNumber;
    int messageSize;
    char message[1000];
} MESSAGEBLOCK;

//computes a checksum which is the 1's compliment of the sum of each 2 bytes
unsigned int computeChecksum(struct MessageBlock m)
{

    unsigned int sum = 0;

    short *start, *end;
    start = (short *)&m;
    end = (short *)&m + sizeof(struct MessageBlock);

    while (start < end)
    {
        sum += *start;
        start++;
    }

    cout << "sum is " << sum << endl;
    cout << "compliment is " << ~sum << endl;
    return ~sum;
}

struct MessageBlock setBlockHash(struct MessageBlock m)
{
    m.checksum = 0;
    m.checksum = computeChecksum(m);
    return m;
}

bool checkBlockHash(struct MessageBlock m)
{
    struct MessageBlock copy;
    copy = m;
    copy.checksum = 0;
    unsigned int result = computeChecksum(copy);
    if (m.checksum & result)
    {
        return false;
    }
    else
    {
        return true;
    }
}