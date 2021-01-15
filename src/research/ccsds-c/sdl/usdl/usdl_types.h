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
#include "usdl_basic_types.h"




typedef struct map_mx_t map_mx_t;
typedef struct map_t map_t;
typedef struct vc_t vc_t;
typedef struct vc_mx_t vc_mx_t;


typedef struct map_t {
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



typedef struct map_mx_t {
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

typedef struct queue_element_t queue_element_t;

typedef struct queue_element_t{
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

typedef struct vc_t {
	transfer_frame_t transfer_frame_type;
	vc_id_t vc_id;
	vcf_count_length_t vcf_count_length;
	vcf_count_t frame_count_expedited;
	vcf_count_t frame_count_seq_control;
	sod_flag_t src_or_dest_id;
	vc_parameters_t parameters;
	fop_t fop;
	vc_mx_t *vc_mx;
} vc_t;



typedef struct vc_mx_t {

} vc_mx_t;

#endif /* SDL_USDL_USDL_TYPES_H_ */
