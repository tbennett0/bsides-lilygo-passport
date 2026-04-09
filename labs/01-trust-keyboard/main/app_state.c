/*
 * app_state.c — State name helper.
 */

#include "app_state.h"

const char *app_state_name(app_state_t state)
{
    switch (state) {
    case APP_STATE_INIT:   return "INIT";
    case APP_STATE_READY:  return "READY";
    case APP_STATE_TYPING: return "TYPING";
    case APP_STATE_DONE:   return "DONE";
    default:               return "???";
    }
}
