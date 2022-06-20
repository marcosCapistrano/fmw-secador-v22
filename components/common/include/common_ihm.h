#ifndef COMMON_PERIF_H
#define COMMON_PERIF_H

#include "inttypes.h"

typedef enum ihm_event_t {
	EVENT_NONE, 
	PAGE,
	VALUE,
} ihm_event_t;

typedef struct s_ihm_update_t *ihm_update_t;
typedef struct s_ihm_update_t {
	ihm_event_t type;
	uint8_t target_id;
	uint8_t value;
} s_ihm_update_t;

ihm_update_t ihm_update_create(ihm_event_t type, uint8_t target_id, uint8_t value);

#endif