#ifndef _CANID_SEARCH_H_
#define _CANID_SEARCH_H_
#include "canid_info.h"
extern int addCANID_dict(struct canid_info_t *canid);
extern void canid_walker(void (*walker_action)(struct canid_info_t *));
extern struct canid_info_t * getCANID_dict(jcanid_t canid);
extern struct can_bit_t * getProperty_dict(const char *propertyName);
extern void destroy_dict();
const char **getSupportPropertiesList(int *nEntry);
#endif
