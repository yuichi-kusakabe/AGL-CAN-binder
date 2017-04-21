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
#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>
#include "can_json.h"
#include "error.h"
#include "canid_info.h"
#include "canid_search.h"
#include "can_signal_event.h"
enum {
	ZONE_NONE  =  0,
	ZONE_FRONT = 1,
	ZONE_MIDDLE= 1 << 1,
	ZONE_RIGHT = 1 << 2,
	ZONE_LEFT  = 1 << 3,
	ZONE_REAR  = 1 << 4,
	ZONE_CENTER = 1 << 5,
	ZONE_LEFTSIDE = 1 << 6,
	ZONE_RIGHTSIDE = 1 << 7,
	ZONE_FRONTSIDE = 1 << 8,
	ZONE_BACKSIDE = 1 << 9,
	ZONE_PASSENGER = 1 << 10,
	ZONE_DRIVER = 1 << 11
};

static char * _getZoneName(json_object *obj) 
{
	int i;
	int n;
	int zone_val = 0;
	static char zone_name[128];
	if (obj == NULL)
		return 0;
	array_list *zonelist = json_object_get_array(obj);
	n =  array_list_length(zonelist);
	for(i=0; i < n ;i++) {
		json_object *zobj = array_list_get_idx(zonelist,i);
		const char *zname = json_object_get_string(zobj);

		if (strcmp(zname ,"Zone::None") == 0)
			zone_val = 0;
		else
		if (strcmp(zname ,"Zone::Front") == 0)
			zone_val |= ZONE_FRONT;
		else
		if (strcmp(zname ,"Zone::Middle") == 0)
			zone_val |= ZONE_MIDDLE;
		else
		if (strcmp(zname ,"Zone::Right") == 0)
			zone_val |= ZONE_RIGHT;
		else
		if (strcmp(zname ,"Zone::Left") == 0)
			zone_val |= ZONE_LEFT;
		else
		if (strcmp(zname ,"Zone::Rear") == 0)
			zone_val |= ZONE_REAR;
		else
		if (strcmp(zname ,"Zone::Center") == 0)
			zone_val |= ZONE_CENTER;
		else
		if (strcmp(zname ,"Zone::LeftSide") == 0)
			zone_val |= ZONE_LEFTSIDE;
		else
		if (strcmp(zname ,"Zone::RightSide") == 0)
			zone_val |= ZONE_RIGHTSIDE;
		else
		if (strcmp(zname ,"Zone::FrontSide") == 0)
			zone_val |= ZONE_FRONTSIDE;
		else
		if (strcmp(zname ,"Zone::BackSide") == 0)
			zone_val |= ZONE_BACKSIDE;
		else
		if (strcmp(zname ,"Zone::Driver")  == 0)
			zone_val |= ZONE_DRIVER;
		else
		if (strcmp(zname ,"Zone::Passenger") == 0)
			zone_val |= ZONE_PASSENGER;
		else
			ERRMSG("Unknown Zone type:%s", zname);
	}

	if (zone_val == 0)
		return NULL;
	memset(zone_name,0,sizeof(zone_name));
	if (zone_val & ZONE_FRONT)
		strcat(zone_name,"Front");
	if (zone_val & ZONE_MIDDLE)
		strcat(zone_name,"Middle");
	if (zone_val & ZONE_RIGHT)
		strcat(zone_name,"Right");
	if (zone_val & ZONE_LEFT)
		strcat(zone_name,"Left");
	if (zone_val & ZONE_REAR)
		strcat(zone_name,"Rear");
	if (zone_val & ZONE_CENTER)
		strcat(zone_name,"Center");
	if (zone_val & ZONE_LEFTSIDE)
		strcat(zone_name,"Leftside");
	if (zone_val & ZONE_RIGHTSIDE)
		strcat(zone_name,"Rightside");
	if (zone_val & ZONE_FRONTSIDE)
		strcat(zone_name,"Frontside");
	if (zone_val & ZONE_BACKSIDE)
		strcat(zone_name,"Backside");
	if (zone_val & ZONE_DRIVER)
		strcat(zone_name,"Driver");
	if (zone_val & ZONE_PASSENGER)
		strcat(zone_name,"Passenger");

	return zone_name;
}

static void *create_convert_tables( json_object *obj, unsigned int var_type, unsigned int *nEntry)
{
	int n,i;
	void *table;
	size_t entrySize;
	array_list *convlist;

	if (obj == NULL)
		return NULL;

	switch(var_type) {
	case STRING_T:
		entrySize = sizeof(char *);
		break;
	case INT8_T:
	case INT16_T:
	case INT_T:
	case INT32_T:
	case UINT8_T:
	case UINT16_T:
	case UINT_T:
	case UINT32_T:
	case BOOL_T:
	case ENABLE1_T:
		entrySize = sizeof(int);
		break;
	default:
		return NULL;
		/* NOTREACHED */
	}

	convlist = json_object_get_array(obj);
	if (convlist == NULL)
		return NULL;

	n =  array_list_length(convlist);
	if (n <= 0)
		return NULL;

	table = calloc((size_t)n, entrySize);

	if (var_type == STRING_T) {	
		char **p = (char **)table;
		for(i=0; i < n ;i++) {
			json_object *eobj = array_list_get_idx(convlist,i);

			/* specified type check */
			if (json_object_is_type(eobj, json_type_string)) {
				const char *name = json_object_get_string(eobj);
				p[i] = strdup(name);
			} else {
				ERRMSG("Invalid SET-DATA array type: var_type is \"string\", But SET-DATA is not string\n");
				free(table);
				return NULL;
			}
		}
	} else {
		/* ENABLE1_T or BOOL_T or *INT* */
		int *p = (int *)table;
		for(i=0; i < n ;i++) {
			json_object *eobj = array_list_get_idx(convlist,i);
			/* specified type check */
			if (json_object_is_type(eobj, json_type_int)) {
				p[i] = json_object_get_int(eobj);
			} else {
				ERRMSG("Invalid SET-DATA array type: var_type is integer, But SET-DATA is not int\n");
				free(table);
				return NULL;
			}
		}
	}
	*nEntry = (unsigned int)n;
	return table;
}

static int add_propertyMember(struct canid_info_t *canid_info, int idx, json_object *obj, unsigned int property_kind)
{
	char buf[256];
	unsigned char byte_pos = 0;
	unsigned char bit_pos = 0;
	unsigned char bit_len = 0;
	int var_type = 0;
	char *name = NULL;
	int custom = 0;
	char *zone = NULL;
	unsigned int nconv = 0;
	void *dataconv = NULL;
	canid_info->property[idx].mycanid = canid_info;
   	json_object_object_foreach(obj, key, val) {
		if (strcmp("POS", key) == 0) {
			byte_pos = (unsigned char)json_object_get_int(val);
			if (byte_pos > 8) {
				ERRMSG("CAN-ID(%03X):Invalid POS value:%d", canid_info->canid, byte_pos);
				byte_pos = 0;
			}
		} else
		if (strcmp("OFFS", key) == 0) {
			bit_pos = (unsigned char)json_object_get_int(val);
			if (bit_pos > 8) {
				ERRMSG("CAN-ID(%03X):Invalid OFFS value:%d", canid_info->canid,bit_pos);
				bit_pos = 0;
			}
		} else
		if (strcmp("LEN", key) == 0) {
			bit_len = (unsigned char)json_object_get_int(val);
			if (bit_len > 32) {
				ERRMSG("CAN-ID(%03X):Invalid LEN value:%d", canid_info->canid,bit_len);
				bit_len = 0;
			}
		} else
#ifdef DEBUG /* CUSTOM is not nessary */
		if (strcmp("CUSTOM", key) == 0) {
			if ( json_object_get_boolean(val) ) {
				custom = 1;
			} else {
				custom = 0;
			}
#if 1  /* CANRawPlugin では, CUSTOMの設定を json_object_get_int() を用いている様だがよいのだろうか？ */
			{
				int v = json_object_get_int(val);
				if ( v != custom )
					ERRMSG("json_object_get_int() vs json_object_get_boolean() is Diffrent-Result ");
			}
#endif
		} else
#endif /* CUSTOM is not nessary */
		if (strcmp("PROPERTY", key) == 0) {
			name = (char *)json_object_get_string(val);
		} else
		if (strcmp("ZONE", key) == 0) {
			zone = _getZoneName(val);
		} else
		if (strcmp("TYPE", key) == 0) {
			const char *_varname = json_object_get_string(val);
			var_type = string2vartype(_varname);
			if (var_type < 0) {
				ERRMSG("CAN-ID(%03X): unknown TYPE :\"%s\"", canid_info->canid, _varname);
				var_type = 0;
			}
		} else
		if (strcmp("MULTI-PARAM", key) == 0) {
			/* NOTHING : Ignore */
		} else
		if (strcmp("ADD-PARAM", key) == 0) {
			/* NOTHING : Ignore */
		} else
		if (strcmp("SET-DATA",key) == 0) {
			if (property_kind != CAN_KIND_SEND) {
				dataconv = create_convert_tables(val, (unsigned int)var_type, &nconv);
				if (dataconv == NULL) {
					const char *typename = vartype2string((unsigned int)var_type);
					WARNMSG("CAN-ID(%03X)#%s SET-DATA is ignored for %s type", 
						canid_info->canid,
						name,
						typename == NULL ? "<Unknown>":typename);
				}
			} else {
				ERRMSG("<LIMITATION> Not support SET-DATA into the TRANSMISSION block");
			}
		} else {
			ERRMSG("json: canid=%03X unknown token \"%s\"",canid_info->canid, key);
		}
	}

	if (name == NULL) {
			ERRMSG("json: canid=%03X : name is notdefined.",canid_info->canid );
			name = "Unknown";
	}
	if (zone == NULL) {
		canid_info->property[idx].name = strdup(name);
	} else {
		sprintf(buf,"%s@%s", name, zone);
		canid_info->property[idx].name = strdup(buf);
	}

	canid_info->property[idx].byte_pos = byte_pos;
	canid_info->property[idx].bit_pos = bit_pos;
	canid_info->property[idx].bit_len = bit_len;
	canid_info->property[idx].var_type = (unsigned char)var_type;
	canid_info->property[idx].property_kind = property_kind;
	canid_info->property[idx].dataconv = dataconv;
	canid_info->property[idx].nconv = nconv;
	return 0;
}

static int add_propertyMapping(struct canid_info_t *canid_info, int max, json_object *array_obj, unsigned int property_kind)
{
	int err = 0;
	int i;
	json_object * prop_obj;
	if (max < 1) {
		ERRMSG("property-mapping  request is  empty: max=%d", max);
		return 1;
	}
	canid_info->nData = (unsigned int)max;

	for ( i = 0; i < max; i++) {
		prop_obj = json_object_array_get_idx(array_obj, i);
		add_propertyMember(canid_info, i, prop_obj, property_kind);
	}

	return err;
}
/**
 * json CAN-ID form:
 *		"MODE"	: "CAN_THINOUT_CHG",
 *		"CYCLE"	: 200,
 *		"DATA" : [ ... ] array
 */

#define DEFAULT_CANINFO (16)
static int parse_caninfo(jcanid_t canid, json_object *obj, unsigned int property_kind)
{
	int err = 0;
	size_t ms = sizeof(struct canid_info_t) + (size_t)(DEFAULT_CANINFO*sizeof(struct can_bit_t));
	json_object *sub_obj;
	struct canid_info_t *canid_info;

	if (getCANID_dict(canid) != NULL) {
		ERRMSG("Duplicate definition CANID(%03X) :", canid);
		return 1;
	}

	canid_info=malloc(ms);
	if (canid_info == NULL) {
		ERRMSG("Not enogh memory");
		return 1;
	}
	memset(canid_info, 0 , ms);
	canid_info->canid = canid & CAN_SFF_MASK;
	canid_info->kind  = property_kind;
	canid_info->canraw_frame.can_id = canid;
	canid_info->canraw_frame.can_dlc= 8;
	json_object_object_get_ex(obj, "MODE", &sub_obj);
	if (sub_obj) {
		const char *s_mode = json_object_get_string(sub_obj);
		if (strcmp(s_mode, "CAN_THINOUT_CHG") == 0)
			canid_info->mode = (unsigned char)CAN_THINOUT_CHG;
		else
		if (strcmp(s_mode, "CAN_THINOUT_NON") == 0)
			canid_info->mode = (unsigned char)CAN_THINOUT_NON;
		else {
			ERRMSG("json: CAN-ID %03X : invalid mode \"%s\"", canid_info->canid, s_mode);
			canid_info->mode = (unsigned char)CAN_THINOUT_NON;
		}
	} /*else default : CAN_THINOUT_NON */

	json_object_object_get_ex(obj, "CYCLE", &sub_obj);
	if (sub_obj) {
		unsigned int cycle = (unsigned int)json_object_get_int(sub_obj);
		if (canid_info->mode == CAN_THINOUT_CHG) {
 			canid_info->cycle = (cycle == 0) ? 0xffffffff : cycle;
		} else {
			if (cycle > 0)
				WARNMSG("json: CAN-ID(%03X) : ignore CYCLE(%d), not CAH_THINOUT_CHG.",
					(int)cycle,canid_info->canid);
				/* canid_info->cycle = <alrady initialize by 0> */
		}
	}
	/* else 
 	 *	canid_info->cycle = 0 == CAN_THINOUT_NON;
	 */

	json_object_object_get_ex(obj, "DLC", &sub_obj);
	if (sub_obj) {
		canid_info->dlc = (unsigned char)json_object_get_int(sub_obj);
	}

	json_object_object_get_ex(obj, "DATA", &sub_obj);
	if (sub_obj) {
		enum json_type type = json_object_get_type(sub_obj);
		if (type == json_type_array) {
			int array_len = json_object_array_length(sub_obj);
			if (array_len > DEFAULT_CANINFO) {
				canid_info = realloc(canid_info,
					sizeof(struct canid_info_t) + ((size_t)array_len * sizeof(struct can_bit_t)));
				if (canid_info == NULL) {
					ERRMSG("not enogh memory");
					exit(1);
				}
				/* clear appended area */
				memset(&canid_info->property[DEFAULT_CANINFO], 0,
				 (size_t)(array_len - DEFAULT_CANINFO)*sizeof(struct can_bit_t));
			}
			err += add_propertyMapping(canid_info, array_len, sub_obj, property_kind);

			if ( addCANID_dict(canid_info) ) {
				free(canid_info);
				canid_info = NULL;
				++err;
			}
		} else {
			++err;
			ERRMSG("invalid DATA format. (must be array)");
		}
	} else {
		++err;
		ERRMSG("Not have DATA");
	}
	if (err == 0) {
		if  (property_kind & CAN_KIND_SEND) {
			addSendTimer(canid_info);
		}
	}
	return err;
}

/**
 * json form:
 * 	  "CANID" : {...}
 */
static int parse_canid_def(json_object *obj, unsigned int property_kind)
{
	int err=0;
	long int canid;
    json_object_object_foreach(obj, key, val) {
		canid = strtol(key,NULL,16);
		if (canid <= 0) {
			ERRMSG("json: invalid can-id:\"%s\"",key);
			++err;
		}
		parse_caninfo((jcanid_t)canid, val, property_kind);
	}
	return err;
}

/**
 * json form:
 * 	  "STANDARD" : {...}
 * 	  "EXTENDED" : {...}
 */
static int parse_recept_and_transmission(json_object *obj, unsigned int property_kind)
{
	int err = 0;
    json_object_object_foreach(obj, key, val) {
		if (strcmp(key,"STANDARD") == 0) {
			err += parse_canid_def(val, property_kind);
		} else
		if (strcmp(key,"EXTENDED") == 0) {
//			err += parse_canid_def(val, readonlyProperties);
			WARNMSG("json: EXTENDED block(Extend can frame) is not Supported.");
		} else {
			ERRMSG("json: Unknown key: \"%s\"",key);
			++err;
		}
    }
	return err;
}

/**
 * json form:
 *
 *   "RECEPTION" :   { ... }
 *   "TRANSMISSION": { ... }
 */  
static int parse_json(json_object *obj)
{
	int err = 0;
    json_object_object_foreach(obj, key, val) {
        if (strcmp(key,"RECEPTION") == 0) {
			err += parse_recept_and_transmission(val, CAN_KIND_READONLY);
		} else
		if (strcmp(key,"TRANSMISSION") == 0) {
			err += parse_recept_and_transmission(val, CAN_KIND_SEND);
		} else {
			++err;
			ERRMSG("json: Unknown  key \"%s\"", key);
		}
    }
	return err;
}

int can_define_init(const char *fname)
{
	struct json_object *jobj;

	jobj = json_object_from_file(fname);
	if (jobj == NULL) {
		ERRMSG("cannot data from file \"%s\"",fname);
		return 1;
	}
	parse_json(jobj);

	json_object_put(jobj);

	return 0;
}

struct json_object *property2json(struct can_bit_t *property)
{
	int i;
	struct json_object *jobj;
	struct json_object *valueObject = NULL;
	if (property == NULL)
		return NULL;

	jobj = json_object_new_object();
	if (jobj == NULL)
		return NULL;
	switch(property->var_type) {
	case VOID_T:	/* FALLTHROUGH */
	case INT8_T:	/* FALLTHROUGH */
	case INT16_T:	/* FALLTHROUGH */
	case INT_T:		/* FALLTHROUGH */
	case INT32_T:	/* FALLTHROUGH */
	case INT64_T:	/* FALLTHROUGH */
	case UINT8_T:	/* FALLTHROUGH */
	case UINT16_T:	/* FALLTHROUGH */
	case UINT_T:	/* FALLTHROUGH */
	case UINT32_T:	/* FALLTHROUGH */
	case ENABLE1_T:
		valueObject = json_object_new_int(propertyValue_int(property));
		break;
	case STRING_T:
		if (property->curValue.string != NULL) {
			valueObject = json_object_new_string(property->curValue.string);
		} else {
			valueObject = json_object_new_string("null");
		}
		break;
	case BOOL_T:
		valueObject = json_object_new_boolean(property->curValue.bool_val);
		break;
	case ARRAY_T:
		/* LIMITATION:  Array is int arrays */
		valueObject = json_object_new_array();
		for(i = 0; i < property->curValue.array_values.number_of_entry; i++)
			json_object_array_add(valueObject,
			  json_object_new_int(property->curValue.array_values.array[i]));
		break;
	default:
		ERRMSG("Unknown value type:%d", property->var_type);
		break;
	}
	if (valueObject == NULL) {
		ERRMSG("fail json ValueObject");
		json_object_put(jobj);
		return NULL;
	}

	json_object_object_add(jobj, "value", valueObject);
	return jobj;
}

#ifdef OWN_TEST
static void print_property_info(struct can_bit_t *property, int printParent, int printchain)
{
	struct can_bit_t *next;
	if (printParent) {
		printf("CAN-ID:%03X , DLC:%d, MODE:%d,CYCLUE:%d, [nData=%d]\n",
			property->mycanid->canid,
			property->mycanid->dlc,
			property->mycanid->mode,
			property->mycanid->cycle,
			property->mycanid->nData);
	}
	printf("name=%s, pos=%d, offs=%d, len=%d, type=\"%s\"\n",
		property->name,
		property->byte_pos,
		property->bit_pos,
		property->bit_len,
		vartype2string((unsigned int)property->var_type)
	);
	if (printchain) {
		for (next = property->next; next != NULL; next = next->next) {
			printf("\t-> ");
			print_property_info(next, 0, 0);
		}
	}
}

static void print_canid_info(struct canid_info_t *canid_info)
{
	unsigned int i;
	const unsigned int max = canid_info->nData;
	printf("CAN-ID:%03X , DLC:%d, MODE:%d,CYCLUE:%d, [nData=%d]\n",
		canid_info->canid,
		canid_info->dlc,
		canid_info->mode,
		canid_info->cycle,
		canid_info->nData);
	for ( i = 0; i < max; i++) {
		printf("    [%d] ", i);
		print_property_info(&canid_info->property[i], 0, 1);
	}
}

int main(int argc, char *argv[])
{
	int i;

	if ( argc != 2 ) {
		printf("[Usage] can_json file\n");
		return 1;
	}
	can_define_init(argv[1]);

	for(i=1; i < 80*1000*1000;  i = i << 1) {
		char *p = malloc(i);
		memset(p, 0xff, i);
		free(p);
	}

	canid_walker(print_canid_info);

	printf("=== CANID 0x6E2 Search TEST ===\n");
	print_canid_info(getCANID_dict(0x6E2));
	printf("=== Property \"TransmissionShiftPosition\" Search TEST ===\n");
	print_property_info( getProperty_dict("TransmissionShiftPosition"), 1, 1);

	destroy_dict();
	return 0;
}
#endif
