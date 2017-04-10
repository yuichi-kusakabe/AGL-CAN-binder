/*
 * Copyright (C) 2017 FUJITSU TEN Limited
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
#include <stdlib.h>
#include <string.h>
#include <afb/afb-binding.h>
#include <search.h>
#include <linux/can.h>
#include "canid_info.h"
#include "bind_event.h"
#include "af-canivi-binding.h"
#include "canid_search.h"

#define SUPPORT_CANID_MAX (512)
static  void *bind_event_root = NULL;
static  void (*walk_handler)(struct bind_event_t *entry);
static  canid_t  subscribed_canids[SUPPORT_CANID_MAX];
static  int  nSubscribed_canid;

static int compare(const void *pa, const void *pb)
{
	struct bind_event_t *a = (struct bind_event_t *)pa;
	struct bind_event_t *b = (struct bind_event_t *)pb;
	return strcmp(a->name, b->name);
}

static void _action(const void *nodep, const VISIT which, const int depth)
{
	switch (which) {
	case preorder:
		break;
	case leaf: /*FALLTHROUGH */
	case postorder:
		walk_handler(*(struct bind_event_t **)nodep);
		break;
	case endorder:
		break;
	}
}

void bind_event_walker(void (*handler)(struct bind_event_t *entry))
{
	if (bind_event_root == NULL)
		return;

	walk_handler = handler;
	twalk(bind_event_root, _action);
}

static  void _addSubscribedCanid(struct bind_event_t *entry)
{
	int i;
	for(i=0; i < nSubscribed_canid; i++) {
		if ( (int)(entry->raw_info->mycanid->canid) == subscribed_canids[i]) {
			return ; /* Alrady set*/
		}
	}
	if (SUPPORT_CANID_MAX <= nSubscribed_canid) {
		ERRMSG("Too many subsribed can-id: MAX=%d", SUPPORT_CANID_MAX);
		return;
	}
	subscribed_canids[nSubscribed_canid] = (int)(entry->raw_info->mycanid->canid);
	++nSubscribed_canid;
}

int getSubscribedCanids(canid_t **p)
{
	memset(&subscribed_canids, 0 ,sizeof(subscribed_canids));
	nSubscribed_canid = 0;
	bind_event_walker(_addSubscribedCanid);
	*p = subscribed_canids;
	return  nSubscribed_canid;
}

struct bind_event_t *get_event(const char *name)
{
	struct bind_event_t *evt;
	struct bind_event_t key;
	struct can_bit_t *property_info;

	void *ent;
#ifdef DEBUG
	if (afbitf == NULL) {
		ERRMSG("not have afb interface. not binding??");
		return NULL;
	}
#endif

	/*
 	 * search subscribed event.
 	 */
	key.name = name;
	ent = tfind(&key, &bind_event_root, compare);
	if (ent != NULL)
		return *(struct bind_event_t **)ent;

	/*
 	 * It's new event
 	 */
	/* check supported poropety name. */
	property_info = getProperty_dict(name);
	if (property_info == NULL) {
		ERRMSG("NOT Supported property:\"%s\".", name);
		return NULL;
	}
	evt = calloc(1, sizeof(*evt));
	if (evt == NULL)
		return NULL;

	evt->event = afb_daemon_make_event(afbitf->daemon, property_info->name);
	if (evt->event.itf == NULL) {
		free(evt);
		return NULL;
	}
	evt->name = property_info->name;
	evt->raw_info = property_info;

	ent = tsearch(evt, &bind_event_root, compare);
	if (ent == NULL) {
		ERRMSG("not enogh memory");
		free(evt);
		return NULL;
	}
#ifdef DEBUG /* double check */
	if ((*(struct bind_event_t **)ent) != evt)
		ERRMSG("event \"%s\" is duplicate?", property_info->name);
#endif
	return  evt;
}

int remove_event(const char *name)
{
	struct bind_event_t *dnode;
	struct bind_event_t key;
	void *p;

	key.name = name;
	p = tfind(&key, &bind_event_root, compare);
	if (p == NULL)
		return 0;
	dnode = *(struct bind_event_t **)p;
	tdelete(&key, &bind_event_root, compare);
	free(dnode);

	return 0;
}
