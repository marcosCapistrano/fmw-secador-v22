#ifndef COMMON_SERVER_H
#define COMMON_SERVER_H

#include "inttypes.h"

typedef enum server_event_t {
	MASSA_21,
	MASSA_22
} server_event_t;

typedef struct s_server_update_t *server_update_t;
typedef struct s_server_update_t {
	server_event_t type;
	uint8_t value;
} s_server_update_t;

server_update_t server_update_create(server_event_t type, uint8_t value);

#endif