/*
 * Copyright (C) 2017 FUJITSUTEN Limited
 * Author Yuichi Kusakabe <yuichi.kusakabe@jp.fujitsu.com>
 * Author Motoyuki.ito <Motoyuki.ito@software-architect-ito.jp> 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <net/if.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <alloca.h>
#include "socketcan_raw.h"
#include "error.h"

static int canfd = -1;
static int can_sendfd = -1;
/**
 * SoketCAN I/F:
 * 	see ${KERNEL_SRC}/Documentation/networking/can.txt
 */
int socketcan_open(const char *devname)
{
	int s;
	struct sockaddr_can addr;
	struct ifreq ifr;
	int err;
	const int timestamp_on = 1;
	struct can_filter rfilter = {
		.can_id =  CAN_SFF_MASK,
		.can_mask =CAN_SFF_MASK
	};

	struct timeval timeout = {
		.tv_sec = 1,
		.tv_usec= 0 };

	s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (s < 0) {
		ERRMSG("SocketCAN create failed:error=%d",errno);
		return -1;
	}
	canfd = s;

#if 1  /* FIXed: send & Receive by same can i/f.*/
	can_sendfd = canfd;
#endif

 	/* Set timeout for read: 1sec */
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	setsockopt(s, SOL_SOCKET, SO_TIMESTAMP, &timestamp_on, sizeof(timestamp_on));
	/* subscribeされるまで CAN受信しない */
	setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, (const void *)&rfilter, sizeof(rfilter));

#if 0  /* Not Support EXTEND-CAN-Frame */
	{
	int canfd_on = 1;
	setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAME, &canfd_on, sizeof(canfd_on));
	}
#endif
	strcpy(ifr.ifr_name, devname);
    err = ioctl(s, SIOCGIFINDEX, &ifr);	/* To determine the interface index */
	if (err) {
		ERRMSG("ioctl(SIOCGIFINDEX)failed: error=%d",errno);
		goto SOCKET_ERROR;
	}

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    err = bind(s, (struct sockaddr *)&addr, sizeof(addr));
	if (err) {
		ERRMSG("bind(SocketCAN) failed: error=%d",errno);
		goto SOCKET_ERROR;
	}
	return s;

/* -- Error return -- */
SOCKET_ERROR:
	close(s);
	return -1;		
}

void socketcan_update_filter(canid_t *canid_list, int n)
{
	struct can_filter *rfilter;
	const socklen_t len = (socklen_t)(sizeof(struct can_filter) * (size_t)n);
	int i;

	if (n <= 0)
		return;
	
	rfilter = alloca(len);
	for (i = 0;  i < n ; i++) {
		rfilter[i].can_id   = canid_list[i];
		rfilter[i].can_mask = CAN_SFF_MASK;
	} 
	setsockopt(canfd, SOL_CAN_RAW, CAN_RAW_FILTER, rfilter, len);
}

/**
 * Send the CAN frame.
 */
int socketcan_send(struct can_frame *frame)
{
	ssize_t nbyte;
	if (can_sendfd < 0) {
		ERRMSG("CAN device is not active");
		return -1;
	}
	nbyte = send(can_sendfd, frame, sizeof(*frame), 0);
	if (nbyte != sizeof(*frame)) {
		ERRMSG("send failed: ret=%d errno=%d", (int)nbyte, errno);
		return -1;
	}
	return 0;
}

/**
 * Read the CAN frame.
 */
int socketcan_read(int fd, struct can_frame *frame, struct timeval *tm)
{
	char tmpbuf[1024];
	struct msghdr msg;
	ssize_t nbyte;
	struct iovec io;
	struct cmsghdr *cmsg;

	memset(frame, 0, sizeof(*frame));	
	memset(&msg, 0 , sizeof(msg));

	io.iov_base = frame;
	io.iov_len  = sizeof(*frame);

	msg.msg_iov    =  &io;
	msg.msg_iovlen =  1;
	msg.msg_control    = &tmpbuf;
	msg.msg_controllen =sizeof(tmpbuf);

	nbyte = recvmsg(fd, &msg, 0);
	if (nbyte <= 0) {
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
			return 1;
		ERRMSG("CAN Raw socket read failed:errno=%d",errno);
		return -1;
	}
	if (nbyte < (ssize_t)(CAN_MTU)) {
		ERRMSG("incomplete CAN frame : %d < %d", (int)nbyte, (int)CAN_MTU);
		return -1;
	}

	if (tm != NULL) {
		struct timeval *t = NULL;
		for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
			if (cmsg->cmsg_type == SO_TIMESTAMP) {
				t = (struct timeval *)CMSG_DATA(cmsg);
				break;
			}
		}
		if (t != NULL) {
			*tm = *t;
		}
	}
	return 0;
}

void socketcan_close(int s)
{
	if (s < 0)
		return;
	close(s);
	canfd = -1;
#if 1
	can_sendfd = -1;
#endif
}
