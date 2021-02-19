/*
 * usdl_types.h
 *
 *  Created on: 8 янв. 2021 г.
 *      Author: HP
 */

#ifndef SDL_USDL_USDL_TYPES_H_
#define SDL_USDL_USDL_TYPES_H_

#include <stdbool.h>
#include <inttypes.h>


#include "usdl_defines.h"
#include "usdl_basic_types.h"
#include "multiplexer.h"

typedef struct {
	uint8_t *data;
	size_t size;
	size_t max_size;
	size_t index;
	union {
		fhd_t fhd;
		lvo_t lvo;
	};
	tfdz_construction_rules_t rules;
} map_buffer_t;






typedef struct map_t map_t;
typedef struct vc_t vc_t;
typedef struct mc_t mc_t;
typedef struct pc_t pc_t;

typedef enum {
	MAPR_STATE_BEGIN,
	MAPR_STATE_SIZE_UNKNOWN,
	MAPR_STATE_SIZE_KNOWN,
	MAPR_STATE_FINISH
} mapr_state_t;

typedef struct mapr_t {
	map_buffer_t packet;
	map_buffer_t tfdz;
	mapr_state_t state;

	tfdz_construction_rules_t rules;
} mapr_t;

typedef struct map_t {
	usdl_node_t base;
	void (*map_parse)(map_t *map, uint8_t *data, size_t size, map_params_t *map_params);
	int (*map_request_from_down)(map_t *map);

	mapr_t mapr;
	map_id_t map_id;
	vc_t *vc;
	bool size_fixed;
	bool split_allowed;
	quality_of_service_t qos;

	map_buffer_t buf_ex;
	map_buffer_t buf_sc;
	map_buffer_t buf_packet;
	quality_of_service_t packet_qos;
} map_t;


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
	usdl_node_t base;
	transfer_frame_t transfer_frame_type;
	vc_id_t vc_id;
	vcf_count_t frame_count_expedited;
	vcf_count_t frame_count_seq_control;
	vc_parameters_t vc_parameters;
	fop_t fop;
	mc_t *mc;
	struct multiplexer_abstract *map_mx;
	map_t *map_arr[MAP_COUNT];

	uint32_t mapcf_length_ex;
	uint32_t mapcf_length_sc;
} vc_t;



typedef enum {
	MC_OCF_PROHIBTED,
	MC_OCF_ALLOWED,
	MC_OCF_REQUIRED
} mc_ocf_t;

typedef struct {
	uint16_t scid;
	transfer_frame_t tft;
	sod_flag_t sod_flag;
	mc_ocf_t ocf_flag;
} mc_paramaters_t;


typedef struct mc_t {
	usdl_node_t base;
	mc_id_t mc_id;
	pc_t *pc;
	vc_t *vc_arr[VC_COUNT];
	struct multiplexer_abstract *vc_mx;
	mc_paramaters_t mc_parameters;
	uint8_t ocf[4];
	uint32_t vcf_length;
} mc_t;

typedef struct {
	transfer_frame_t tft;
	int tf_length;
	tfvn_t tfvn;
	uint8_t fec_length;
	bool is_fec_presented;
	bool can_generate_oid;

} pc_paramaters_t;

typedef struct pc_t {
	usdl_node_t base;
	uint8_t *data;
	uint8_t size;
	int is_valid;

	mc_t *mc_arr[MC_COUNT];
	struct multiplexer_abstract *mc_mx;
	pc_paramaters_t pc_parameters;
	uint8_t *insert_data;
	size_t insert_size;
	uint8_t fec_field[4];
	uint32_t mcf_length;
} pc_t;

#endif /* SDL_USDL_USDL_TYPES_H_ */
