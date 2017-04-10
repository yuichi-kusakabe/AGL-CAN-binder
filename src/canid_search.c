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
#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <string.h>
#include "canid_info.h"
#include "error.h"

static void *canid_root = NULL;
static void *property_root = NULL;
static void (*_walker_action)(struct canid_info_t *);
#define SUPPORT_PROPERTY_LIMIT 512
static const char *support_property_list[SUPPORT_PROPERTY_LIMIT];
static int nProperties = 0;
static int property_compare(const void *pa, const void *pb)
{
	struct can_bit_t *a =(struct can_bit_t *)pa;
	struct can_bit_t *b =(struct can_bit_t *)pb;
	return strcmp(a->name,b->name);
}

static int addProperty_dict(struct can_bit_t *can_property)
{
	struct can_bit_t *last, *cpEntry;
	void *v;
	v = tfind((void *)can_property, &property_root, property_compare);
	if (v == NULL) {
		v = tsearch((void *)can_property, &property_root, property_compare);
		if (v == NULL) {
			ERRMSG("add property failed: not enogh memory?");
			return -1;
		}
		if (nProperties >= SUPPORT_PROPERTY_LIMIT) {
			ERRMSG("Reach properties limit");
			return -1;
		}
		support_property_list[nProperties] = can_property->name;
		++nProperties;
		return 0;
	} /* else  Multi entry */

	DBGMSG("CANID(%03X)#Property(%s) is duplicate define.",
			can_property->mycanid->canid, can_property->name);
	if (can_property->mycanid->canid != (*(struct can_bit_t **)v)->mycanid->canid) {
		ERRMSG("CANID(%03X)#%s conflicts with CANID(%03X)#%s",
			can_property->mycanid->canid,
			can_property->name,
			(*(struct can_bit_t **)v)->mycanid->canid,
			(*(struct can_bit_t **)v)->name);
 			return -1;
	}
	cpEntry = malloc(sizeof(struct can_bit_t));
	if (cpEntry == NULL) {
		ERRMSG("not enogh memory");
		return -1;
	}
	memcpy(cpEntry, can_property, sizeof(struct can_bit_t));
	for(last = *(struct can_bit_t **)v; last->next != NULL; last = last->next) {
		if (last->var_type != cpEntry->var_type) {
			ERRMSG("CANID(%03X)#%s conflicts with var_type: %s and %s",
				cpEntry->mycanid->canid,
				cpEntry->name,
				vartype2string(cpEntry->var_type),
				vartype2string(last->var_type));
		}
	}
	last->next = cpEntry;
	return 1;
}

static int canid_compare(const void *pa, const void *pb)
{
	struct canid_info_t *a =(struct canid_info_t *)pa;
	struct canid_info_t *b =(struct canid_info_t *)pb;
	return (int)(a->canid - b->canid);
}

int addCANID_dict(struct canid_info_t *canid_info)
{
	void *v;
	unsigned int i;
	unsigned int maxEntry = canid_info->nData;
	v = tsearch((void *)canid_info, &canid_root, canid_compare);
	if (v == NULL) {
		ERRMSG("add CANID failed");
		return -1;
	}
	if ( (*(struct canid_info_t **)v) != canid_info ) {
		ERRMSG("CANID(%03X) entry is duplicate define.",
			canid_info->canid);
		return -1;
	}

	for(i=0; i < maxEntry ; i++) {
		int ret;
		ret = addProperty_dict(&canid_info->property[i]);
		if (!ret)
			continue;
		if (ret > 0) {
			if ( i < (maxEntry-1) ) {
				memmove(&canid_info->property[i], &canid_info->property[i+1], sizeof(struct can_bit_t) * (maxEntry - (i+1)));
			}
			--maxEntry;
			--i;
		}
	}
	if (maxEntry != canid_info->nData) {
#if 1
		DBGMSG("[SHRINKed] CANID:%03X DATA[%d-->%d] = ", canid_info->canid, canid_info->nData, maxEntry);
		for (i=0; i < maxEntry; i++) {
			struct can_bit_t *chain;
			DBGMSG("	[%d] %s : ...", i, canid_info->property[i].name);
			for( chain = canid_info->property[i].next ; chain != NULL; chain = chain->next)
				DBGMSG("		next --> %s :{ ...}", canid_info->property[i].name);
		}
		DBGMSG("	[%d] <following DUST>", i);
#endif
		canid_info->nData = maxEntry;
	}
	return 0;
}

struct canid_info_t * getCANID_dict(jcanid_t canid)
{
	struct canid_info_t key;
	void *v;
	key.canid = canid;

	v = tfind((void *)&key, &canid_root, canid_compare);
	if (v == NULL) {
		return NULL;
	}
	return (*(struct canid_info_t **)v);
}

struct can_bit_t * getProperty_dict(const char *propertyName)
{
	struct can_bit_t key;
	void *v;

	key.name = propertyName;
	v = tfind((void *)&key, &property_root, property_compare);
	if (v == NULL) {
		return NULL;
	}
	return (*(struct can_bit_t **)v);
}

static void canid_walk_action(const void *nodep, const VISIT which , const int depth)
{
	struct canid_info_t *datap;

	switch (which) {
	case preorder:
		break;
	case postorder:
		datap = *(struct canid_info_t **) nodep;
		_walker_action(datap);
		break;
	case endorder:
		break;
	case leaf:
		datap = *(struct canid_info_t **) nodep;
		_walker_action(datap);
		break;
	}
}

void canid_walker(void (*walker_action)(struct canid_info_t *))
{
	if (walker_action == NULL) 
		return;
	_walker_action = walker_action;
	twalk(canid_root, canid_walk_action);
}

const char **getSupportPropertiesList(int *nEntry)
{
	*nEntry = nProperties;
	return support_property_list;
}

static void free_canidinfo(void *nodep)
{
 	struct canid_info_t * node = (struct canid_info_t *)nodep;
	free(node);
}
static void free_canbit(void *nodep)
{
	struct can_bit_t *node;

	for( node = (struct can_bit_t *)nodep; node != NULL; node = node->next) {
		if (node->name == NULL)
			continue;
		free((void *)node->name);
		node->name = NULL;
	}
}

void destroy_dict()
{
	tdestroy(property_root, free_canbit);
	tdestroy(canid_root, free_canidinfo);
	canid_root = NULL;
	property_root = NULL;
}
