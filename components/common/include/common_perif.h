#ifndef COMMON_perif_H
#define COMMON_perif_H

#include "inttypes.h"

typedef enum perif_event_t {
	ACT,
	PERIF_RESPONSE
} perif_event_t;

typedef enum perif_t {
	PERIF_NONE,
	QUEIMADOR,
	BUZINA,
	LED_ENTRADA_QUENTE,
	LED_ENTRADA_FRIO,
	LED_MASSA_1_QUENTE,
	LED_MASSA_1_FRIO,
	LED_MASSA_2_QUENTE,
	LED_MASSA_2_FRIO,
	LED_CONEXAO,
} perif_t;

typedef enum perif_resp_t {
	PERIF_RESP_NONE,
	MASSA_1,
	MASSA_2
} perif_resp_t;

typedef struct s_perif_update_t *perif_update_t;
typedef struct s_perif_update_t {
	perif_event_t type;
	perif_t target;
	perif_resp_t resp_type;
	int64_t value;
} s_perif_update_t;

perif_update_t perif_update_create(perif_event_t type, perif_t target, perif_resp_t resp_type, int64_t value);

#endif