#ifndef LIB485NET_H
#define LIB485NET_H

#include "lib485net_hl.h"

extern void getVersion(char *v);
extern void initLib(void);
extern void setAddr(unsigned char a);
extern void setMulticast(unsigned char group, unsigned char which);
extern unsigned char doChecksum(const unsigned char *buf, unsigned char len);
extern unsigned char sendRaw(const unsigned char *packet, unsigned char len);
extern unsigned char recvRaw(unsigned char *packet, unsigned char *len);
extern unsigned char peekPackets(void);
extern void *connectDGram(unsigned char addr, unsigned char localport, unsigned char remoteport);
extern void *listenDGram(unsigned char localport);
extern unsigned char sendDGram(void *conn, const unsigned char *packet, unsigned char len);
extern unsigned char recvDGram(void *conn, unsigned char *packet, unsigned char *len);
extern unsigned char recvDGramLL(void *conn, unsigned char *packet, unsigned char *len);
extern void closeDGram(void *conn);
extern void *listenStream(unsigned char localport);
extern void *connectStream(unsigned char addr, unsigned char localport, unsigned char remoteport);
extern unsigned char sendStream(void *conn, const unsigned char *packet, unsigned char len);
extern unsigned char recvStream(void *conn, unsigned char *packet, unsigned char *len);
extern void closeStream(void *conn);
#endif
