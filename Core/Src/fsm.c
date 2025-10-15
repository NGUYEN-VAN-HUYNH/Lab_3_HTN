/*
 * fsm.c
 */

#include "fsm.h"
#include <string.h>

// Variables
State current_state = STATE_INIT;
uint8_t red_duration = 5;    // 5s đỏ
uint8_t green_duration = 3;  // 3s xanh
uint8_t yellow_duration = 2; // 2s vàng
uint16_t sch_counter_A = 0;  // Đếm ngược cho đèn A
uint16_t sch_counter_B = 0;  // Đếm ngược cho đèn B
uint8_t tick_count = 0;      // accumulate for 1s (20*50ms)
uint8_t blink_count = 0;     // for 0.5s blink (10*50ms)
uint8_t blink_flag = 0;      // 0 off, 1 on
uint16_t init_timeout = 100; // 100*50ms = 5s auto start in INIT

// Flags from buttons (local, reset after use)
uint8_t button1_flag = 0;
uint8_t button2_flag = 0;
uint8_t button3_flag = 0;

// Lamp positions on LCD (tối ưu cho giao diện thẩm mỹ)
#define LAMP_RADIUS 25
#define LAMP_A_RED_X 90
#define LAMP_A_RED_Y 80
#define LAMP_A_GREEN_X 90
#define LAMP_A_GREEN_Y 160
#define LAMP_A_YELLOW_X 90
#define LAMP_A_YELLOW_Y 240
#define LAMP_B_RED_X 150
#define LAMP_B_RED_Y 80
#define LAMP_B_GREEN_X 150
#define LAMP_B_GREEN_Y 160
#define LAMP_B_YELLOW_X 150
#define LAMP_B_YELLOW_Y 240
#define STATUS_BAR_Y 20 // Vùng hiển thị số đếm và chế độ ở dưới cùng (giả định màn hình 320px cao)
#define STATUS_BAR_HEIGHT 40 // Chiều cao vùng status bar

// Function to draw all lamps off
void draw_lamps_off(void) {
    lcd_draw_circle(LAMP_A_RED_X, LAMP_A_RED_Y, BLACK, LAMP_RADIUS, 1);
    lcd_draw_circle(LAMP_A_GREEN_X, LAMP_A_GREEN_Y, BLACK, LAMP_RADIUS, 1);
    lcd_draw_circle(LAMP_A_YELLOW_X, LAMP_A_YELLOW_Y, BLACK, LAMP_RADIUS, 1);
    lcd_draw_circle(LAMP_B_RED_X, LAMP_B_RED_Y, BLACK, LAMP_RADIUS, 1);
    lcd_draw_circle(LAMP_B_GREEN_X, LAMP_B_GREEN_Y, BLACK, LAMP_RADIUS, 1);
    lcd_draw_circle(LAMP_B_YELLOW_X, LAMP_B_YELLOW_Y, BLACK, LAMP_RADIUS, 1);
}

// Function to set lamp colors with border
void set_lamps(State state) {
    switch (state) {
        case STATE_A_RED_B_GREEN:
            lcd_draw_circle(LAMP_A_RED_X, LAMP_A_RED_Y, RED, LAMP_RADIUS, 1);
            lcd_draw_circle(LAMP_B_GREEN_X, LAMP_B_GREEN_Y, GREEN, LAMP_RADIUS, 1);
            break;
        case STATE_A_RED_B_YELLOW:
            lcd_draw_circle(LAMP_A_RED_X, LAMP_A_RED_Y, RED, LAMP_RADIUS, 1);
            lcd_draw_circle(LAMP_B_YELLOW_X, LAMP_B_YELLOW_Y, YELLOW, LAMP_RADIUS, 1);
            break;
        case STATE_A_GREEN_B_RED:
            lcd_draw_circle(LAMP_A_GREEN_X, LAMP_A_GREEN_Y, GREEN, LAMP_RADIUS, 1);
            lcd_draw_circle(LAMP_B_RED_X, LAMP_B_RED_Y, RED, LAMP_RADIUS, 1);
            break;
        case STATE_A_YELLOW_B_RED:
            lcd_draw_circle(LAMP_A_YELLOW_X, LAMP_A_YELLOW_Y, YELLOW, LAMP_RADIUS, 1);
            lcd_draw_circle(LAMP_B_RED_X, LAMP_B_RED_Y, RED, LAMP_RADIUS, 1);
            break;
        case STATE_RED_GREEN:
        case STATE_RED_GREEN_MAN:
            lcd_draw_circle(LAMP_A_RED_X, LAMP_A_RED_Y, RED, LAMP_RADIUS, 1);
            lcd_draw_circle(LAMP_B_GREEN_X, LAMP_B_GREEN_Y, GREEN, LAMP_RADIUS, 1);
            break;
        case STATE_RED_YELLOW:
        case STATE_RED_YELLOW_MAN:
            lcd_draw_circle(LAMP_A_RED_X, LAMP_A_RED_Y, RED, LAMP_RADIUS, 1);
            lcd_draw_circle(LAMP_B_YELLOW_X, LAMP_B_YELLOW_Y, YELLOW, LAMP_RADIUS, 1);
            break;
        case STATE_GREEN_RED:
        case STATE_GREEN_RED_MAN:
            lcd_draw_circle(LAMP_A_GREEN_X, LAMP_A_GREEN_Y, GREEN, LAMP_RADIUS, 1);
            lcd_draw_circle(LAMP_B_RED_X, LAMP_B_RED_Y, RED, LAMP_RADIUS, 1);
            break;
        case STATE_YELLOW_RED:
        case STATE_YELLOW_RED_MAN:
            lcd_draw_circle(LAMP_A_YELLOW_X, LAMP_A_YELLOW_Y, YELLOW, LAMP_RADIUS, 1);
            lcd_draw_circle(LAMP_B_RED_X, LAMP_B_RED_Y, RED, LAMP_RADIUS, 1);
            break;
        default:
            break;
    }
}

// Blink function for EDIT states only
void blink_editing_lamp(uint16_t color) {
    if (current_state == STATE_MODE2 || current_state == STATE_RED_EDIT ||
        current_state == STATE_MODE3 || current_state == STATE_GREEN_EDIT ||
        current_state == STATE_MODE4 || current_state == STATE_YELLOW_EDIT) {
        uint16_t blink_color = blink_flag ? color : BLACK;
        if (current_state == STATE_MODE2 || current_state == STATE_RED_EDIT) {
            lcd_draw_circle(LAMP_A_RED_X, LAMP_A_RED_Y, blink_color, LAMP_RADIUS, 1);
            lcd_draw_circle(LAMP_B_RED_X, LAMP_B_RED_Y, blink_color, LAMP_RADIUS, 1);
        } else if (current_state == STATE_MODE3 || current_state == STATE_GREEN_EDIT) {
            lcd_draw_circle(LAMP_A_GREEN_X, LAMP_A_GREEN_Y, blink_color, LAMP_RADIUS, 1);
            lcd_draw_circle(LAMP_B_GREEN_X, LAMP_B_GREEN_Y, blink_color, LAMP_RADIUS, 1);
        } else if (current_state == STATE_MODE4 || current_state == STATE_YELLOW_EDIT) {
            lcd_draw_circle(LAMP_A_YELLOW_X, LAMP_A_YELLOW_Y, blink_color, LAMP_RADIUS, 1);
            lcd_draw_circle(LAMP_B_YELLOW_X, LAMP_B_YELLOW_Y, blink_color, LAMP_RADIUS, 1);
        }
    }
}

// Show mode and value on LCD
void show_mode_and_value(const char* mode_str, uint8_t value_A, uint8_t value_B) {
    // Không xóa vùng, giữ nguyên cách cũ
    // Hiển thị mode ở trên
    lcd_show_string_center(75, STATUS_BAR_Y + 5, (char*)mode_str, WHITE, BLUE, 16, 0);

    // Hiển thị hai số đếm A và B ở dưới
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "A:%u s B:%u s", value_A, value_B);
    lcd_show_string_center(2, 290, buffer, WHITE, BLACK, 16, 0); // Sử dụng BLACK làm nền để nổi bật
}
// Show mode and value on LCD in edit mode
void show_mode_and_value_edit(const char* mode_str, uint8_t value_A) {
    // Không xóa vùng, giữ nguyên cách cũ
    // Hiển thị mode ở trên
    lcd_show_string_center(75, STATUS_BAR_Y + 5, (char*)mode_str, WHITE, BLUE, 16, 0);

    // Hiển thị hai số đếm A và B ở dưới
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "Edit time:%u s", value_A);
    lcd_show_string_center(2, 290, buffer, WHITE, BLACK, 16, 0); // Sử dụng BLACK làm nền để nổi bật
}


// Debug show button pressed
//void show_button_debug(void) {
//    for (int i = 0; i < 16; i++) {
//        if (button_count[i] > 0) {
//            lcd_show_int_num(140, STATUS_BAR_Y + 10, i, 2, BRED, WHITE, 32);
//        }
//    }
//}

// Main FSM process
void FSM_process(void) {
    // Get button flags (using first 3 buttons: 0, 1, 2)
    button1_flag = (button_count[0] == 1);
    button2_flag = (button_count[1] == 1 || (button_count[1] > 20 && button_count[1] % 2 == 0));
    button3_flag = (button_count[2] == 1);

    // Accumulate ticks
    tick_count = (tick_count + 1) % 20; // 20*50ms = 1s
    blink_count = (blink_count + 1) % 10; // 10*50ms = 0.5s blink
    if (blink_count == 0) blink_flag ^= 1;

    // 1s flag for sch_counter decrement
    uint8_t second_flag = (tick_count == 0);

    switch (current_state) {
        case STATE_INIT:
            show_mode_and_value("INIT       ", 0, 0);
            draw_lamps_off(); // Only draw lamps off, no blinking
            sch_counter_A = 0; // Start with 3s for both
            sch_counter_B = 0;
            if (button1_flag) {
                current_state = STATE_MODE1;
            }
            break;

        case STATE_MODE1:
            if (sch_counter_A == 0 && sch_counter_B == 0) {
                current_state = STATE_A_RED_B_GREEN;
                sch_counter_A = red_duration;
                sch_counter_B = green_duration;
                draw_lamps_off();
            }
            show_mode_and_value("MODE 1     ", sch_counter_A, sch_counter_B);
            if (button1_flag) current_state = STATE_MODE2;
            break;

        case STATE_A_RED_B_GREEN:
            set_lamps(STATE_A_RED_B_GREEN);
            show_mode_and_value("MODE 1     ", sch_counter_A, sch_counter_B);
            if (second_flag && sch_counter_A > 0) sch_counter_A--;
            if (second_flag && sch_counter_B > 0) sch_counter_B--;
            if (sch_counter_B == 0) {
                current_state = STATE_A_RED_B_YELLOW;
                sch_counter_A = yellow_duration;
                sch_counter_B = yellow_duration;
                draw_lamps_off();
            } else if (button1_flag) {
                current_state = STATE_MODE2;
                lcd_clear(WHITE);
                draw_lamps_off();
            }
            break;

        case STATE_A_RED_B_YELLOW:
            set_lamps(STATE_A_RED_B_YELLOW);
            show_mode_and_value("MODE 1     ", sch_counter_A, sch_counter_B);
            if (second_flag && sch_counter_A > 0) sch_counter_A--;
            if (second_flag && sch_counter_B > 0) sch_counter_B--;
            if (sch_counter_B == 0 && sch_counter_B == 0) {
                sch_counter_A = green_duration;
                sch_counter_B = red_duration;
                current_state = STATE_A_GREEN_B_RED;
                draw_lamps_off();
            } else if (button1_flag) {
                current_state = STATE_MODE2;
                lcd_clear(WHITE);
                draw_lamps_off();
            }
            break;

        case STATE_A_GREEN_B_RED:
            set_lamps(STATE_A_GREEN_B_RED);
            show_mode_and_value("MODE 1     ", sch_counter_A, sch_counter_B);
            if (second_flag && sch_counter_A > 0) sch_counter_A--;
            if (second_flag && sch_counter_B > 0) sch_counter_B--;
            if (sch_counter_A == 0) {
                current_state = STATE_A_YELLOW_B_RED;
                sch_counter_A = yellow_duration;
                sch_counter_B = yellow_duration;
                draw_lamps_off();
            } else if (button1_flag) {
                current_state = STATE_MODE2;
                lcd_clear(WHITE);
                draw_lamps_off();
            }
            break;

        case STATE_A_YELLOW_B_RED:
            set_lamps(STATE_A_YELLOW_B_RED);
            show_mode_and_value("MODE 1      ", sch_counter_A, sch_counter_B);
            if (second_flag && sch_counter_A > 0) sch_counter_A--;
            if (second_flag && sch_counter_B > 0) sch_counter_B--;
            if (sch_counter_A == 0) {
                sch_counter_A = red_duration;
                sch_counter_B = green_duration;
                current_state = STATE_A_RED_B_GREEN;
                draw_lamps_off();
            } else if (button1_flag) {
                current_state = STATE_MODE2;
                lcd_clear(WHITE);
                draw_lamps_off();

            }
            break;

        case STATE_MODE2:
            show_mode_and_value_edit("MODE 2      ", red_duration);
            blink_editing_lamp(RED);
            if (button1_flag) {
                current_state = STATE_MODE3;
                lcd_clear(WHITE);
                draw_lamps_off();
            } else if (button2_flag) {
                current_state = STATE_RED_EDIT;
                lcd_clear(WHITE);
                draw_lamps_off();
            }
            break;

        case STATE_RED_EDIT:
            show_mode_and_value_edit("MODE 2      ", red_duration);
            if (button2_flag) red_duration = (red_duration % 99) + 1;
            if (button3_flag) current_state = STATE_RED_SAVE;
            blink_editing_lamp(RED);
            sch_counter_A = 3;
            break;

        case STATE_RED_SAVE:
            show_mode_and_value_edit("MODE 2-SAVE      ", red_duration);
            if (second_flag && sch_counter_A > 0) sch_counter_A--;
            if (second_flag && sch_counter_A == 0) {
            	current_state = STATE_INIT;
            	lcd_clear(WHITE);
            }
            break;

        case STATE_MODE3:
            show_mode_and_value_edit("MODE 3      ", green_duration);
            blink_editing_lamp(GREEN);
            if (button1_flag) {
                current_state = STATE_MODE4;
                lcd_clear(WHITE);
                draw_lamps_off();
            } else if (button2_flag) {
                current_state = STATE_GREEN_EDIT;
                draw_lamps_off();
            }
            break;

        case STATE_GREEN_EDIT:
        	show_mode_and_value_edit("MODE 3      ", green_duration);
            if (button2_flag) green_duration = (green_duration % 99) + 1;
            if (button3_flag) current_state = STATE_GREEN_SAVE;
            blink_editing_lamp(GREEN);
            sch_counter_B = 3;
            break;

        case STATE_GREEN_SAVE:
            show_mode_and_value_edit("MODE 3-SAVE      ", green_duration);
            if (second_flag && sch_counter_B > 0) sch_counter_B--;
            if (second_flag && sch_counter_B == 0) {
            	current_state = STATE_INIT;
                lcd_clear(WHITE);
            }
            break;

        case STATE_MODE4:
        	show_mode_and_value_edit("MODE 4      ", yellow_duration);
            blink_editing_lamp(YELLOW);
            if (button1_flag) {
                current_state = STATE_MODE5;
                lcd_clear(WHITE);
                draw_lamps_off();
            } else if (button2_flag) {
                current_state = STATE_YELLOW_EDIT;
                draw_lamps_off();
            }
            break;

        case STATE_YELLOW_EDIT:
        	show_mode_and_value_edit("MODE 4      ", yellow_duration);
            if (button2_flag) yellow_duration = (yellow_duration % 99) + 1;
            if (button3_flag) current_state = STATE_YELLOW_SAVE;
            blink_editing_lamp(YELLOW);
            sch_counter_B = 3;
            break;

        case STATE_YELLOW_SAVE:
        	show_mode_and_value_edit("MODE 4-SAVE     ", yellow_duration);
            if (sch_counter_B > 0) sch_counter_B--;
            if (second_flag && sch_counter_B == 0) {
            	current_state = STATE_INIT;
                lcd_clear(WHITE);
            }
            break;

        case STATE_MODE5:
            draw_lamps_off();
            show_mode_and_value("MODE 5     ", 0, 0);
            sch_counter_A = 0;
            sch_counter_B = 0;
            if (button1_flag) {
                current_state = STATE_MODE1;
                draw_lamps_off();
            } else if (sch_counter_A == 0 && sch_counter_B == 0) {
                current_state = STATE_RED_GREEN_MAN;
                draw_lamps_off();
            }
            break;

        case STATE_RED_GREEN_MAN:
            set_lamps(STATE_RED_GREEN_MAN);
            show_mode_and_value("MAN RED-GRN", 0, 0);
            if (button1_flag) {
                current_state = STATE_MODE1;
                draw_lamps_off();
            }
            if (button3_flag) {
                current_state = STATE_RED_YELLOW_MAN;
                draw_lamps_off();
            }
            break;

        case STATE_RED_YELLOW_MAN:
            set_lamps(STATE_RED_YELLOW_MAN);
            show_mode_and_value("MAN RED-YEL", 0, 0);
            if (button1_flag) {
                current_state = STATE_MODE1;
                draw_lamps_off();
            }
            if (button3_flag) {
                current_state = STATE_GREEN_RED_MAN;
                draw_lamps_off();
            }
            break;

        case STATE_GREEN_RED_MAN:
            set_lamps(STATE_GREEN_RED_MAN);
            show_mode_and_value("MAN GRN-RED", 0, 0);
            if (button1_flag) {
                current_state = STATE_MODE1;
                draw_lamps_off();
            }
            if (button3_flag) {
                current_state = STATE_YELLOW_RED_MAN;
                draw_lamps_off();
            }
            break;

        case STATE_YELLOW_RED_MAN:
            set_lamps(STATE_YELLOW_RED_MAN);
            show_mode_and_value("MAN YEL-RED", 0, 0);
            if (button1_flag) {
                current_state = STATE_MODE1;
                draw_lamps_off();
            }
            if (button3_flag) {
                current_state = STATE_RED_GREEN_MAN;
                draw_lamps_off();
            }
            break;
    }

    // Reset flags if needed (handled by button_scan debouncing)
    //show_button_debug(); // Show which button is pressed
}
