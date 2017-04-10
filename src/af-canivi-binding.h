#ifndef _AF_CANIVI_BINDING_H_
#define _AF_CANIVI_BINDING_H_
#include <afb/afb-binding.h>
#include <afb/afb-service-itf.h>
#include "canid_info.h"

extern const struct afb_binding_interface *afbitf;

#if !defined(NO_BINDING_VERBOSE_MACRO)	
#define	ERRMSG(msg, ...) ERROR(afbitf,msg,##__VA_ARGS__)
#define	WARNMSG(msg, ...) WARNING(afbitf,msg,##__VA_ARGS__)
#define	DBGMSG(msg, ...) DEBUG(afbitf,msg,##__VA_ARGS__)
#define	INFOMSG(msg, ...) INFO(afbitf,msg,##__VA_ARGS__)
#define NOTICEMSG(msg, ...) NOTICE(afbitf,msg,##__VA_ARGS__)
#endif

extern int notify_property_changed(struct can_bit_t *property_info);
#endif
