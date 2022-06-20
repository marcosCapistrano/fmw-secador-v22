#include "common_perif.h"

#include <stdio.h>

perif_update_t perif_update_create(perif_event_t type, perif_t target, perif_resp_t resp_type, uint8_t value) {
	perif_update_t perif_update = (perif_update_t) malloc(sizeof(s_perif_update_t));

	perif_update->type = type;
	perif_update->target = target;
	perif_update->resp_type = resp_type;
	perif_update->value = value;

	return perif_update;
}
