// user_menu.h
#ifndef __USER_MENU_H__
#define __USER_MENU_H__

#include "types.h"

#define NUM_USERS 3

// Database parallel arrays for simplicity (no struct used)
extern const char card_ids[NUM_USERS][9];
extern const char pins[NUM_USERS][5];
extern const char names[NUM_USERS][16];
extern const char dobs[NUM_USERS][12];
extern const char pans[NUM_USERS][12];
extern const char dl_numbers[NUM_USERS][12];
extern const char vehicle_classes[NUM_USERS][20];
extern volatile u16 exp_years[NUM_USERS];
extern const u16 eeprom_balance_addrs[NUM_USERS];
extern const u16 eeprom_vote_addrs[NUM_USERS];
extern const char addresses[NUM_USERS][6];
extern const u16 eeprom_login_pin_addrs[NUM_USERS];
extern const u16 eeprom_atm_pin_addrs[NUM_USERS];

// Global index of active user (-1 if none)
extern volatile int current_user_index;
extern volatile u8 rtc_interrupted_flag;
extern volatile u8 auto_logout_flag;
extern volatile u8 switch_pressed_flag;

// Public functions
void check_eeprom_init(void);
void handle_external_switch(void);
u8 scan_officer_card(void);
u8 verify_password(void);
u8 verify_password_flow(u8 type);
void user_menu(void);
void change_password_flow(void);
void EINT3_ISR(void) __irq;
void officer_menu(void);
void update_rtc_dow(void);

#endif
