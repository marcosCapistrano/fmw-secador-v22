#ifndef COMMON_PERIF_H
#define COMMON_PERIF_H

#include "inttypes.h"

typedef enum ihm_event_t {
	EVENT_NONE, 
	PAGE,
	VALUE,
} ihm_event_t;

typedef enum ihm_target_t {
	TARGET_NONE,
	TARGET_MODE,
	TARGET_LOTE_NUMBER,
	TARGET_ENTRADA,
	TARGET_MASSA_1,
	TARGET_MASSA_2,
	TARGET_CONEXAO_1,
	TARGET_CONEXAO_2,
	TARGET_ENTRADA_MIN,
	TARGET_ENTRADA_MAX,
	TARGET_MASSA_1_MIN,
	TARGET_MASSA_1_MAX,
	TARGET_MASSA_2_MIN,
	TARGET_MASSA_2_MAX,
} ihm_target_t;

typedef struct s_ihm_update_t *ihm_update_t;
typedef struct s_ihm_update_t {
	ihm_event_t type;
	ihm_target_t target_id;
	uint8_t value;
} s_ihm_update_t;

ihm_update_t ihm_update_create(ihm_event_t type, ihm_target_t target_id, uint8_t value);

#endif