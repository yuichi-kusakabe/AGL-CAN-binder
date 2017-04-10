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
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <json-c/json.h>

#include <systemd/sd-event.h>

#include <afb/afb-binding.h>
#include <afb/afb-service-itf.h>
#include "af-canivi-binding.h"
#include "can_json.h"
#include "canid_search.h"
#include "bind_event.h"
#include "socketcan_raw.h"
#include "can_signal_event.h"
struct canivi_conf {
	char *devname;
};

/*
 *  the interface to afb-damon
 */
const struct afb_binding_interface *afbitf;

/***************************************************************************************/
/***************************************************************************************/
/**                                                                                   **/
/**                                                                                   **/
/**       SECTION: HANDLING OF CONNECTION                                             **/
/**                                                                                   **/
/**                                                                                   **/
/***************************************************************************************/
int notify_property_changed(struct can_bit_t *property_info)
{
	struct bind_event_t *event;

	event =  get_event(property_info->name);
	if (event == NULL) {
		ERRMSG("Internal error: cannot get the event(\"%s\")", property_info->name);
		return 1;
	}
	if (afb_event_is_valid(event->event)) {
		int recved_client;
		recved_client = afb_event_push(event->event, property2json(property_info));
		if(recved_client < 1) {
			ERRMSG("count of clients that received the event : %d",recved_client);
			return 1;
		}
	} else {
		ERRMSG("event(\"%s\") is invalid. don't adb_event_push()", property_info->name);
		return 1;
	}
	return 0;
}
/***************************************************************************************/
/*
 * called on an event on the CAN stream
 */
static int on_event(sd_event_source *s, int fd, uint32_t revents, void *userdata)
{
	struct can_signal_t can_signal;
	if ((revents & EPOLLIN) != 0) {
		int rc = can_signal_read(fd, &can_signal);
		if (rc) {
			ERRMSG("CAN Frame Read failed");
			return -1;
		}
	} 
	if ((revents & (EPOLLERR|EPOLLRDHUP|EPOLLHUP)) != 0) {
		/* T.B.D
		 * if error or hungup */
		ERRMSG("Error or Hunup: rvent=%08x", revents);
	}
	return 0;
}

static int	connection(struct canivi_conf *conf)
{
	int s;
	int ret;
	sd_event_source *source;
	s = socketcan_open(conf->devname);
	if (s < 0) {
		return -1;
	}
	ret = sd_event_add_io(
		    afb_daemon_get_event_loop(afbitf->daemon),
			&source,
			s,
			EPOLLIN,
			on_event, NULL);
	if (ret < 0) {
		socketcan_close(s);
		ERRMSG("can't connect, the event loop failed");
		return -1;
	}
	NOTICEMSG("connect to CAN(%s), event loop started",conf->devname);
	return 0;
}

/***************************************************************************************/
/**                                                                                   **/
/**                                                                                   **/
/**       SECTION: BINDING VERBS IMPLEMENTATION                                       **/
/**                                                                                   **/
/**                                                                                   **/
/***************************************************************************************/
/**
 * get method.
 */
static void get(struct afb_req req)
{
	const char *prop_name;
	struct can_bit_t *prop;
	prop_name = afb_req_value(req, "event");
	json_object *ans;
	if (prop_name == NULL) {
		ERRMSG("Not specified the property-name");
		afb_req_fail(req, "request error", NULL);
		return;
	}
	prop = getProperty_dict(prop_name);
	if (prop == NULL) {
		afb_req_fail(req, "Unknown property",NULL);
		return;
	}
	ans = property2json(prop);
	if (ans == NULL) {
		afb_req_fail(req, "Unsupported property",NULL);
		return;
	}
	json_object_object_add(ans, "name", json_object_new_string(prop_name));
	afb_req_success(req, ans, NULL);
}

/**
 * get method.
 */
static void set(struct afb_req req)
{
	const char *prop_name;
	const char *valueStr;
	struct can_bit_t *prop;
	long int v;

	prop_name = afb_req_value(req, "event");
	if (prop_name == NULL) {
		ERRMSG("Not specified the send property-name");
		afb_req_fail(req, "request error", NULL);
		return;
	}
	valueStr = afb_req_value(req, "value");
	if (valueStr == NULL) {
		ERRMSG("Not specified the send value");
		afb_req_fail(req, "request error", NULL);
		return;
	}
	v = strtol(valueStr, NULL, 0);
	prop = getProperty_dict(prop_name);
	if (prop == NULL) {
		afb_req_fail(req, "Unknown property",NULL);
		return;
	}
	can_signal_send(prop, (unsigned int)v);
	updateSendTimer(prop->mycanid);

	afb_req_success(req, NULL, NULL);
}


/**
 * list method.
 */
static struct json_object *tmplists;
static void _listup_properties(struct canid_info_t *can)
{
	unsigned int i;
	for (i=0; i < can->nData ; i++) {
		if (can->property[i].name == NULL)
			continue;
		json_object_array_add(tmplists,
			json_object_new_string(can->property[i].name));
	}
}

static void list(struct afb_req req)
{
	if (tmplists != NULL) {
		json_object_put(tmplists);
	}
	tmplists = json_object_new_array();
	if (tmplists == NULL) {
		ERRMSG("not enogh memory");
		afb_req_fail(req,"not enogh memory", NULL);
		return;
	}
	(void)canid_walker(_listup_properties);
	afb_req_success(req, tmplists, NULL);
}
/**
 * subscribe to notification of CAN properties
 * 
 *  parameters of the subscription are:
 *  
 *  event:   string:  Propertie name.
 *                    All properties is "*"
 */
static int subscribe_all(struct afb_req req, struct json_object *apply);

static int  subscribe_signal(struct afb_req req, const char *name, struct json_object *apply)
{
	struct bind_event_t *event;
	if(strcmp(name, "*") == 0)
		return subscribe_all(req, apply);

	event = get_event(name);
	if (event == NULL) {
		ERRMSG("get_event failed.");
		return 1;
	}
	if (afb_req_subscribe(req, event->event) != 0) {
		ERRMSG("afb_req_subscrive() is failed.");
		return 1;
	}
	json_object_object_add(apply, event->name, json_object_new_int(0));
	return 0;
}

static int subscribe_all(struct afb_req req, struct json_object *apply)
{ 
	int i;
	int nProp;
	const char **list = getSupportPropertiesList(&nProp);
	int err = 0;
	for(i = 0; i < nProp; i++) 
		err += subscribe_signal(req, list[i], apply);

	return err;
}

static void subscribe(struct afb_req req)
{
	struct json_object *apply;
	int err = 0;
	canid_t *targetids;
	int ncanid;
	const char *event;
	
	event = afb_req_value(req, "event");
	if (event == NULL) {
		ERRMSG("Unknwon subscrive event args");
		afb_req_fail(req, "error", NULL);
		return;
	}
	apply = json_object_new_object();
	err = subscribe_signal(req, event, apply);

	ncanid = getSubscribedCanids(&targetids);
	(void)socketcan_update_filter(targetids, ncanid);

	if (!err) {
		afb_req_success(req, apply, NULL);
	} else {
		json_object_put(apply);
		afb_req_fail(req, "error", NULL);
	}

}

static int unsubscribe_all(struct afb_req req);
static int unsubscribe_signal(struct afb_req req, const char *name)
{
	struct bind_event_t *event;
	if(strcmp(name, "*") == 0)
		return unsubscribe_all(req);

	event = get_event(name);
	if (event == NULL) {
		ERRMSG("not subscribed event \"%s\"", name);
		return 0; /* Alrady unsubscribed */
	}
	if (afb_req_unsubscribe(req, event->event) != 0) {
		ERRMSG("afb_req_subscrive() is failed.");
		return 1;
	}
	(void)remove_event(name);
	return 0;
}

static int unsubscribe_all(struct afb_req req)
{ 
	int i;
	int nProp;
	const char **list = getSupportPropertiesList(&nProp);
	int err = 0;
	for(i = 0; i < nProp; i++) 
		err += unsubscribe_signal(req, list[i]);

	return err;
}

static void unsubscribe(struct afb_req req) {
	struct json_object *args, *val;
	int n,i;
	int err = 0;

	args = afb_req_json(req);
	if ((args == NULL) ||
		!json_object_object_get_ex(args, "event", &val)) {
		err = unsubscribe_all(req);
	} else
	if ( json_object_get_type(val) != json_type_array) {
		err = unsubscribe_signal(req, json_object_get_string(val));
	} else {
		struct json_object *ent;
		n = json_object_array_length(val);
		for (i = 0; i < n; i++) {
			ent = json_object_array_get_idx(val,i);
			err += unsubscribe_signal(req, json_object_get_string(ent));
		}
	}
	if (!err)
		afb_req_success(req, NULL, NULL);
	else
		afb_req_fail(req, "error", NULL);
}
/*
 * array of the verbs exported to afb-daemon
 */
static const struct afb_verb_desc_v1 binding_verbs[] = {
  /* VERB'S NAME            SESSION MANAGEMENT          FUNCTION TO CALL         SHORT DESCRIPTION */
  { .name= "get",          .session= AFB_SESSION_NONE, .callback= get,          .info= "get the last known data" },
  { .name= "set",          .session= AFB_SESSION_NONE, .callback= set,          .info= "set the data" },
  { .name= "list",          .session= AFB_SESSION_NONE, .callback= list,          .info= "list of support events" },
  { .name= "subscribe",    .session= AFB_SESSION_NONE, .callback= subscribe,    .info= "subscribe to notification of IVI-property" },
  { .name= "unsubscribe",  .session= AFB_SESSION_NONE, .callback= unsubscribe,  .info= "unsubscribe a previous subscription" },
  { .name= NULL } /* marker for end of the array */
};

/*
 * description of the binding for afb-daemon
 */
static const struct afb_binding binding_description =
{
  /* description conforms to VERSION 1 */
  .type= AFB_BINDING_VERSION_1,
  .v1= {					/* fills the v1 field of the union when AFB_BINDING_VERSION_1 */
    .prefix= "canivi",		 /* the API name (or binding name or prefix) */
    .info= "Vehicle-Information from CAN", /* short description of of the binding */
    .verbs = binding_verbs			 /* the array describing the verbs of the API */
  }
};

/*
 * activation function for registering the binding called by afb-daemon
 */
const struct afb_binding *afbBindingV1Register(const struct afb_binding_interface *itf)
{
	afbitf = itf;			/* records the interface for accessing afb-daemon */
	return &binding_description;	/* returns the description of the binding */
}

/*
 * parse configuration file (canivi.json)
 */
static int init_conf(int fd_conf, struct canivi_conf *conf)
{
#define CONF_FILE_MAX (1024)
	char filebuf[CONF_FILE_MAX];
	json_object *jobj;

	memset(filebuf,0, sizeof(filebuf));	
	FILE *fp = fdopen(fd_conf,"r");
	if (fp == NULL) {
		ERRMSG("canno read configuration file(canivi.json)");
		return -1;
	}
	fread(filebuf, 1, sizeof(filebuf), fp);
	fclose(fp);
	jobj = json_tokener_parse(filebuf);
	if (jobj == NULL) {
		ERRMSG("json: Invalid canivi.json format");
		return -1;
	}
	json_object_object_foreach(jobj, key, val) {
		if (strcmp(key,"dev_name") == 0) {
			conf->devname = strdup(json_object_get_string(val));
		} else
		if (strcmp(key,"can_map") == 0) {
			can_define_init(json_object_get_string(val));
		} 
	}
	json_object_put(jobj);

	return 0;
}

int afbBindingV1ServiceInit(struct afb_service service)
{
	int fd_conf;
	struct canivi_conf conf;
   	fd_conf = afb_daemon_rootdir_open_locale(afbitf->daemon, "canivi.json", O_RDONLY, NULL);
	if (fd_conf < 0) {
		ERRMSG("canivi configuration (canivi.json) is not access");
		return -1;
	}
	if (init_conf(fd_conf, &conf)) {
		return -1;
	}
	return connection(&conf);
}
