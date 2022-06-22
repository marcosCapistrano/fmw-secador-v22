#include "common_ihm.h"

#include <stdio.h>

ihm_update_t ihm_update_create(ihm_event_t type, ihm_target_t target_id, uint8_t value) {
	ihm_update_t ihm_update = (ihm_update_t) malloc(sizeof(s_ihm_update_t));

	ihm_update->type = type;
	ihm_update->target_id = target_id;
	ihm_update->value = value;

	return ihm_update;
}
