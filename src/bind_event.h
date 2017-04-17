#ifndef _BIND_EVENT_H_
#define _BIND_EVENT_H_
#include <afb/afb-binding.h>
#include <linux/can.h>
#include "canid_info.h"
struct bind_event_t {
	const char *name;       /* name of the event : IvI Property-Name */
	struct afb_event event; /* the event for the binder              */
	struct can_bit_t *raw_info;
};

extern int getSubscribedCanids(canid_t **p);
extern struct bind_event_t *get_event(const char *name);
extern struct bind_event_t *register_event(const char *name);
extern void bind_event_walker(void (*handler)(struct bind_event_t *entry));
extern int remove_event(const char *name);
#endif

