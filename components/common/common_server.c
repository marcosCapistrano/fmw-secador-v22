#include "common_server.h"

#include <stdio.h>

server_update_t server_update_create(server_event_t type, uint8_t value) {
	server_update_t server_update = (server_update_t) malloc(sizeof(s_server_update_t));

	server_update->type = type;
	server_update->value = value;

	return server_update;
}
