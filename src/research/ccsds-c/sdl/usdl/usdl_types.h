/*
 * usdl_types.h
 *
 *  Created on: 8 џэт. 2021 у.
 *      Author: HP
 */

#ifndef SDL_USDL_USDL_TYPES_H_
#define SDL_USDL_USDL_TYPES_H_

#include <stdbool.h>
#include <inttypes.h>

#include "usdl_defines.h"


typedef struct {
	uint8_t *data;
	size_t size;
	size_t index;
	size_t fhd;
} map_buffer;


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


typedef struct {
	map_id_t map_id;
	map_mx_t *map_mx;
	bool size_fixed;
	bool split_allowed;
	quality_of_service_t qos;

	tfdz_construction_rules_t flags;
	map_buffer buf_ex;
	map_buffer buf_sc;
#if MAP_BUFFER_SIZE > 0 && defined(MAP_BUFFER_STATIC)
	//uint8_t buffer[MAP_BUFFER_SIZE];
	size_t payload_size;
	//size_t buffer_index;
#elif !defined(MAP_BUFFER_STATIC)
	uint8_t *buffer;
	size_t buffer_index;
	size_t payload_size;
#endif
} map_t;

typedef uint8_t vc_id_t;
typedef uint16_t fhd_t;

typedef struct {
	map_id_t map_id;
	upid_t upid;
	fhd_t fhd;
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

typedef uint8_t vcf_count_length_t;
typedef uint64_t vcf_count_t;
typedef struct {
	uint8_t header[14];
	uint8_t header_size;

} vc_params_t;

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

typedef struct vc_t;


typedef struct {
	map_id_t map_active;
	enum map_mx_state_t state;
	map_t *map_arr[MAP_COUNT];
	vc_t *vc;
} map_mx_t;


#define FOP_DATA_SIZE 200
#define FOP_QUEUE_SIZE 10

typedef enum {
	FOP_STATE_ACTIVE,
	FOP_STATE_RETR_WO_WAIT,
	FOP_STATE_RETR_W_WAIT,
	FOP_STATE_INIT_WO_BCF,
	FOP_STATE_INIT_W_BCF,
	FOP_STATE_INITIAL,
} fop_state_t;

typedef struct {
	uint8_t data[FOP_DATA_SIZE];
	size_t size;
	map_params_t map_params;
	vc_params_t vc_params;
} queue_value_t;

typedef struct queue_value_t;

typedef struct {
	queue_value_t value;
	queue_element_t *next;
} queue_element_t;

typedef struct {
	queue_element_t buf[FOP_QUEUE_SIZE];
	size_t size;
	queue_element_t *empty;
	queue_element_t *first;
	queue_element_t *last;
} queue_t;
typedef struct {
	queue_t queue;
	fop_state_t state;
	size_t frame_count;
} fop_t;



typedef struct {

	transfer_frame_t transfer_frame_type;
	vc_id_t vc_id;
	vcf_count_length_t vcf_count_lenght_expedited;
	vcf_count_length_t vcf_count_lenght_seq_control;
	cop_in_effect_t cop_in_effect;
	clcw_version_t clcw_version;
	clcw_report_rate_t clcw_report_rate;
	map_t *map_set[16];
	uint32_t truncated_frame_length;
	union {
		bool ocf_allowed;
		bool ocf_required;
	};
	uint8_t repetition_max_user;
	uint8_t repetition_max_cop;
	uint32_t max_delay_to_release;
	uint32_t max_delay_between_frames;
} vc_parameters_t;

typedef struct {
	transfer_frame_t transfer_frame_type;
	vc_id_t vc_id;
	vcf_count_length_t vcf_count_length;
	vcf_count_t frame_count_expedited;
	vcf_count_t frame_count_seq_control;
	const sod_flag_t src_or_dest_id = SOD_SOURCE;
	vc_parameters_t parameters;
	fop_t fop;
} vc_t;

typedef struct {
	sod_flag_t sod_flag;
	vc_id_t vc_id;
	bsc_flag_t bsc_flag;
	pcc_flag_t pcc_flag;
	vcf_count_length_t vcf_count_length;
	vcf_count_t vcf_count;
} vc_params_t;

#endif /* SDL_USDL_USDL_TYPES_H_ */
