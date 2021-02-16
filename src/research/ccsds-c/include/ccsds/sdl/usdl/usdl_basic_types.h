/*
 * usdl_basic_types.h
 *
 *  Created on: 15 янв. 2021 г.
 *      Author: HP
 */

#ifndef SDL_USDL_USDL_BASIC_TYPES_H_
#define SDL_USDL_USDL_BASIC_TYPES_H_

#include <inttypes.h>

typedef uint16_t fhd_t;
typedef uint16_t lvo_t;


typedef enum {
	QOS_SEQ_CONTROL,
	QOS_EXPEDITED
} quality_of_service_t;

typedef uint8_t tfvn_t;
typedef uint8_t map_id_t;
typedef enum {
	TFDZCR_SPAN 		= 0x0,
	TFDZCR_SDU_START 	= 0x1,
	TFDZCR_SDU_CONT 	= 0x2,
	TFDZCR_STREAM 		= 0x3,
	TFDZCR_SEGM_START 	= 0x4,
	TFDZCR_SEGM_CONT 	= 0x5,
	TFDZCR_SEGM_LAST 	= 0x6,
	TFDZCR_NO_SEGM 		= 0x7
} tfdz_construction_rules_t;

//SANA UPID registry
typedef enum {
	UPID_SP_OR_EP	 	= 0b00000,
	UPID_COP_1 			= 0b00001,
	UPID_COP_P 			= 0b00010,
	UPID_SDLS 			= 0b00011,
	UPID_STREAM 		= 0b00100,
	UPID_MAPA_SDU 		= 0b00101,
	UPID_PROXIMITY_1 	= 0b00111,
	UPID_IDLE 			= 0b11111
} upid_t;

typedef enum {
	PVN_SPACE_PACKET 			= 0x0,
	PVN_ENCAPSULATION_PROTOCOL 	= 0x7,
} pvn_t;


typedef enum {
	MS_LAST,
	MS_START,
	MS_CONT,
} map_state_t;


typedef uint8_t vc_id_t;

typedef struct {
	map_id_t map_id;
	upid_t upid;

	union{
		fhd_t fhd;
		lvo_t lvo;
	};
	tfdz_construction_rules_t rules;
	quality_of_service_t qos;
} map_params_t;


/*
 * Bypass/Sequence-control flag
 */
typedef enum {
	BSC_SEQ_CONTROL 	= 0,
	BSC_EXPEDITED 		= 1
} bsc_flag_t;

/*
 * Protocol control command flag
 */
typedef enum {
	PCC_USER_DATA 		= 0,
	PCC_PROTOCOL_INFO 	= 1
} pcc_flag_t;

/*
 * Operational control field presence flag
 */
typedef enum {
	OCFP_ABSENT 		= 0,
	OCFP_PRESENT 	= 1
} ocfp_flag_t;

/*
 * Source or destination flag
 */
typedef enum {
	SOD_SOURCE 			= 0,
	SOD_DESTINATION 	= 1
} sod_flag_t;

/*
 * End of Frame Primary Header Flag
 */
typedef enum {
	EOFPH_FULL = 0,
	EOFPH_TRUNCATED = 1
} eofph_flag_t;

typedef uint8_t vcf_count_length_t;
typedef uint64_t vcf_count_t;

typedef enum {
	TF_FIXED,
	TF_VARIABLE
} transfer_frame_t;

typedef enum {
	COP_NONE,
	COP_1,
	COP_P
} cop_in_effect_t;

typedef uint8_t clcw_version_t;
typedef uint8_t clcw_report_rate_t;

enum map_mx_state_t {
	MMX_FIXED,
	MMX_FREE
};


typedef uint16_t sc_id_t;

typedef struct vc_params_t {
	vc_id_t vc_id;
	bsc_flag_t bsc_flag;
	pcc_flag_t pcc_flag;
	vcf_count_length_t vcf_count_length;
	vcf_count_t vcf_count;
	eofph_flag_t eofph_flag;
} vc_params_t;

typedef uint8_t mc_id_t;

typedef struct mc_params_t {
	sod_flag_t sod_flag;
	ocfp_flag_t ocfp_flag;
	uint8_t ocf[4];
	mc_id_t mc_id;
	uint16_t scid; //Переместить в pc_params
} mc_params_t;

typedef struct usdl_node_t {
	int (*request_from_down)(struct usdl_node_t *node);
} usdl_node_t;



#endif /* SDL_USDL_USDL_BASIC_TYPES_H_ */


