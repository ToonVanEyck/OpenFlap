#ifndef XC_H
#define XC_H

// spoof PIC header

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <poll.h> 

extern struct pollfd pfds[3];
extern int id;
extern uint8_t is_col_end;
extern uint8_t letter[4];

#define DEBUG_WIGGLE() ;

#pragma GCC diagnostic ignored "-Wunused-value"
#define DEBUG_PRINT(...) if(id==0) do{printf("[%d] : ",id);printf(__VA_ARGS__);}while(0)

#define RX_BYTE(_b) int rx = read(pfds[0].fd, &_b, 1)
#define TX_BYTE(_b) pfds[1].events = POLLOUT; int tx = write(pfds[1].fd, &_b, 1)
#define TX_DONE do{}while(0)
#define TX_DELAY do{}while(0)


#endif