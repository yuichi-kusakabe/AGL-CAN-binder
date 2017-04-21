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
#include <string.h>
#include <stdint.h>
#include "canid_info.h"
#include "error.h"

/* [Limitation] ONLY STANDARD Frame support*/
#define BITS_OF_BYTE (8)
#define BYTE_OF_FRAME (8)

#define ENABLE1_TYPENAME "ENABLE-1"	/* spec. type1.json original type name */

static const char *vartype_strings[] = {
/* 0 */	"void",			
/* 1 */	"int8_t",
/* 2 */	"int16_t",
/* 3 */	"int",
/* 4 */	"int32_t",
/* 5 */	"int64_t",
/* 6 */	"uint8_t",
/* 7 */	"uint16_t",
/* 8 */	"uint",
/* 9 */	"uint32_t",
/* 10 */"uint64_t",
/* 11 */	"string",
/* 12 */	"bool",
/* 13 */	"LIST",
/* 14 */ ENABLE1_TYPENAME
};

const char * vartype2string(unsigned int t)
{
	if (t > (sizeof(vartype_strings)/sizeof(char *))) {
		ERRMSG("Invalid vartype: %#x", t);
		return NULL;
	}
	return vartype_strings[t];
}

int string2vartype(const char *varname)
{
	unsigned int i;
	if (varname == NULL)
		return -1;

	for(i=0; i < (sizeof(vartype_strings)/sizeof(char *)); i++) {
		if (strcmp(varname, vartype_strings[i]) == 0)
			return (int)i;
	}
	return -1;
};

uint64_t canvalue2host(unsigned char *data)
{
	union {
		uint64_t value64;
		unsigned char b[BYTE_OF_FRAME]; } can_bigendian_data;
	can_bigendian_data.b[0] = data[7];
	can_bigendian_data.b[1] = data[6];
	can_bigendian_data.b[2] = data[5];
	can_bigendian_data.b[3] = data[4];
	can_bigendian_data.b[4] = data[3];
	can_bigendian_data.b[5] = data[2];
	can_bigendian_data.b[6] = data[1];
	can_bigendian_data.b[7] = data[0];
	return can_bigendian_data.value64;
}

void host2canvalue(uint64_t v,unsigned char *outdata)
{
	union {
		uint64_t value64;
		unsigned char b[BYTE_OF_FRAME]; } can_bigendian_data;
	can_bigendian_data.value64 = v;

	outdata[7] = can_bigendian_data.b[0];
	outdata[6] = can_bigendian_data.b[1];
	outdata[5] = can_bigendian_data.b[2];
	outdata[4] = can_bigendian_data.b[3];
	outdata[3] = can_bigendian_data.b[4];
	outdata[2] = can_bigendian_data.b[5];
	outdata[1] = can_bigendian_data.b[6];
	outdata[0] = can_bigendian_data.b[7];
}

static int _updatePropertyValue(struct can_bit_t *property_info, uint32_t v)
{
	unsigned int x = v;
	int update = 0;
	const int is_dataconvert = ((property_info->dataconv != NULL) && (property_info->nconv > x ));
	switch(property_info->var_type) {
	case INT8_T:
		if (!is_dataconvert) {
			if (property_info->curValue.int8_val != (int8_t)x) {
				property_info->curValue.int8_val = (int8_t)x;
				update = 1;
			}
		} else {
			int32_t *conv = (int32_t *)property_info->dataconv;
			if (property_info->curValue.int8_val != (int8_t)conv[x]) {
				property_info->curValue.int8_val = (int8_t)conv[x];
				update = 1;
			}
		}
		break;
	case INT16_T:
		if (!is_dataconvert) {
			if (property_info->curValue.int16_val != (int16_t)x) {
				property_info->curValue.int16_val = (int16_t)x;
				update = 1;
			}
		} else {
			int32_t *conv = (int32_t *)property_info->dataconv;
			if (property_info->curValue.int16_val != (int16_t)conv[x]) {
				property_info->curValue.int16_val = (int16_t)conv[x];
				update = 1;
			}
		}
		break;
	case VOID_T:/* FALLTHROGH */
	case INT_T:	/* FALLTHROGH */
	case INT32_T:/* FALLTHROGH */
		if (!is_dataconvert) {
			if (property_info->curValue.int32_val != (int32_t)x) {
				property_info->curValue.int32_val = (int32_t)x;
				update = 1;
			}
		} else {
			int32_t *conv = (int32_t *)property_info->dataconv;
			if (property_info->curValue.int32_val != conv[x]) {
				property_info->curValue.int32_val = (int32_t)conv[x];
				update = 1;
			}
		}
		break;
	case UINT8_T:
		if (!is_dataconvert) {
			if (property_info->curValue.uint8_val != (int8_t)x) {
				property_info->curValue.uint8_val = (uint8_t)x;
				update = 1;
			}
		} else {
			uint32_t *conv = (uint32_t *)property_info->dataconv;
			if (property_info->curValue.uint8_val != (int8_t)conv[x]) {
				property_info->curValue.uint8_val = (uint8_t)conv[x];
				update = 1;
			}
		}
		break;
	case UINT16_T:
		if (!is_dataconvert) {
			if (property_info->curValue.uint16_val != (uint16_t)x) {
				property_info->curValue.uint16_val = (uint16_t)x;
				update = 1;
			}
		} else {
			uint32_t *conv = (uint32_t *)property_info->dataconv;
			if (property_info->curValue.uint16_val != (int16_t)conv[x]) {
				property_info->curValue.uint16_val = (uint16_t)conv[x];
				update = 1;
			}
		}
		break;
	case UINT_T:
	case UINT32_T:
		if (!is_dataconvert) {
			if (property_info->curValue.uint32_val != (uint32_t)x) {
				property_info->curValue.uint32_val = (uint32_t)x;
				update = 1;
			}
		} else {
			uint32_t *conv = (uint32_t *)property_info->dataconv;
			if (property_info->curValue.uint32_val != conv[x]) {
				property_info->curValue.uint32_val = conv[x];
				update = 1;
			}
		}
		break;
	case BOOL_T:
		if (!is_dataconvert) {
			if (property_info->curValue.bool_val != (!!x)) {
				property_info->curValue.bool_val = !!x ;
				update = 1;
			}
		} else  {
			int *conv = (int *)property_info->dataconv;
			if (property_info->curValue.bool_val != conv[x]) {
				property_info->curValue.bool_val = !! conv[x];
				update = 1;
			}
		}
		break;

	case STRING_T:
		if (is_dataconvert) {
			char **conv = (char **)property_info->dataconv;
			if (property_info->curValue.string != conv[x]) {
					property_info->curValue.string = conv[x];
		
				if (strcmp(conv[x], "NOTSET") != 0) {
					update = 1;
				} 
			}
		} else {
			ERRMSG("Cannot string conversion: CAN-ID(%03X)#%s frame-Value:%d Check SET-DATA fileld", 
				property_info->mycanid->canid,
				property_info->name,
				x);
		}
		break;
	case ENABLE1_T:
		if (is_dataconvert) {
			int *conv = (int *)property_info->dataconv;
			if (property_info->curValue.int32_val != conv[x]) {
				property_info->curValue.int32_val = conv[x];

				if (x != 0) 
					update = 1;
			}
		} else {
			ERRMSG("Cannot substitution-value conversion: CAN-ID(%03X)#%s frame-Value:%d Check SET-DATA fileld", 
				property_info->mycanid->canid,
				property_info->name,
				x);
		}
		break;
	default:
	case ARRAY_T:
	case UINT64_T:
	case INT64_T:
		ERRMSG("NOT SUPPORT vartype contents:%d", property_info->var_type);
		break;
	}

	return update; /* 0: NOT-Updated  1: UPdated */
}

int property2canframe(struct can_bit_t *property_info, unsigned int value)
{
	const int bit_shift = ((BYTE_OF_FRAME - property_info->byte_pos) * BITS_OF_BYTE) + property_info->bit_pos;
	uint64_t can_mask64 = (1ULL << property_info->bit_len)-1;
	uint64_t can_v64;
	uint64_t can_curvalue;
	int update;
	update = _updatePropertyValue(property_info, value);
	if (!update) {
		NOTICEMSG("%s property is not updated", property_info->name);
		return 1;
	}
	can_v64 = ((uint64_t)value & can_mask64) << bit_shift;
	can_curvalue  = canvalue2host(property_info->mycanid->canraw_frame.data);
	can_curvalue &= ~(can_mask64 << bit_shift);
	can_curvalue |= can_v64;
	
	host2canvalue(can_curvalue, property_info->mycanid->canraw_frame.data);
	return 0;
}

int canframe2property(uint64_t can_v64, struct can_bit_t *property_info)
{
	int update = 0;
	uint64_t can_mask64;
	const int bit_shift = ((BYTE_OF_FRAME - property_info->byte_pos) * BITS_OF_BYTE) + property_info->bit_pos;
	can_mask64 = (1ULL << property_info->bit_len)-1;
	can_v64 = can_v64 >> bit_shift;
	can_v64 &= can_mask64;

	update = _updatePropertyValue(property_info, (uint32_t) can_v64);

	return update; /* 0: NOT-Updated  1: UPdated */
}

int propertyValue_int(struct can_bit_t *property_info) 
{
	int x = -1;
	switch(property_info->var_type) {
	case INT8_T:
		x = (int)property_info->curValue.int8_val;
		break;
	case INT16_T:
		x =	(int)property_info->curValue.int16_val;
		break;
	case VOID_T:/* FALLTHROGH */
	case INT_T:	/* FALLTHROGH */
	case INT32_T:/* FALLTHROGH */
		x =	(int)property_info->curValue.int32_val;
		break;
	case UINT8_T:
		x =	(int)property_info->curValue.uint8_val;
		break;
	case UINT16_T:
		x =	(int)property_info->curValue.uint16_val;
		break;
	case UINT_T:
	case UINT32_T:
		x = (int)property_info->curValue.uint32_val;
		break;
	case BOOL_T:
		x = (int)property_info->curValue.bool_val;
		break;
	case ENABLE1_T:
		x = (int)property_info->curValue.int32_val;
		break;

	default:
	case STRING_T:
	case ARRAY_T:
	case UINT64_T:
	case INT64_T:
		ERRMSG("Getting property Value:NOT SUPPORT vartype contents:%s %d", property_info->name, property_info->var_type);
		break;
	}
	return x;
}
