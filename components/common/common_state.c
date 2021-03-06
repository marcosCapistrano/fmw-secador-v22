#include "common_state.h"

#include <stdio.h>

state_msg_t state_msg_create_empty() {
	state_msg_t state_msg = (state_msg_t) malloc(sizeof(s_state_msg_t));

	state_msg->msg_type = STATE_MSG_NONE;
	state_msg->target = STATE_TARGET_NONE;
	state_msg->value = -1;
	
	return state_msg;
}

state_msg_t state_msg_create(state_msg_type_t msg_type, state_target_t target, uint8_t value) {
	state_msg_t state_msg = (state_msg_t) malloc(sizeof(s_state_msg_t));

	state_msg->msg_type = msg_type;
	state_msg->target = target;
	state_msg->value = value;
	
	return state_msg;
}
