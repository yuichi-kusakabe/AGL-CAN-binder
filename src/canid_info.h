#ifndef _CANID_INFO_H_
#define _CANID_INFO_H_
#include <sys/types.h>
#include <sys/time.h>
#include <stdint.h>
#include <linux/can/raw.h>

struct canid_info_t;
typedef unsigned char bool_t;
typedef unsigned short jcanid_t;

enum can_property_kind_t {
	CAN_KIND_NONE,
	CAN_KIND_READONLY = 0x01,
	CAN_KIND_SEND    = 0x02
};

enum var_type_t {
/* 0 */	VOID_T,
/* 1 */	INT8_T,
/* 2 */	INT16_T,
/* 3 */	INT_T,
/* 4 */	INT32_T,
/* 5 */	INT64_T,
/* 6 */	UINT8_T,
/* 7 */	UINT16_T,
/* 8 */	UINT_T,
/* 9 */	UINT32_T,
/* 10 */	UINT64_T,
/* 11 */	STRING_T,
/* 12 */	BOOL_T,
/* 13 */	ARRAY_T,
/* 14 */	ENABLE1_T	/* AMB#CANRawPlugin original type */
};

#define STRING_MAX (256)
#define ARRAY_MAX  ((STRING_MAX/sizeof(int)) - 1)
union data_content_t	{
	uint32_t uint32_val;
	int32_t  int32_val;
	uint16_t uint16_val;
	int16_t  int16_val;
	uint8_t  uint8_val;
	int8_t   int8_val;
	bool_t   bool_val;
	char     *string;	/* --> *dataconv[x] */
	struct {
		int	 number_of_entry;
		int  array[ARRAY_MAX];
	} array_values;
};

struct can_bit_t {
	struct canid_info_t *mycanid;
	const char * name;
	unsigned int  property_kind;
	unsigned char property_type;
	unsigned char byte_pos;
	unsigned char bit_pos;
	unsigned char bit_len;
	unsigned char var_type;
#if 0 /* NO-USE : NO-Support */
	unsigned int multi_param;
	unsigned int add_param;
#endif
	struct can_bit_t *next;
	struct can_bit_t *applyinfo;
	void *dataconv;
	unsigned int   nconv;
	struct timeval updatetime;
	union data_content_t curValue;
};
	

/* mode type */
enum { CAN_THINOUT_NON=0x00,
	   CAN_THINOUT_CHG=0x01 };

struct can_sendTimer {
	int timerfd;
};

struct canid_info_t {
	jcanid_t        canid;
	unsigned char  dlc;
	unsigned char  mode;
	unsigned int   cycle;
	struct can_frame canraw_frame; /* latest readed frame */
	struct can_sendTimer sendTimer;
	unsigned int   kind;
	unsigned int   nData;
	struct can_bit_t property[0];
	/* This is variable structure */
};
typedef enum { UPSTAT_ERROR = -1, UPSTAT_NO_UPDATED = 0, UPSTAT_UPDATED = 1 } update_stat_t;
extern const char * vartype2string(unsigned int t);
extern int string2vartype(const char *varname);
extern update_stat_t canframe2property(uint64_t can_v64, struct can_bit_t *property_info);
extern int propertyValue_int(struct can_bit_t *property_info);
extern update_stat_t property2canframe(struct can_bit_t *property_info, unsigned int value);
extern uint64_t canvalue2host(unsigned char *data);
extern void host2canvalue(uint64_t v,unsigned char *outdata);
#endif
