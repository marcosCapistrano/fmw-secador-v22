#ifndef COMMON_STATE_H
#define COMMON_STATE_H

#include "inttypes.h"

typedef enum state_msg_type_t {
	STATE_MSG_NONE,
	PERIPHERAL_UPDATE,
	UPDATE,
	REQUEST,
	RESPONSE
} state_msg_type_t;

typedef enum state_target_t {
	STATE_TARGET_NONE,
	MODE,
	LOTE_NUMBER,

	SENSOR_ENTRADA,
	SENSOR_MASSA_1,
	LAST_SENSOR_MASSA_1,
	SENSOR_MASSA_2,
	LAST_SENSOR_MASSA_2,
	BUZINA,
	CONEXAO_1,
	CONEXAO_2,
	
	ENTRADA_MIN,
	ENTRADA_MAX,
	MASSA_1_MIN,
	MASSA_1_MAX,
	MASSA_2_MIN,
	MASSA_2_MAX,

	FINISHED,
	CENTRAL
} state_target_t;

typedef struct s_state_msg_t *state_msg_t;
typedef struct s_state_msg_t {
	state_msg_type_t msg_type;
	state_target_t target;
	uint8_t value;
} s_state_msg_t;

state_msg_t state_msg_create(state_msg_type_t msg_type, state_target_t target, uint8_t value);

#endif