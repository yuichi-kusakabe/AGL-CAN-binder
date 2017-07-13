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
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <linux/can/raw.h>
#include <sys/timerfd.h>
#include <systemd/sd-event.h>
#include <alloca.h>
#include "af-canivi-binding.h"
#include "can_signal_event.h"
#include "error.h"
#include "canid_search.h"
#include "socketcan_raw.h"
static pthread_mutex_t frameLock = PTHREAD_MUTEX_INITIALIZER;

static unsigned int elaps_ms(struct timeval *old, struct timeval *new)
{
	time_t elaps_sec = new->tv_sec - old->tv_sec;
	long int result = (elaps_sec*1000)+((new->tv_usec/1000) - (old->tv_usec/1000));
	return (unsigned int)result;
}

static int update_iviproperty(uint64_t can_v64, struct can_bit_t *property_info, struct timeval *timestamp)
{
	update_stat_t ret;
	int update = 0;
	unsigned int elaps = 0;
	unsigned int thinout_cycle = property_info->mycanid->cycle;
	struct can_bit_t *mark = NULL;
	struct can_bit_t *p;
	int is_multiProperty = 0;
	union data_content_t pre_apValue;

	DBGMSG("update iviproperty(\"%s\") called", property_info->name);

	if (property_info->next != NULL) {
		/* Reset exclusive-fileild value*/
		struct can_bit_t *n;

		is_multiProperty = 1;
		if (property_info->applyinfo != NULL) {
			pre_apValue = property_info->applyinfo->curValue;
		} else {
			memset(&pre_apValue, 0, sizeof(pre_apValue));
		}

		for(n= property_info; n != NULL ; n = n->next)
			memset(&n->curValue, 0 , sizeof(n->curValue)); /* to initial */
	}

	elaps = elaps_ms(&property_info->updatetime, timestamp);
	for(p = property_info; p != NULL ; p = p->next) {
		ret = canframe2property(can_v64, p);

		/* property is update ? */
		switch (ret) {
		case UPSTAT_UPDATED: /* property is updated */
			if (mark != NULL) {
				WARNMSG("CAN-ID(%02x): %s properties update conflict. Applied data {POS:%d OFFS:%d LEN:%d} ,ignore {POS:%d OFFS:%d LEN:%d}",
					property_info->mycanid->canid, property_info->name,
					p->byte_pos, p->bit_pos, p->bit_len,
					mark->byte_pos, mark->bit_pos, mark->bit_len);
			}
			mark = p;
			++update;
			if (!is_multiProperty)
				p->updatetime =  *timestamp;
			break;
		case  UPSTAT_NO_UPDATED: /* property is NOT updated */
			if ((!is_multiProperty) && (elaps >= thinout_cycle)) {
				++update;
				p->updatetime =  *timestamp;
			}
			break;
		default: /* ERROR */
			if (ret > 0)
				ERRMSG("Internal error: invalid canframe2property() return:%d", ret);
			/* else
 			 *	  ret < 0 is error
 			 */
			return 1;
			break;
		}
	}

	if (is_multiProperty) {
		if (mark == NULL) {
			/* NOT updated anything. */
			if (elaps >= thinout_cycle) {
				++update;
				property_info->updatetime = *timestamp;
			}
		} else
		if (property_info->applyinfo != mark) {
			/* Applied propety is changed */
			property_info->applyinfo = mark;
			++update;
			property_info->updatetime = *timestamp;
			mark->updatetime = *timestamp;
		} else {
			/* Applied propety is same and applyinfo is !NULL */
			int check = memcmp(&mark->curValue, &pre_apValue, sizeof(pre_apValue));
			if ((check != 0) || (elaps >= thinout_cycle)) {
				++update;
				property_info->updatetime = *timestamp;
			} else {
				update = 0;
			}
		}
	}

	if (update) {
		DBGMSG("	[%s] %s   [elaps:%d msec > (cycle)%u]",
			property_info->name,
			update ? "VALUE-updated":"not change",
			elaps, thinout_cycle);
		notify_property_changed(property_info);
	} /* else
 	   * 		same data in thinout_cycle. So ignore
 	   */


	return 0;
}

static int parse_candata(uint64_t can_v64, struct can_bit_t *properties, unsigned int nProperty, struct timeval *timestamp)
{
	unsigned int i;
	struct can_bit_t *property_info = properties;
	if (property_info == NULL)
		return 0; /* NOT-ERROR */

	for(i = 0; i < nProperty; i++) {
		update_iviproperty(can_v64, property_info, timestamp);
		++property_info;
	}
	return 0;
}

static int parse_frame(struct can_frame *frame, struct timeval *timestamp)
{
	const jcanid_t targetcanid = frame->can_id & CAN_SFF_MASK; /* ONLY STANDARD CAN Frame */
	struct canid_info_t *canid_info;
	int ret = 1;
	uint64_t can_v64;

	if (frame->can_dlc > 8) {
		ERRMSG("invalid frame DLC:%d, CANID=%03x Extend frame is not supported,yet", targetcanid, frame->can_dlc);
		return 1;
	}

	pthread_mutex_lock(&frameLock);
	canid_info = getCANID_dict(targetcanid);
	if (canid_info == NULL)
		goto ERROR_RETURN; /* NOT TARGET frame */
	
	canid_info->canraw_frame = *frame;

	if (frame->can_dlc != canid_info->dlc) {
		WARNMSG("CAN-ID <%03X> invalid frame DLC(%d). <define dlc>=%d", targetcanid, frame->can_dlc, canid_info->dlc);
	}
	DBGMSG("CAN-ID <%03X> readed: %02x%02x%02x%02x %02x%02x%02x%02x", targetcanid,
		frame->data[0], frame->data[1], frame->data[2], frame->data[3],
		frame->data[4], frame->data[5], frame->data[6], frame->data[7]
		);
	if  (canid_info->kind == CAN_KIND_SEND) {
		pthread_mutex_unlock(&frameLock);
		DBGMSG("IGNORE CAN-ID <%03X>: this is Send Only frame.", targetcanid);
		return 0; /* This frame is SEND only (By my service) */
		
	}
	can_v64 = canvalue2host(frame->data);
	ret = parse_candata(can_v64, canid_info->property, canid_info->nData, timestamp);
ERROR_RETURN:
	pthread_mutex_unlock(&frameLock);
	return ret;

}

/* TO other ECU */
void can_signal_send(struct can_bit_t *prop, unsigned int v)
{
	update_stat_t ret;
	pthread_mutex_lock(&frameLock);
	ret = property2canframe(prop, v);
	if (ret != UPSTAT_ERROR)
		socketcan_send(&prop->mycanid->canraw_frame);
	/* else
 	 *		error. not send.
 	 */
	pthread_mutex_unlock(&frameLock);
}
/*
 * Abount CAN Raw frame reading:
 *   see ${KERNELSRC}/Documentation/networking/can.txt
 */
int can_signal_read(int fd, struct can_signal_t *out)
{
	struct can_frame frame;
	struct timeval tstamp;
	int err;
	int read_max=0;
	while( (err = socketcan_read(fd, &frame, &tstamp)) == 0) {
		parse_frame(&frame, &tstamp);
		if (++read_max > 5)
			break;
	}
	
	return 0;
}

void    stopSendTimer(struct canid_info_t *caninfo)
{
	const struct itimerspec t = {
		.it_value    = { .tv_sec = 0, .tv_nsec = 0},
		.it_interval = { .tv_sec = 0, .tv_nsec = 0} };

	timerfd_settime(caninfo->sendTimer.timerfd , /* FD    */
			0,									 /* Flags */
		   &t,									 /* New   */
		   NULL);								 /* Old   */
}

void    updateSendTimer(struct canid_info_t *caninfo)
{
	const struct itimerspec t = {
		.it_value    = { .tv_sec = 1, .tv_nsec = 0},
		.it_interval = { .tv_sec = 1, .tv_nsec = 0} };

	timerfd_settime(caninfo->sendTimer.timerfd ,
			0,
		   &t,
		   NULL);
}

static int on_timerEvent(sd_event_source *s, int fd, uint32_t revents, void *userdata)
{
	uint64_t exp;
	ssize_t  len;
	struct canid_info_t *caninfo = (struct canid_info_t *)userdata;
	len = read(fd, &exp, sizeof(uint64_t));
	if (len != sizeof(uint64_t)) {
		ERRMSG("Timer Event read failed: ret=%d : errno=%d", (int)len, errno);
	}
	socketcan_send(&caninfo->canraw_frame);
	return 0;
}

void addSendTimer(struct canid_info_t *caninfo)
{
	int ret;
	sd_event_source *source;
	caninfo->sendTimer.timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
	if (caninfo->sendTimer.timerfd < 0) {
		ERRMSG("Cannot create timer-fd: errno=%d",errno);
		return;
	}
	ret = sd_event_add_io(
		    afb_daemon_get_event_loop(afbitf->daemon),
			&source,
			caninfo->sendTimer.timerfd,
			EPOLLIN,
			on_timerEvent, caninfo);
	if (ret < 0) {
		ERRMSG("Timer Event loop failed");
	}
	updateSendTimer(caninfo);
}
