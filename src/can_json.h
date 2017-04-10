#ifndef _CAN_JSON_H_
#define _CAN_JSON_H_
#include <json-c/json.h>
#include "canid_info.h"
extern int can_define_init(const char *fname);
extern struct json_object *property2json(struct can_bit_t *property);
#endif
