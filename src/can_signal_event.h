#ifndef _CAN_SIGNAL_EVENT_H_
#define _CAN_SIGNAL_EVENT_H_
#include "canid_info.h"
enum value_type_t {
	VALUE_TYPE_VOID,
	VALUE_TYPE_UINT,
	VALUE_TYPE_INT
};

struct can_signal_t {
	const char *name;
	enum value_type_t v_type;
	union v_t {
		unsigned int v_uint;
		int v_int;
	} v;
};


extern int  can_signal_read(int fd, struct can_signal_t *out);
extern void can_signal_send(struct can_bit_t *prop, unsigned int v);

extern void	addSendTimer(struct canid_info_t *caninfo);
extern void updateSendTimer(struct canid_info_t *caninfo);
extern void stopSendTimer(struct canid_info_t *caninfo);
#endif
