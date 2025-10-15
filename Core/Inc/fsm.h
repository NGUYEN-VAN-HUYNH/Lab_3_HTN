/*
 * fsm.h
 */

#ifndef INC_FSM_H_
#define INC_FSM_H_

#include "lcd.h"        // for lcd functions
#include "button.h"     // for button_count
#include "software_timer.h" // for timers
#include "main.h"       // for HAL_GPIO etc. if needed

// Enum for all states from FSM diagram
typedef enum {
    STATE_INIT,
    STATE_MODE1,
    STATE_MODE2,
    STATE_MODE3,
    STATE_MODE4,
    STATE_MODE5,
    STATE_RED_EDIT,
    STATE_RED_SAVE,
    STATE_GREEN_EDIT,
    STATE_GREEN_SAVE,
    STATE_YELLOW_EDIT,
    STATE_YELLOW_SAVE,
    STATE_RED_GREEN_MAN,
    STATE_RED_YELLOW_MAN,
    STATE_GREEN_RED_MAN,
    STATE_YELLOW_RED_MAN,
    STATE_RED_GREEN,
    STATE_RED_YELLOW,
    STATE_GREEN_RED,
    STATE_YELLOW_RED,
    STATE_A_RED_B_GREEN,
    STATE_A_RED_B_YELLOW,
    STATE_A_GREEN_B_RED,
    STATE_A_YELLOW_B_RED
} State;

// Function prototypes
void FSM_process(void);

// Extern variables
extern State current_state;
extern uint8_t red_duration;
extern uint8_t green_duration;
extern uint8_t yellow_duration;
extern uint16_t sch_counter;

#endif /* INC_FSM_H_ */
