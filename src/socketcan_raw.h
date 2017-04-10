#ifndef _SOCKETCAN_RAW_H_
#define _SOCKETCAN_RAW_H_
#include <sys/time.h>
#include <linux/can.h>
extern int  socketcan_open(const char *devname);
extern void socketcan_close(int s);
extern void socketcan_update_filter(canid_t *canid_list, int n);
extern int socketcan_read(int fd, struct can_frame *frame, struct timeval *tm);
extern int socketcan_send(struct can_frame *frame);
#endif
