/******************************************************************************
 * @file    user_menu.c
 * @brief   Citizen service operations and menu dashboard implementation.
 *          Implements PAN details, ATM transactions, Voting with custom icons,
 *          and Driving License checking.
 * @author  RFID Pair Programming Team
 * @date    July 2026
 ******************************************************************************/

#include <lpc21xx.h>
#include "types.h"
#include "lcd_defines.h"
#include "delay.h"
#include "kpm.h"
#include "spi_eeprom_defines.h"
#include "user_menu.h"

// Define 3 system users' database using parallel arrays (no structs)
const char card_ids[NUM_USERS][9] = {
    "12547508",     // Card ID - User 1 
    "12546806",     // Card ID - User 2 
    "12543380"      // Card ID - User 3 
};

const char pins[NUM_USERS][5] = {
    "1234",         // PIN for User 1
    "1234",         // PIN for User 2
    "1234"          // PIN for User 3
};

const char names[NUM_USERS][16] = {
    "Harsha",
    "Sai",
    "Prakash"
};

const char dobs[NUM_USERS][12] = {
    "04-07-2003",
    "10-08-2004",
    "14-07-2003"
};

const char pans[NUM_USERS][12] = {
    "ABCDE1234F",
    "XYZWV9876A",
    "PQRST5678B"
};

const char dl_numbers[NUM_USERS][12] = {
    "AP-22456789",
    "AP-22201302",
    "AP-33201403"
};

const char vehicle_classes[NUM_USERS][20] = {
    "2 Wheeler",
    "2/4 Wheeler",
    "4 Wheeler"
};

volatile u16 exp_years[NUM_USERS] = { 2029, 2028, 2026 }; 

const u16 eeprom_balance_addrs[NUM_USERS] = { 0x0010, 0x0012, 0x0014 };
const u16 eeprom_vote_addrs[NUM_USERS] = { 0x0020, 0x0021, 0x0022 };
const char addresses[NUM_USERS][6] = { "BZA", "HYD", "KCH" };

volatile int current_user_index = -1; // Global index of the active logged-in user
volatile u8 rtc_interrupted_flag = 0;
volatile u8 auto_logout_flag = 0;

// PIN EEPROM Addresses
const u16 eeprom_login_pin_addrs[NUM_USERS] = { 0x0030, 0x0034, 0x0038 };
const u16 eeprom_atm_pin_addrs[NUM_USERS] = { 0x0040, 0x0044, 0x0048 };

// Custom party symbols data (8 bytes each, total 32 bytes)
const u8 party_symbols[32] = {
    0x04, 0x0E, 0x04, 0x04, 0x0E, 0x0E, 0x0E, 0x00, // Candle
    0x04, 0x0E, 0x1F, 0x1F, 0x0E, 0x04, 0x00, 0x00, // Diamond 
    0x06, 0x0E, 0x1C, 0x18, 0x18, 0x1C, 0x0E, 0x00, // Moon
    0x04, 0x15, 0x0E, 0x1F, 0x0E, 0x15, 0x04, 0x00  // Sun
};

/**
 * @brief  Write user's ATM balance to external EEPROM.
 */
void write_balance(u16 bal)
{
    u8 buf[2];
    if (current_user_index < 0 || current_user_index >= NUM_USERS) return;
    buf[0] = (bal >> 8) & 0xFF;
    buf[1] = bal & 0xFF;
    BufferWrite_25LC512(eeprom_balance_addrs[current_user_index], buf, 2);
}

/**
 * @brief  Read user's ATM balance from external EEPROM.
 */
u16 read_balance(void)
{
    u8 buf[2];
    u16 bal;
    if (current_user_index < 0 || current_user_index >= NUM_USERS) return 0;
    BufferRead_25LC512(eeprom_balance_addrs[current_user_index], buf, 2);
    bal = ((u16)buf[0] << 8) | buf[1];
    
    if (bal == 0xFFFF)
    {
        bal = 5000; // Initialize with 5000 if EEPROM is blank
        write_balance(bal);
    }
    return bal;
}

/**
 * @brief  Verify and load defaults into EEPROM on first boot magic byte check.
 */
void check_eeprom_init(void)
{
    u8 magic = ByteRead_25LC512(0x0000);
    if (magic != 0xC5)
    {
        u8 pin_def[] = {'1', '2', '3', '4'};
        u8 bal1[] = {(20000 >> 8) & 0xFF, 20000 & 0xFF};
        u8 bal2[] = {(5000 >> 8) & 0xFF, 5000 & 0xFF};
        u8 bal3[] = {(12500 >> 8) & 0xFF, 12500 & 0xFF};
        
        // Write magic byte
        ByteWrite_25LC512(0x0000, 0xC5);
        
        // Initialize User 1: Balance = 20000, Vote = 0, Login PIN = "1234", ATM PIN = "1234"
        BufferWrite_25LC512(0x0010, bal1, 2);
        ByteWrite_25LC512(0x0020, 0x00);
        BufferWrite_25LC512(0x0030, pin_def, 4);
        BufferWrite_25LC512(0x0040, pin_def, 4);
        
        // Initialize User 2: Balance = 5000, Vote = 2, Login PIN = "1234", ATM PIN = "1234"
        BufferWrite_25LC512(0x0012, bal2, 2);
        ByteWrite_25LC512(0x0021, 0x02);
        BufferWrite_25LC512(0x0034, pin_def, 4);
        BufferWrite_25LC512(0x0044, pin_def, 4);
        
        // Initialize User 3: Balance = 12500, Vote = 0, Login PIN = "1234", ATM PIN = "1234"
        BufferWrite_25LC512(0x0014, bal3, 2);
        ByteWrite_25LC512(0x0022, 0x00);
        BufferWrite_25LC512(0x0038, pin_def, 4);
        BufferWrite_25LC512(0x0048, pin_def, 4);
    }
}

/**
 * @brief  Prompt and verify passcode with a 20s timeout and clear feature.
 */
u8 verify_password_flow(u8 type)
{
    u8 attempts = 3;
    char entered_pin[5] = {0};
    char correct_pin[5] = {0};
    u16 pin_eeprom_addr;
    u8 len;
    char key;
    
    if (current_user_index < 0 || current_user_index >= NUM_USERS) return 0;
    
    if (type == 0)
    {
        pin_eeprom_addr = eeprom_login_pin_addrs[current_user_index];
    }
    else
    {
        pin_eeprom_addr = eeprom_atm_pin_addrs[current_user_index];
    }
    
    while (attempts > 0)
    {
        // Load the correct PIN from EEPROM (single sequential read)
        BufferRead_25LC512(pin_eeprom_addr, (u8 *)correct_pin, 4);
        correct_pin[4] = 0;
        
        len = 0;
        entered_pin[0] = 0; 
		entered_pin[1] = 0; 
		entered_pin[2] = 0; 
		entered_pin[3] = 0; 
		entered_pin[4] = 0;
        
        CmdLCD(CLEAR_LCD);
        CmdLCD(GOTO_LINE1_POS0);
        if (type == 0)
        {
            StrLCD("Enter LOGIN PIN:");
        }
        else
        {
            StrLCD("Enter ATM PIN:");
        }
        CmdLCD(GOTO_LINE2_POS0);
        StrLCD("PIN: ");
        CmdLCD(GOTO_LINE3_POS0);
        StrLCD("#:Enter *:Clr B:Exit");
        
        while (1)
        {
            CmdLCD(GOTO_LINE2_POS0 + 5 + len); // Force cursor to correct position on Line 2
            key = KeyScanWithTimeout(20000);
            if (key == 0 || key == 0xFE) return 0; // Timeout or Exit
            
            if (key >= '0' && key <= '9')
            {
                if (len < 4)
                {
                    entered_pin[len] = key;
                    CmdLCD(GOTO_LINE2_POS0 + 5 + len);
                    CharLCD('*');
                    len++;
                    CmdLCD(GOTO_LINE2_POS0 + 5 + len);
                }
            }
            else if (key == '*') // Clear / Backspace
            {
                if (len > 0)
                {
                    len--;
                    entered_pin[len] = 0;
                    CmdLCD(GOTO_LINE2_POS0 + 5 + len);
                    CharLCD(' ');
                    CmdLCD(GOTO_LINE2_POS0 + 5 + len);
                }
            }
            else if (key == 'B') // Dedicated Exit key
            {
                return 0;
            }
            else if (key == '#')
            {
                if (len == 4)
                {
                    break; // PIN fully entered, proceed to compare
                }
            }
        }
        
        // Compare PIN
        if (entered_pin[0] == correct_pin[0] &&
            entered_pin[1] == correct_pin[1] &&
            entered_pin[2] == correct_pin[2] &&
            entered_pin[3] == correct_pin[3])
        {
            return 1; // Correct password
        }
        else
        {
            attempts--;
            CmdLCD(CLEAR_LCD);
            CmdLCD(GOTO_LINE1_POS0);
            StrLCD("Entered Wrong PIN!");
            CmdLCD(GOTO_LINE2_POS0);
            StrLCD("Attempts Left: ");
            U32LCD(attempts);
            delay_ms(2000);
        }
    }
    
    return 0; // 3 wrong attempts, fail verification
}

u8 verify_password(void)
{
    return verify_password_flow(0); // type 0 = Login PIN
}

void change_password_flow(void)
{
    char entered_prev[5] = {0};
    char correct_prev[5] = {0};
    char entered_new[5] = {0};
    u16 pin_eeprom_addr;
    u8 i, len;
    char key;
    
    if (current_user_index < 0 || current_user_index >= NUM_USERS) return;
    
    pin_eeprom_addr = eeprom_login_pin_addrs[current_user_index];
    
    // 1. Ask for Previous Password
    len = 0;
    CmdLCD(CLEAR_LCD);
    CmdLCD(GOTO_LINE1_POS0);
    StrLCD("Old Password:");
    CmdLCD(GOTO_LINE2_POS0);
    StrLCD("PIN: ");
    CmdLCD(GOTO_LINE3_POS0);
    StrLCD("#:Enter *:Clr B:Exit");
    
    while (1)
    {
        CmdLCD(GOTO_LINE2_POS0 + 5 + len);
        key = KeyScanWithTimeout(20000);
        if (key == 0 || key == 0xFE) return; // Timeout or Exit
        
        if (key >= '0' && key <= '9')
        {
            if (len < 4)
            {
                entered_prev[len] = key;
                CmdLCD(GOTO_LINE2_POS0 + 5 + len);
                CharLCD('*');
                len++;
                CmdLCD(GOTO_LINE2_POS0 + 5 + len);
            }
        }
        else if (key == '*') // Clear / Backspace
        {
            if (len > 0)
            {
                len--;
                entered_prev[len] = 0;
                CmdLCD(GOTO_LINE2_POS0 + 5 + len);
                CharLCD(' ');
                CmdLCD(GOTO_LINE2_POS0 + 5 + len);
            }
        }
        else if (key == 'B') // Dedicated Exit key
        {
            return;
        }
        else if (key == '#')
        {
            if (len == 4) break;
        }
    }
    
    // Verify with old Password
    BufferRead_25LC512(pin_eeprom_addr, (u8 *)correct_prev, 4);
    correct_prev[4] = 0;
    
    if (entered_prev[0] != correct_prev[0] ||
        entered_prev[1] != correct_prev[1] ||
        entered_prev[2] != correct_prev[2] ||
        entered_prev[3] != correct_prev[3])
    {
        CmdLCD(CLEAR_LCD);
        CmdLCD(GOTO_LINE1_POS0);
        StrLCD("Wrong Password!");
        delay_ms(2000);
        return;
    }
    
    // 2. Ask for New Password
    len = 0;
    CmdLCD(CLEAR_LCD);
    CmdLCD(GOTO_LINE1_POS0);
    StrLCD("New Password:");
    CmdLCD(GOTO_LINE2_POS0);
    StrLCD("PIN: ");
    CmdLCD(GOTO_LINE3_POS0);
    StrLCD("#:Enter *:Clr B:Exit");
    
    while (1)
    {
        CmdLCD(GOTO_LINE2_POS0 + 5 + len);
        key = KeyScanWithTimeout(20000);
        if (key == 0 || key == 0xFE) return; // Timeout or Exit
        
        if (key >= '0' && key <= '9')
        {
            if (len < 4)
            {
                entered_new[len] = key;
                CmdLCD(GOTO_LINE2_POS0 + 5 + len);
                CharLCD('*');
                len++;
                CmdLCD(GOTO_LINE2_POS0 + 5 + len);
            }
        }
        else if (key == '*') // Clear / Backspace
        {
            if (len > 0)
            {
                len--;
                entered_new[len] = 0;
                CmdLCD(GOTO_LINE2_POS0 + 5 + len);
                CharLCD(' ');
                CmdLCD(GOTO_LINE2_POS0 + 5 + len);
            }
        }
        else if (key == 'B') // Dedicated Exit key
        {
            return;
        }
        else if (key == '#')
        {
            if (len == 4) break;
        }
    }
    
    // 3. Ask to Confirm New Password
    {
        char entered_confirm[5] = {0};
        len = 0;
        CmdLCD(CLEAR_LCD);
        CmdLCD(GOTO_LINE1_POS0);
        StrLCD("Re-enter Password:");
        CmdLCD(GOTO_LINE2_POS0);
        StrLCD("PIN: ");
        CmdLCD(GOTO_LINE3_POS0);
        StrLCD("#:Enter *:Clr B:Exit");
        
        while (1)
        {
            CmdLCD(GOTO_LINE2_POS0 + 5 + len);
            key = KeyScanWithTimeout(20000);
            if (key == 0 || key == 0xFE) return; // Timeout or Exit
            
            if (key >= '0' && key <= '9')
            {
                if (len < 4)
                {
                    entered_confirm[len] = key;
                    CmdLCD(GOTO_LINE2_POS0 + 5 + len);
                    CharLCD('*');
                    len++;
                    CmdLCD(GOTO_LINE2_POS0 + 5 + len);
                }
            }
            else if (key == '*') // Clear / Backspace
            {
                if (len > 0)
                {
                    len--;
                    entered_confirm[len] = 0;
                    CmdLCD(GOTO_LINE2_POS0 + 5 + len);
                    CharLCD(' ');
                    CmdLCD(GOTO_LINE2_POS0 + 5 + len);
                }
            }
            else if (key == 'B') // Dedicated Exit key
            {
                return;
            }
            else if (key == '#')
            {
                if (len == 4) break;
            }
        }
        
        // Compare new password with confirm password
        for (i = 0; i < 4; i++)
        {
            if (entered_new[i] != entered_confirm[i])
            {
                CmdLCD(CLEAR_LCD);
                CmdLCD(GOTO_LINE1_POS0);
                StrLCD("Not Matched...");
                delay_ms(2000);
                return;
            }
        }
    }
    
    // Save New Password to EEPROM for BOTH Login PIN and ATM PIN
    BufferWrite_25LC512(eeprom_login_pin_addrs[current_user_index], (u8 *)entered_new, 4);
    BufferWrite_25LC512(eeprom_atm_pin_addrs[current_user_index], (u8 *)entered_new, 4);
    
    CmdLCD(CLEAR_LCD);
    CmdLCD(GOTO_LINE1_POS0);
    StrLCD("PWD Changed!");
    delay_ms(2000);
}

/**
 * @brief  Enter numeric amount for deposits/withdrawals with backspace support.
 */
u32 enter_amount(u8 is_withdrawal)
{
    u32 val = 0;
    char key;
    u8 len = 0;
    
    CmdLCD(CLEAR_LCD);
    CmdLCD(GOTO_LINE1_POS0);
    StrLCD("Multiples of 100");
    CmdLCD(GOTO_LINE2_POS0);
    StrLCD("Amount: ");
    CmdLCD(GOTO_LINE3_POS0);
    StrLCD("#:Enter *:Clr B:Exit");
    
    while (1)
    {
        CmdLCD(GOTO_LINE2_POS0 + 8 + len);
        key = KeyScanWithTimeout(20000);
        if (key == 0) return 0xFFFFFFFF; // Timeout
        
        if (key >= '0' && key <= '9')
        {
            if (len < 5) // Limit input to maximum 5 digits
            {
                val = (val * 10) + (key - '0');
                CmdLCD(GOTO_LINE2_POS0 + 8 + len);
                CharLCD(key);
                len++;
                CmdLCD(GOTO_LINE2_POS0 + 8 + len);
            }
        }
        else if (key == '*') // Clear / Backspace
        {
            if (len > 0)
            {
                val = val / 10;
                len--;
                CmdLCD(GOTO_LINE2_POS0 + 8 + len);
                CharLCD(' ');
                CmdLCD(GOTO_LINE2_POS0 + 8 + len);
            }
        }
        else if (key == 'B') // Dedicated Exit key
        {
            return 0xFFFFFFFF; // Exit/Cancel amount entry!
        }
        else if (key == '#')
        {
            if (len > 0) // Must enter at least one digit
            {
                return val;
            }
        }
    }
}

/**
 * @brief  Read generic numeric value for calendar adjustments (hour, minute, etc.)
 */
u32 rtc_read_num(u8 max_digits)
{
    u32 val = 0;
    char key;
    u8 len = 0;
    
    CmdLCD(GOTO_LINE2_POS0);
    StrLCD("Val: ");
    
    while (1)
    {
        CmdLCD(GOTO_LINE2_POS0 + 5 + len);
        key = KeyScanWithTimeout(20000);
        if (key == 0 || key == 0xFE) return 999999; // Return special timeout/cancel code
        
        if (key >= '0' && key <= '9')
        {
            if (len < max_digits) // Limit input to specified digit length
            {
                val = (val * 10) + (key - '0');
                CmdLCD(GOTO_LINE2_POS0 + 5 + len);
                CharLCD(key);
                len++;
                CmdLCD(GOTO_LINE2_POS0 + 5 + len);
            }
        }
        else if (key == '*') // Clear / Backspace
        {
            if (len > 0)
            {
                val = val / 10;
                len--;
                CmdLCD(GOTO_LINE2_POS0 + 5 + len);
                CharLCD(' ');
                CmdLCD(GOTO_LINE2_POS0 + 5 + len);
            }
        }
        else if (key == 'B') // Dedicated Exit key
        {
            return 999999;
        }
        else if (key == '#')
        {
            if (len > 0) // Must enter at least one digit
            {
                return val;
            }
        }
    }
}

/**
 * @brief  Show citizen PAN database.
 */
void PAN_menu(void)
{
    char key;
    if (!verify_password()) return;
    if (auto_logout_flag) return;
    
    CmdLCD(CLEAR_LCD);
    CmdLCD(GOTO_LINE1_POS0);
    StrLCD("Name: "); StrLCD((s8 *)names[current_user_index]);
    CmdLCD(GOTO_LINE2_POS0);
    StrLCD("DOB: "); StrLCD((s8 *)dobs[current_user_index]);
    CmdLCD(GOTO_LINE3_POS0);
    StrLCD("PAN: "); StrLCD((s8 *)pans[current_user_index]);
    CmdLCD(GOTO_LINE4_POS0);
    StrLCD("B:Exit");
    
    key = KeyScanWithTimeout(20000);
    if (key == 0 || key == '*' || key == 'B' || auto_logout_flag)
    {
        return;
    }
}

/**
 * @brief  ATM balance check, withdrawals, and deposits.
 */
void ATM_menu(void)
{
    char key;
    u16 bal;
    u32 amt;
    
    if (!verify_password_flow(1)) return;
    if (auto_logout_flag) return;
    
    while (1)
    {
        if (auto_logout_flag) break;
        
        CmdLCD(CLEAR_LCD);
        CmdLCD(GOTO_LINE1_POS0);
        StrLCD("1:Balance Enquiry");
        CmdLCD(GOTO_LINE2_POS0);
        StrLCD("2:Cash Withdrawal");
        CmdLCD(GOTO_LINE3_POS0);
        StrLCD("3:Cash Deposit");
        CmdLCD(GOTO_LINE4_POS0);
        StrLCD("4:Exit");
        //CmdLCD(GOTO_LINE4_POS0 + 16);
        key = KeyScanWithTimeout(20000);
        if (key == 0 || key == '4' || auto_logout_flag) break; // Timeout or Exit
        
        if (key == '1')
        {
            bal = read_balance();
            while (1)
            {
                CmdLCD(CLEAR_LCD);
                CmdLCD(GOTO_LINE1_POS0);
				StrLCD("Balance Available:");
                CmdLCD(GOTO_LINE2_POS0);
                U32LCD(bal);
				StrLCD("/-");
                CmdLCD(GOTO_LINE4_POS0);
                StrLCD("B:Exit");
                key = KeyScanWithTimeout(20000);
                if (key == 0 || key == '*' || key == 'B' || auto_logout_flag) break; // Timeout or Exit
            }
        }
        else if (key == '2')
        {
            while (1)
            {
                amt = enter_amount(1); // 1 = Withdrawal
                if (amt == 0xFFFFFFFF) break;   // Timeout or cancelled
                if (amt > 0 && amt % 100 == 0)
                {
                    break;
                }
                
                CmdLCD(CLEAR_LCD);
                CmdLCD(GOTO_LINE1_POS0);
                StrLCD("Invalid Amount");
                CmdLCD(GOTO_LINE2_POS0);
                StrLCD("multiples_100/200/500");
                //CmdLCD(GOTO_LINE3_POS0);
                //StrLCD("of 100");
                delay_ms(2500);
            }
            if (amt == 0xFFFFFFFF) continue;
            
            bal = read_balance();
            /* After withdrawal, at least 500/- must remain in the account */
            if (bal >= amt && (bal - amt) >= 500)
            {
                bal -= amt;
                write_balance(bal);
                CmdLCD(CLEAR_LCD);
                CmdLCD(GOTO_LINE1_POS0);
                StrLCD("Withdrawal Success");
            }
            else if (bal >= amt && (bal - amt) < 500)
            {
                CmdLCD(CLEAR_LCD);
                CmdLCD(GOTO_LINE1_POS0);
                StrLCD("Min bal required");
                CmdLCD(GOTO_LINE2_POS0);
                StrLCD("500/- is Mandatory");
                CmdLCD(GOTO_LINE3_POS0);
                StrLCD("Withdrawal is");
                CmdLCD(GOTO_LINE4_POS0);
                StrLCD("Not Possible");
            }
            else
            {
                CmdLCD(CLEAR_LCD);
                CmdLCD(GOTO_LINE1_POS0);
                StrLCD("Insufficient balance");
                CmdLCD(GOTO_LINE2_POS0);
                StrLCD("Min Bal: 500/-");
            }
            delay_ms(2500);
        }
        else if (key == '3')
        {
            while (1)
            {
                amt = enter_amount(0); // 0 = Deposit
                if (amt == 0xFFFFFFFF) break;   // Timeout or cancelled
                if (amt > 0 && amt % 100 == 0)
                {
                    break;
                }
                
                CmdLCD(CLEAR_LCD);
                CmdLCD(GOTO_LINE1_POS0);
                StrLCD("Invalid Amount");
                CmdLCD(GOTO_LINE2_POS0);
                StrLCD("multiple_100/200/500");
                //CmdLCD(GOTO_LINE3_POS0);
                //StrLCD("of 100");
                delay_ms(2500);
            }
            if (amt == 0xFFFFFFFF) continue;
            
            bal = read_balance();
            if (65535 - bal >= amt)
            {
                bal += amt;
                write_balance(bal);
                CmdLCD(CLEAR_LCD);
                StrLCD("Deposit Success");
                /*CmdLCD(GOTO_LINE2_POS0);
                StrLCD("New Bal: Rs.");
                U32LCD(bal);*/
            }
            else
            {
                CmdLCD(CLEAR_LCD);
                CmdLCD(GOTO_LINE1_POS0);
                StrLCD("Limit Exceeded");
            }
            delay_ms(2500);
        }
    }
}

/**
 * @brief  Cast vote for parties (1-4) or automatically cast NOTA.
 */
void VOTE_menu(void)
{
    u8 status;
    char key;
    
    if (current_user_index < 0 || current_user_index >= NUM_USERS) return;
    if (!verify_password()) return;
    if (auto_logout_flag) return;
    
    status = ByteRead_25LC512(eeprom_vote_addrs[current_user_index]);
    if (status != 0 && status != 0xFF)
    {
        CmdLCD(CLEAR_LCD);
        CmdLCD(GOTO_LINE1_POS0);
        StrLCD("Vote Casted");
        delay_ms(2500);
        return;
    }
    
    // Load custom icons into CGRAM
    BuildCGRAM((u8*)party_symbols, 32);
    
    while (1)
    {
        if (auto_logout_flag) return;
        
        CmdLCD(CLEAR_LCD);
        CmdLCD(GOTO_LINE1_POS0);
        StrLCD("1.P1:"); CharLCD(0); StrLCD("      2.P2:"); CharLCD(1);
        CmdLCD(GOTO_LINE2_POS0);
        StrLCD("3.P3:"); CharLCD(2); StrLCD("      4.P4:"); CharLCD(3);
        CmdLCD(GOTO_LINE3_POS0);
        StrLCD("5:Exit");
        
        key = KeyScanWithTimeout(20000);
        if (key == 0 || key == '5' || auto_logout_flag) return; // Timeout or Exit
        
        if (key >= '1' && key <= '4')
        {
            u8 party = key - '0';
            ByteWrite_25LC512(eeprom_vote_addrs[current_user_index], party);
            
            CmdLCD(CLEAR_LCD);
            CmdLCD(GOTO_LINE1_POS0);
            StrLCD("Vote Casted");
            delay_ms(2500);
            break; // Vote recorded successfully, exit loop
        }
    }
}

// Forward declarations
void rtc_edit_menu(void);

/**
 * @brief  Show license details screen with custom bold check/cross validity indicators.
 */
void show_license_details(void)
{
    u8 d = DOM;
    u8 m = MONTH;
    u16 y = YEAR;
    u8 expired = 0;
    char *short_class = "2/4W";
    char *v_class;
    char k;
    
    // Bold Check-cross patterns for CGRAM (0 = Check, 1 = Cross)
    const u8 check_cross_symbols[16] = {
        0x00, 0x00, 0x01, 0x03, 0x16, 0x1C, 0x08, 0x00, // Bold Checkmark (0)
        0x00, 0x11, 0x1B, 0x0E, 0x04, 0x0E, 0x1B, 0x11  // Bold Cross (1)
    };
    
    if (current_user_index < 0 || current_user_index >= NUM_USERS) return;
    
    // Load custom check/cross icons into CGRAM
    BuildCGRAM((u8*)check_cross_symbols, 16);
    
    // Display Present Date & Time for 3 seconds (No buzzer/LEDs active)
    IOCLR0 = (1 << 19) | (1 << 20) | (1 << 21); // Ensure Buzzer and LEDs are OFF
    CmdLCD(CLEAR_LCD);
    CmdLCD(GOTO_LINE1_POS0);
    StrLCD("Current Date/Time:");
    
    CmdLCD(GOTO_LINE2_POS0);
    StrLCD("Date: ");
    if (d < 10) CharLCD('0');
    U32LCD(d);
    CharLCD('-');
    if (m < 10) CharLCD('0');
    U32LCD(m);
    CharLCD('-');
    U32LCD(y);
    
    CmdLCD(GOTO_LINE3_POS0);
    StrLCD("Time: ");
    if (HOUR < 10) CharLCD('0');
    U32LCD(HOUR);
    CharLCD(':');
    if (MIN < 10) CharLCD('0');
    U32LCD(MIN);
    CharLCD(':');
    if (SEC < 10) CharLCD('0');
    U32LCD(SEC);
    
    CmdLCD(GOTO_LINE4_POS0);
    StrLCD("Day : ");
    {
        const char *day_names[7] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
        StrLCD((s8 *)day_names[DOW % 7]);
    }
    
    delay_ms(3000);
    
    while (1)
    {
        y = YEAR;
        expired = 0;
        short_class = "2/4W";
        
        // License Validity Check (Year only)
        if (y > exp_years[current_user_index]) expired = 1;
        
        // Map class to short representation
        v_class = (char *)vehicle_classes[current_user_index];
        if (v_class[0] == '2' && v_class[1] == ' ' && v_class[2] == 'W') short_class = "2W";
        else if (v_class[0] == '4') short_class = "4W";
        
        CmdLCD(CLEAR_LCD);
        CmdLCD(GOTO_LINE1_POS0);
        StrLCD("DL No: ");
        StrLCD((s8 *)dl_numbers[current_user_index]);
        
        if (!expired)
        {
            CmdLCD(GOTO_LINE2_POS0);
            StrLCD("Cls: "); StrLCD((s8 *)short_class);
            StrLCD("   Add: "); StrLCD((s8 *)addresses[current_user_index]);
            
            CmdLCD(GOTO_LINE3_POS0);
            StrLCD("Exp Yr: ");
            U32LCD(exp_years[current_user_index]);
            
            CmdLCD(GOTO_LINE4_POS0);
            StrLCD("Valid ");
            CharLCD(0); // Print Custom Checkmark symbol
            StrLCD("   B:Exit");
            
            IOSET0 = (1 << 21); // Turn ON Green LED
            IOCLR0 = (1 << 20) | (1 << 19); // Turn OFF Red LED and Buzzer
        }
        else
        {
            CmdLCD(GOTO_LINE2_POS0);
            StrLCD("Cls: "); StrLCD((s8 *)short_class);
            StrLCD("   Add: "); StrLCD((s8 *)addresses[current_user_index]);
            
            CmdLCD(GOTO_LINE3_POS0);
            StrLCD("Exp Yr: ");
            U32LCD(exp_years[current_user_index]);
            
            CmdLCD(GOTO_LINE4_POS0);
            StrLCD("Invalid ");
            CharLCD(1); // Print Custom Cross symbol
            StrLCD(" B:Exit");
            
            IOSET0 = (1 << 20) | (1 << 19); // Turn ON Red LED and Buzzer
            IOCLR0 = (1 << 21); // Turn OFF Green LED
        }
        
        if (auto_logout_flag) break;
        k = KeyScanWithTimeout(20000);
        if (k == '*' || k == 'B' || k == 0 || auto_logout_flag) break;
    }
    
    // Restore indicators back to active login state
    IOSET0 = (1 << 21);
    IOCLR0 = (1 << 20) | (1 << 19);
}

/**
 * @brief  Driving License menu dashboard.
 */
void DRIVING_menu(void)
{
    char key;
    while (1)
    {
        if (auto_logout_flag) break;
        
        CmdLCD(CLEAR_LCD);
        CmdLCD(GOTO_LINE1_POS0);
        StrLCD("1.Show Card Details");
        CmdLCD(GOTO_LINE2_POS0);
        StrLCD("B:Exit");
        
        key = KeyScanWithTimeout(20000);
        if (key == 0 || key == '*' || key == 'B' || auto_logout_flag) break; // Timeout -> exit
        
        if (key == '1')
        {
            show_license_details();
        }
    }
}

/**
 * @brief  Citizen Options Dashboard.
 */
void user_menu(void)
{
    char key;
    while (1)
    {
        if (auto_logout_flag) return;
        
        CmdLCD(CLEAR_LCD);
        CmdLCD(GOTO_LINE1_POS0);
        StrLCD("1.PAN     2.ATM");
        CmdLCD(GOTO_LINE2_POS0);
        StrLCD("3.VOTE    4.DRIV_LIC");
        CmdLCD(GOTO_LINE3_POS0);
        StrLCD("5.PWD_CHG 6.EXIT");
        
        key = KeyScanWithTimeout(20000);
        if (key == 0 || key == '6' || key == 'B' || auto_logout_flag) return; // Timeout or option 6/B -> auto log out
        
        if (key == '1')
        {
            PAN_menu();
        }
        else if (key == '2')
        {
            ATM_menu();
        }
        else if (key == '3')
        {
            VOTE_menu();
        }
        else if (key == '4')
        {
            DRIVING_menu();
        }
        else if (key == '5')
        {
            change_password_flow();
        }
    }
}

/**
 * @brief  Calculate and update the RTC's Day of Week (DOW) register dynamically
 *         using Sakamoto's algorithm (0 = Sunday, 1 = Monday, ..., 6 = Saturday).
 */
void update_rtc_dow(void)
{
    u8 d = DOM;
    u8 m = MONTH;
    u16 y = YEAR;
    u8 dow;
    static const u8 t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    
    if (m < 3)
    {
        y -= 1;
    }
    dow = (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
    
    CCR = 0x02; // Disable RTC and reset CTC
    DOW = dow;
    CCR = 0x01; // Enable RTC
}

/**
 * @brief  Menu-based Clock adjust interface for RTC.
 */
void rtc_edit_menu(void)
{
    char key;
    u32 val;
    
    while (1)
    {
        CmdLCD(CLEAR_LCD);
        CmdLCD(GOTO_LINE1_POS0);
        StrLCD("1.Hour 2.Min 3.Sec");
        CmdLCD(GOTO_LINE2_POS0);
        StrLCD("4.Day  5.Mon 6.Yr");
        CmdLCD(GOTO_LINE3_POS0);
        StrLCD("*:Exit");
        
        //delay_ms(300); // Prevent keypress leakage/double-trigger on cancel/exit
        key = KeyScanWithTimeout(20000);
        if (key == 0 || key == '*') break; // Timeout -> exit
        
        if (key == '1') // Hour
        {
            while (1)
            {
                CmdLCD(CLEAR_LCD);
                CmdLCD(GOTO_LINE1_POS0);
                StrLCD("Enter Hour (0-23):");
                CmdLCD(GOTO_LINE3_POS0);
                StrLCD("#:Enter *:Clr B:Exit");
                
                val = rtc_read_num(2);
                if (val == 999999) break; // Timeout
                
                if (val <= 23)
                {
                    CCR = 0x02;
                    HOUR = val;
                    CCR = 0x01;
                    
                    CmdLCD(CLEAR_LCD);
                    CmdLCD(GOTO_LINE1_POS0);
                    StrLCD("Hour Updated!");
                    delay_ms(1500);
                    break;
                }
                
                CmdLCD(CLEAR_LCD);
                CmdLCD(GOTO_LINE1_POS0);
                StrLCD("Invalid Hour!");
                CmdLCD(GOTO_LINE2_POS0);
                StrLCD("Must be 0 to 23");
                delay_ms(1500);
            }
        }
        else if (key == '2') // Minute
        {
            while (1)
            {
                CmdLCD(CLEAR_LCD);
                CmdLCD(GOTO_LINE1_POS0);
                StrLCD("Enter Min (0-59):");
                CmdLCD(GOTO_LINE3_POS0);
                StrLCD("#:Enter *:Clr B:Exit");
                
                val = rtc_read_num(2);
                if (val == 999999) break; // Timeout
                
                if (val <= 59)
                {
                    CCR = 0x02;
                    MIN = val;
                    CCR = 0x01;
                    
                    CmdLCD(CLEAR_LCD);
                    CmdLCD(GOTO_LINE1_POS0);
                    StrLCD("Minute Updated");
                    delay_ms(1500);
                    break;
                }
                
                CmdLCD(CLEAR_LCD);
                CmdLCD(GOTO_LINE1_POS0);
                StrLCD("Minute Invalid");
                CmdLCD(GOTO_LINE2_POS0);
                StrLCD("Must be 0 to 59");
                delay_ms(1500);
            }
        }
        else if (key == '3') // Second
        {
            while (1)
            {
                CmdLCD(CLEAR_LCD);
                CmdLCD(GOTO_LINE1_POS0);
                StrLCD("Enter Sec (0-59):");
                CmdLCD(GOTO_LINE3_POS0);
                StrLCD("#:Enter *:Clr B:Exit");
                
                val = rtc_read_num(2);
                if (val == 999999) break; // Timeout
                
                if (val <= 59)
                {
                    CCR = 0x02;
                    SEC = val;
                    CCR = 0x01;
                    
                    CmdLCD(CLEAR_LCD);
                    CmdLCD(GOTO_LINE1_POS0);
                    StrLCD("Second Updated");
                    delay_ms(1500);
                    break;
                }
                
                CmdLCD(CLEAR_LCD);
                CmdLCD(GOTO_LINE1_POS0);
                StrLCD("Second Invalid");
                CmdLCD(GOTO_LINE2_POS0);
                StrLCD("Must be 0 to 59");
                delay_ms(1500);
            }
        }
        else if (key == '4') // Day
        {
            while (1)
            {
                u8 cur_m = MONTH;
                u16 cur_y = YEAR;
                u8 max_days = 31;
                
                if (cur_m == 2)
                {
                    if (cur_y % 4 == 0) max_days = 29;
                    else max_days = 28;
                }
                else if (cur_m == 4 || cur_m == 6 || cur_m == 9 || cur_m == 11)
                {
                    max_days = 30;
                }
                
                CmdLCD(CLEAR_LCD);
                CmdLCD(GOTO_LINE1_POS0);
                StrLCD("Enter Day (1-31)");
                //U32LCD(max_days);
                //StrLCD("):");
                CmdLCD(GOTO_LINE3_POS0);
                StrLCD("#:Enter *:Clr B:Exit");
                
                val = rtc_read_num(2);
                if (val == 999999) break; // Timeout
                
                if (val >= 1 && val <= max_days)
                {
                    CCR = 0x02;
                    DOM = val;
                    CCR = 0x01;
                    
                    update_rtc_dow(); // Recalculate day of week
                    
                    CmdLCD(CLEAR_LCD);
                    CmdLCD(GOTO_LINE1_POS0);
                    StrLCD("Day Updated");
                    delay_ms(1500);
                    break;
                }
                
                CmdLCD(CLEAR_LCD);
                CmdLCD(GOTO_LINE1_POS0);
                StrLCD("Day Invalid");
                CmdLCD(GOTO_LINE2_POS0);
                StrLCD("Must be 1 to ");
                U32LCD(max_days);
                delay_ms(1500);
            }
        }
        else if (key == '5') // Month
        {
            while (1)
            {
                CmdLCD(CLEAR_LCD);
                CmdLCD(GOTO_LINE1_POS0);
                StrLCD("Enter Mon (1-12):");
                CmdLCD(GOTO_LINE3_POS0);
                StrLCD("#:Enter *:Clr B:Exit");
                
                val = rtc_read_num(2);
                if (val == 999999) break; // Timeout
                
                if (val >= 1 && val <= 12)
                {
                    u16 cur_y = YEAR;
                    u8 cur_d = DOM;
                    u8 max_days = 31;
                    if (val == 2)
                    {
                        if (cur_y % 4 == 0) max_days = 29;
                        else max_days = 28;
                    }
                    else if (val == 4 || val == 6 || val == 9 || val == 11)
                    {
                        max_days = 30;
                    }
                    
                    CCR = 0x02;
                    if (cur_d > max_days)
                    {
                        DOM = 1; // Safeguard out of range dates
                    }
                    MONTH = val;
                    CCR = 0x01;
                    
                    update_rtc_dow(); // Recalculate day of week
                    
                    CmdLCD(CLEAR_LCD);
                    CmdLCD(GOTO_LINE1_POS0);
                    StrLCD("Month Updated!");
                    delay_ms(1500);
                    break;
                }
                
                CmdLCD(CLEAR_LCD);
                CmdLCD(GOTO_LINE1_POS0);
                StrLCD("Month Invalid");
                CmdLCD(GOTO_LINE2_POS0);
                StrLCD("Must be 1 to 12");
                delay_ms(1500);
            }
        }
        else if (key == '6') // Year
        {
            while (1)
            {
                CmdLCD(CLEAR_LCD);
                CmdLCD(GOTO_LINE1_POS0);
                StrLCD("Enter Year (YYYY):");
                CmdLCD(GOTO_LINE3_POS0);
                StrLCD("#:Enter *:Clr B:Exit");
                
                val = rtc_read_num(4);
                if (val == 999999) break; // Timeout
                
                if (val >= 2000 && val <= 2099)
                {
                    u8 cur_m = MONTH;
                    u8 cur_d = DOM;
                    if (cur_m == 2 && cur_d == 29 && (val % 4 != 0))
                    {
                        CCR = 0x02;
                        DOM = 28; // Adjust Feb 29 to Feb 28 on non-leap years
                        YEAR = val;
                        CCR = 0x01;
                    }
                    else
                    {
                        CCR = 0x02;
                        YEAR = val;
                        CCR = 0x01;
                    }
                    
                    update_rtc_dow(); // Recalculate day of week
                    
                    CmdLCD(CLEAR_LCD);
                    CmdLCD(GOTO_LINE1_POS0);
                    StrLCD("Year Updated!");
                    delay_ms(1500);
                    break;
                }
                
                CmdLCD(CLEAR_LCD);
                CmdLCD(GOTO_LINE1_POS0);
                StrLCD("Year Invalid");
                CmdLCD(GOTO_LINE2_POS0);
                StrLCD("Must be 2000-2099");
                delay_ms(1500);
            }
        }
    }
}

extern volatile u8 rfid_ready;
extern unsigned char rfid_buffer[16];

/**
 * @brief  Scan and identify a user card for administrative edits.
 *         Returns user index (0 to NUM_USERS-1) or -1 on cancel/timeout.
 */
int scan_target_user(void)
{
    unsigned char card[16];
    u8 u_idx;
    extern void flush_rfid_reader(void);
    
    flush_rfid_reader(); // Reset scanner and flush FIFO
    
    CmdLCD(CLEAR_LCD);
    CmdLCD(GOTO_LINE1_POS0);
    StrLCD("--------------------");
    CmdLCD(GOTO_LINE2_POS0);
    StrLCD(" Scan Citizen Card  ");
    CmdLCD(GOTO_LINE3_POS0);
    StrLCD("--------------------");
    CmdLCD(GOTO_LINE4_POS0);
    StrLCD("*:Cancel      B:Exit");
    
    while (1)
    {
        if (auto_logout_flag) return -1;
        
        // Check if cancel was pressed
        if (ColScan() == 0)
        {
            char key = KeyScanWithTimeout(200);
            if (key == '*' || key == 'B')
            {
                return -1;
            }
        }
        
        if (rfid_ready)
        {
            for (u_idx = 0; u_idx < 15 && rfid_buffer[u_idx] != '\0'; u_idx++)
            {
                card[u_idx] = rfid_buffer[u_idx];
            }
            card[u_idx] = '\0';
            flush_rfid_reader();
            
            // Scan user database
            for (u_idx = 0; u_idx < NUM_USERS; u_idx++)
            {
                extern u8 is_same_id(const unsigned char *s1, const char *s2, u8 len);
                if (is_same_id(card, card_ids[u_idx], 8))
                {
                    return u_idx;
                }
            }
            
            // Invalid
            CmdLCD(CLEAR_LCD);
            CmdLCD(GOTO_LINE1_POS0);
            StrLCD("--------------------");
            CmdLCD(GOTO_LINE2_POS0);
            StrLCD("Invalid Citizen Card");
            CmdLCD(GOTO_LINE3_POS0);
            StrLCD("--------------------");
            delay_ms(2000);
            
            // Prompt again
            CmdLCD(CLEAR_LCD);
            CmdLCD(GOTO_LINE1_POS0);
            StrLCD("--------------------");
            CmdLCD(GOTO_LINE2_POS0);
            StrLCD(" Scan Citizen Card  ");
            CmdLCD(GOTO_LINE3_POS0);
            StrLCD("--------------------");
            CmdLCD(GOTO_LINE4_POS0);
            StrLCD("*:Cancel      B:Exit");
            
            flush_rfid_reader();
        }
        delay_ms(10);
    }
}

/**
 * @brief  Scan and identify the officer card.
 *         Returns 1 if officer card matches, 0 on cancel/timeout.
 */
u8 scan_officer_card(void)
{
    extern u8 is_same_id(const unsigned char *s1, const char *s2, u8 len);
    extern void flush_rfid_reader(void);
    unsigned char card[16];
    u8 u_idx;
    
    flush_rfid_reader(); // Reset scanner and flush FIFO
    
    CmdLCD(CLEAR_LCD);
    CmdLCD(GOTO_LINE1_POS0);
    StrLCD("--------------------");
    CmdLCD(GOTO_LINE2_POS0);
    StrLCD(" Scan Officer Card  ");
    CmdLCD(GOTO_LINE3_POS0);
    StrLCD("--------------------");
    CmdLCD(GOTO_LINE4_POS0);
    StrLCD("*:Cancel      B:Exit");
    
    while (1)
    {
        if (auto_logout_flag) return 0;
        
        // Check if cancel was pressed
        if (ColScan() == 0)
        {
            char key = KeyScanWithTimeout(200);
            if (key == '*' || key == 'B')
            {
                return 0;
            }
        }
        
        if (rfid_ready)
        {
            for (u_idx = 0; u_idx < 15 && rfid_buffer[u_idx] != '\0'; u_idx++)
            {
                card[u_idx] = rfid_buffer[u_idx];
            }
            card[u_idx] = '\0';
            flush_rfid_reader();
            
            // Check if card is the Officer Card ("12531874")
            if (is_same_id(card, "00315103", 8))
            {
                return 1;
            }
            
            // Invalid
            CmdLCD(CLEAR_LCD);
            CmdLCD(GOTO_LINE1_POS0);
            StrLCD("--------------------");
            CmdLCD(GOTO_LINE2_POS0);
            StrLCD("Officer Card Invalid");
            CmdLCD(GOTO_LINE3_POS0);
            StrLCD("--------------------");
            delay_ms(2000);
            
            // Prompt again
            CmdLCD(CLEAR_LCD);
            CmdLCD(GOTO_LINE1_POS0);
            StrLCD("--------------------");
            CmdLCD(GOTO_LINE2_POS0);
            StrLCD(" Scan Officer Card  ");
            CmdLCD(GOTO_LINE3_POS0);
            StrLCD("--------------------");
            CmdLCD(GOTO_LINE4_POS0);
            StrLCD("*:Cancel      B:Exit");
            
            flush_rfid_reader();
        }
        delay_ms(10);
    }
}

void license_edit_menu(int target_idx)
{
    u32 val;
    
    if (target_idx < 0 || target_idx >= NUM_USERS) return;
    
    while (1)
    {
        u16 cur_rtc_y = YEAR;
        
        if (auto_logout_flag) break;
        
        CmdLCD(CLEAR_LCD);
        CmdLCD(GOTO_LINE1_POS0);
        StrLCD("Edit Exp Yr: ");
        StrLCD((s8 *)names[target_idx]);
        CmdLCD(GOTO_LINE2_POS0);
        StrLCD("Enter Yr (YYYY):");
        CmdLCD(GOTO_LINE3_POS0);
        StrLCD("#:Enter *:Clr B:Exit");
        
        val = rtc_read_num(4);
        if (val == 999999 || auto_logout_flag) break;
        
        if (val >= cur_rtc_y && val <= 2099)
        {
            exp_years[target_idx] = val;
            
            CmdLCD(CLEAR_LCD);
            CmdLCD(GOTO_LINE1_POS0);
            StrLCD("Year Updated!");
            delay_ms(1500);
            break;
        }
        
        CmdLCD(CLEAR_LCD);
        CmdLCD(GOTO_LINE1_POS0);
        StrLCD("Year Invalid!");
        CmdLCD(GOTO_LINE2_POS0);
        if (val < cur_rtc_y)
        {
            StrLCD("Must be >= RTC Yr");
        }
        else
        {
            StrLCD("Must be ");
            U32LCD(cur_rtc_y);
            StrLCD("-2099");
        }
        delay_ms(1500);
    }
}

volatile u8 switch_pressed_flag = 0;

/**
 * @brief  Handle external switch press by verifying Officer Card and loading Officer Menu.
 */
void handle_external_switch(void)
{
    extern void officer_beep(void);
    
    // Sound buzzer for exactly 1 second on switch press
    IOSET0 = (1 << 19);  // Turn Buzzer ON (P0.19)
    delay_ms(1000);
    IOCLR0 = (1 << 19);  // Turn Buzzer OFF
    
    if (scan_officer_card())
    {
        officer_beep();
        
        // Open the Officer Menu
        officer_menu();
    }
    
    rtc_interrupted_flag = 1; // Mark that the screen needs to be redrawn
    CCR = 0x01;          // Ensure RTC is enabled when exiting editing
}

/**
 * @brief  EINT3 ISR handler on P0.30.
 *         Only triggers when outside citizen sessions. Sets flag to process in main loop context.
 */
void EINT3_ISR(void) __irq
{
    EXTINT = 0x08;       // Clear EINT3 interrupt flag (bit 3)
    
    // Ignore interrupt if a citizen user is logged in
    if (current_user_index >= 0 && current_user_index < NUM_USERS)
    {
        VICVectAddr = 0x00;  // End of Interrupt
        return;
    }
    
    switch_pressed_flag = 1;
    
    EXTINT = 0x08;       // Clear EINT3 interrupt flag (bit 3) again to clear any bounces
    VICVectAddr = 0x00;  // End of Interrupt
}

/**
 * @brief  Officer console options: Reset Voting database, System Time and Expiry edits.
 */
void officer_menu(void)
{
    char key;
    while (1)
    {
        CmdLCD(CLEAR_LCD);
        CmdLCD(GOTO_LINE1_POS0);
        StrLCD("1.Reset the Vote");
        CmdLCD(GOTO_LINE2_POS0);
        StrLCD("2.Edit the Driv_Lic");
        CmdLCD(GOTO_LINE3_POS0);
        StrLCD("*:Exit");
        
        key = KeyScanWithTimeout(20000);
        if (key == 0 || key == '*' || auto_logout_flag)
        {
            break;
        }
        
        if (key == '1')
        {
            // Reset voting status for ALL 3 users in EEPROM
            ByteWrite_25LC512(eeprom_vote_addrs[0], 0x00); delay_ms(5);
            ByteWrite_25LC512(eeprom_vote_addrs[1], 0x00); delay_ms(5);
            ByteWrite_25LC512(eeprom_vote_addrs[2], 0x00); delay_ms(5);
            
            CmdLCD(CLEAR_LCD);
            CmdLCD(GOTO_LINE1_POS0);
            StrLCD("Voting Reset Done");
            CmdLCD(GOTO_LINE2_POS0);
            StrLCD("Users can cast vote");
            delay_ms(2000);
        }
        else if (key == '2')
        {
            // Sub-menu for Driving License Edit
            char sub_key;
            while (1)
            {
                CmdLCD(CLEAR_LCD);
                CmdLCD(GOTO_LINE1_POS0);
                StrLCD("1.System Time Edit");
                CmdLCD(GOTO_LINE2_POS0);
                StrLCD("2.License Edit");
                CmdLCD(GOTO_LINE3_POS0);
                StrLCD("*:Exit");
                
                sub_key = KeyScanWithTimeout(20000);
                if (sub_key == 0 || sub_key == '*' || auto_logout_flag)
                {
                    break;
                }
                
                if (sub_key == '1')
                {
                    rtc_edit_menu();
                    CCR = 0x01; // Enable clock to make it run!
                    while (ColScan() == 0) delay_ms(10); // Wait for key release before scanning parent menu
                }
                else if (sub_key == '2')
                {
                    int target = scan_target_user();
                    if (target != -1)
                    {
                        u16 y = YEAR;
                        u8 expired = 0;
                        
                        // Check target citizen's license validity (Year only)
                        if (y > exp_years[target]) expired = 1;
                        
                        if (expired)
                        {
                            // Validity completed -> Officer can edit
                            license_edit_menu(target);
                        }
                        else
                        {
                            // License is still valid -> Unable to edit
                            CmdLCD(CLEAR_LCD);
                            CmdLCD(GOTO_LINE1_POS0);
                            StrLCD("License Valid");
                            CmdLCD(GOTO_LINE2_POS0);
                            StrLCD("Unable to Edit");
                            delay_ms(2500);
                        }
                    }
                    while (ColScan() == 0) delay_ms(10); // Wait for key release before scanning parent menu
                }
            }
        }
    }
}
