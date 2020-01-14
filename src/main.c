/**
 * AVR Slideshow
 * by Peter Varga
 *
 * ...
 *
 * modified into gatherer game
 * by N0rbi
 */

#ifdef NO_AVR
#include <stdio.h>
#include "buttonsim.h"
#include "lcdsim.h"
#else
#include "avr/io.h"
#include "lcd.h"
#endif // NO_AVR

#include <stdint.h>
#include <stdlib.h>
#include "utils.h"

#ifndef NO_AVR
#define __AVR_ATMEGA128__ 1

#define CG_RAM_ADDR       0x40

#define BUTTON_NONE   0
#define BUTTON_CENTER 1
#define BUTTON_LEFT   2
#define BUTTON_RIGHT  3
#define BUTTON_UP     4
#define BUTTON_DOWN   5
#endif // NO_AVR

#define MAX_LEN 96
#define END_GAME_THRESHOLD 32
#define LCD_WIDTH 16

#define CHAR_WIDTH 1

#ifndef NO_AVR
// GENERAL INIT - USED BY ALMOST EVERYTHING ----------------------------------

static void port_init() {
  PORTA = 0b00000000; DDRA = 0b01000000; // buttons
  PORTB = 0b00000000; DDRB = 0b00000000;
  PORTC = 0b00000000; DDRC = 0b11110111; // lcd
  PORTD = 0b11000000; DDRD = 0b00001000;
  PORTE = 0b00100000; DDRE = 0b00110000; // buzzer
  PORTF = 0b00000000; DDRF = 0b00000000;
  PORTG = 0b00000000; DDRG = 0b00000000;
}

// TIMER-BASED RANDOM NUMBER GENERATOR ---------------------------------------

static void rnd_init() {
  TCCR0 |= (1  << CS00);  // Timer 0 no prescaling (@FCPU)
  TCNT0 = 0;              // init counter
}

// generate a value between 0 and max
static int rnd_gen(int max) {
  return TCNT0 % max;
}

// BUTTON HANDLING -----------------------------------------------------------
static int button_accept = 1;

static int button_pressed() {
  // down
  if (!(PINA & 0b00000001) & button_accept) { // check state of button 1 and value of button_accept
    button_accept = 0; // button is pressed
    return BUTTON_DOWN;
  }

  // left
  if (!(PINA & 0b00000010) & button_accept) { // check state of button 2 and value of button_accept
    button_accept = 0; // button is pressed
    return BUTTON_LEFT;
  }

  // center
  if (!(PINA & 0b00000100) & button_accept) { // check state of button 3 and value of button_accept
    button_accept = 0; // button is pressed
    return BUTTON_CENTER;
  }

  // right
  if (!(PINA & 0b00001000) & button_accept) { // check state of button 4 and value of button_accept
    button_accept = 0; // button is pressed
    return BUTTON_RIGHT;
  }

  // up
  if (!(PINA & 0b00010000) & button_accept) { // check state of button 5 and value of button_accept
    button_accept = 0; // button is pressed
    return BUTTON_UP;
  }

  return BUTTON_NONE;
}

static void button_unlock() {
  //check state of all buttons
  if (
    ((PINA & 0b00000001)
    |(PINA & 0b00000010)
    |(PINA & 0b00000100)
    |(PINA & 0b00001000)
    |(PINA & 0b00010000)) == 31)
  button_accept = 1; //if all buttons are released button_accept gets value 1
}

static int button_released() {
    button_unlock();
    return button_accept;
}

// LCD HELPERS ---------------------------------------------------------------

static void lcd_delay(unsigned int a) {
  while (a)
    a--;
}

static void lcd_pulse() {
  PORTC = PORTC | 0b00000100; //set E to high
  //lcd_delay(1400);          //delay ~110ms
  lcd_delay(700);
  PORTC = PORTC & 0b11111011; //set E to low
}

static void lcd_send(int command, unsigned char a) {
  unsigned char data;

  data = 0b00001111 | a;                //get high 4 bits
  PORTC = (PORTC | 0b11110000) & data;  //set D4-D7
  if (command)
    PORTC = PORTC & 0b11111110;         //set RS port to 0 -> display set to command mode
  else
    PORTC = PORTC | 0b00000001;         //set RS port to 1 -> display set to data mode
  lcd_pulse();                          //pulse to set D4-D7 bits

  data = a<<4;                          //get low 4 bits
  PORTC = (PORTC & 0b00001111) | data;  //set D4-D7
  if (command)
    PORTC = PORTC & 0b11111110;         //set RS port to 0 -> display set to command mode
  else
    PORTC = PORTC | 0b00000001;         //set RS port to 1 -> display set to data mode
  lcd_pulse();                          //pulse to set d4-d7 bits
}

static void lcd_send_command(unsigned char a) {
  lcd_send(1, a);
}

static void lcd_send_data(unsigned char a) {
  lcd_send(0, a);
}

static void lcd_init() {
  //LCD initialization
  //step by step (from Gosho) - from DATASHEET

  PORTC = PORTC & 0b11111110;

  lcd_delay(10000);

  PORTC = 0b00110000;   //set D4, D5 port to 1
  lcd_pulse();          //high->low to E port (pulse)
  lcd_delay(1000);

  PORTC = 0b00110000;   //set D4, D5 port to 1
  lcd_pulse();          //high->low to E port (pulse)
  lcd_delay(1000);

  PORTC = 0b00110000;   //set D4, D5 port to 1
  lcd_pulse();          //high->low to E port (pulse)
  lcd_delay(1000);

  PORTC = 0b00100000;   //set D4 to 0, D5 port to 1
  lcd_pulse();          //high->low to E port (pulse)

  lcd_send_command(DISP_ON);    // Turn ON Display
  lcd_send_command(CLR_DISP);   // Clear Display
}

static void lcd_send_text(char *str) {
  while (*str)
    lcd_send_data(*str++);
}

static void lcd_send_line1(char *str) {
  lcd_send_command(DD_RAM_ADDR);
  lcd_send_text(str);
}

static void lcd_send_line2(char *str) {
  lcd_send_command(DD_RAM_ADDR2);
  lcd_send_text(str);
}
#else
static int rnd_gen(int max) {
  return rand() % max;
}
#endif // NO_AVR


// CUSTOM STUFF --------------------------------------------------------------

typedef struct Game_str {
    int playerPos, playerWidth, playerScore, maxScore;
} Game;

static void generate_map(unsigned char map[], int map_size) {
    int i=0;
    while (i < map_size-END_GAME_THRESHOLD) {
        for(int j=0; j < 5; j++) {
            (map)[i++] = 0x0;
        }

        (map)[i++] = (char)rnd_gen(8);
    }
    do {
        map[i++] = 0;
    } while (i < map_size);

}

static void render_player(unsigned char* line0_buffer, unsigned char* line1_buffer, Game game, int gameOffset) {
    int start = MAX((game.playerPos - (game.playerWidth / 2)), 0);
    int end = MIN((game.playerPos + (game.playerWidth / 2)), 7);

    for (int i=start; i <= end; i++) {
        line0_buffer[gameOffset] |= (1 << (i)) & 0b1111;
        line1_buffer[gameOffset] |= ((1 << (i)) >> 4) & 0b1111;
    }

}

static unsigned char pattern_map[16];

// Key: Pattern
// Value: Offset in CG RAM or character in ROM
void init_pattern_map()
{
    pattern_map[0] = ' ';
    for (int i = 1; i < 16; ++i)
        pattern_map[i] = '?';
}

// Bubble short pattern frequencies
// Skip zero (0x0): space character is already stored in ROM
void sort_patterns(unsigned char patterns[16], int pattern_freqs[16])
{
    for (int i = 0; i < 15; ++i) {
        for (int j = 0; j < 14; ++j) {
            unsigned char a = pattern_freqs[patterns[j]];
            unsigned char b = pattern_freqs[patterns[j + 1]];

            if (a < b) {
                unsigned char temp = patterns[j + 1];
                patterns[j + 1] = patterns[j];
                patterns[j] = temp;
            }
        }
    }
}

// Try to keep pointer of pattern in CG RAM
void fill_cg_buffer(unsigned char cg_buffer[8], unsigned char patterns[16])
{
    // Each bit represents a pattern. If bit is 1 the pattern is already stored.
    unsigned short stored_mask = 0;

    // Each bit represents a memory slot in CG RAM. If bit is 1 the slot is reserved.
    unsigned char reserved_mask = 0;

    // Mark patterns that are already stored in CG RAM
    for (int i = 0; i < 8; ++i) {
        unsigned char pattern = patterns[i];
        unsigned char cg_pos = pattern_map[pattern];

        // Pattern is already in CG RAM
        if (cg_pos < 8) {
            cg_buffer[cg_pos] = pattern;
            stored_mask |= 1 << pattern;
            reserved_mask |= 1 << cg_pos;
        }
    }

    // Add new patterns to CG RAM
    for (int i = 0; i < 8; ++i) {
        unsigned char pattern = patterns[i];

        // Pattern is already stored
        if (stored_mask & (1 << pattern))
            continue;

        // Find empty slot in CG RAM
        unsigned char cg_pos = 0;
        for (; cg_pos < 8; ++cg_pos) {
            if (~reserved_mask & (1 << cg_pos))
                break;

            // No more free slot
            if (cg_pos == 7) {
#ifdef NO_AVR
                button_sim_terminate();
                lcd_sim_terminate();

                fprintf(stderr, "CG RAM BUFFER ERROR\n");
#else
                lcd_send_line1("  !!! ERROR !!! ");
                lcd_send_line2("CG RAM BUFFER   ");
#endif
                exit(1);
            }
        }

        cg_buffer[cg_pos] = pattern;
        stored_mask |= 1 << pattern;
        reserved_mask |= 1 << cg_pos;
    }

    for (int i = 1; i < 16; ++i) {
        if (stored_mask & (1 << i))
            continue;
        pattern_map[i] = '?';
    }
}

// Load first 8 patterns to the CG RAM (if non-zero!)
void update_cg_ram(unsigned char cg_buffer[8])
{
    for (int i = 0; i < 8; ++i) {
        unsigned char pattern = cg_buffer[i];
        if (!pattern)
            break;

        // Pattern is already in CG RAM
        if (pattern_map[pattern] == i)
            continue;

        pattern_map[pattern] = i;

        lcd_send_command(CG_RAM_ADDR + i*8);
        // Pattern size is 4bit
        for (int j = 0; j < 4; ++j) {
            unsigned char row = 0x00;
            if ((pattern >> j) & 1)
                row = 0xff;
            lcd_send_data(row);
            lcd_send_data(row);
        }
    }
}

unsigned char diff_bits_in_pattern(unsigned char a, unsigned char b)
{
    unsigned char count = 0;

    count += (0x01 & (a >> 0)) ^ (0x01 & (b >> 0));
    count += (0x01 & (a >> 1)) ^ (0x01 & (b >> 1));
    count += (0x01 & (a >> 2)) ^ (0x01 & (b >> 2));
    count += (0x01 & (a >> 3)) ^ (0x01 & (b >> 3));

    return count;
}


unsigned char find_similar_pattern(unsigned char unloaded_pattern, unsigned char cg_buffer[8])
{
    unsigned char similar_pattern = 0x0;
    unsigned char min_diff = diff_bits_in_pattern(unloaded_pattern, similar_pattern);

    if (min_diff <= 1)
        return 0x0;

    for (int i = 0; i < 8; ++i) {
        unsigned char cg_pattern = cg_buffer[i];
        unsigned char diff = diff_bits_in_pattern(unloaded_pattern, cg_pattern);

        if (diff < min_diff) {
            min_diff = diff;
            similar_pattern = cg_pattern;
        }
    }

    return similar_pattern;
}

void show(unsigned char *line0_buffer, unsigned char *line1_buffer, int buffer_index, int line_len)
{
    unsigned char line0_shadow[LCD_WIDTH];
    unsigned char line1_shadow[LCD_WIDTH];

    int pattern_freqs[16] = { 0, 0, 0, 0,
                              0, 0, 0, 0,
                              0, 0, 0, 0,
                              0, 0, 0, 0 };

    for (int i = 0; i < MIN(LCD_WIDTH, line_len); ++i, ++buffer_index) {
        if (buffer_index >= line_len)
            buffer_index = 0;

        line0_shadow[i] = line0_buffer[buffer_index];
        line1_shadow[i] = line1_buffer[buffer_index];

        pattern_freqs[line0_shadow[i]]++;
        pattern_freqs[line1_shadow[i]]++;
    }

    unsigned char patterns[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x0 };
    sort_patterns(patterns, pattern_freqs);

    unsigned char cg_buffer[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    fill_cg_buffer(cg_buffer, patterns);

    update_cg_ram(cg_buffer);

    // Last element of cg_buffer is set -> there might be unloaded patterns
    if (cg_buffer[7]) {
        // Replace unloaded but used patterns by the most most similar loaded pattern in shadow memory
        for (int i = 8; i < 15; ++i) {
            unsigned char unloaded_pattern = patterns[i];
            if (!pattern_freqs[unloaded_pattern])
                break;

            // Find most similar pattern in CG RAM
            unsigned char similar_pattern = find_similar_pattern(unloaded_pattern, cg_buffer);

            // Replace occurences of unloaded pattern in shadow memory
            for (int j = 0; j < LCD_WIDTH; ++j) {
                if (line0_shadow[j] == unloaded_pattern)
                    line0_shadow[j] = similar_pattern;

                if (line1_shadow[j] == unloaded_pattern)
                    line1_shadow[j] = similar_pattern;
            }
        }
    }

    lcd_send_command(DD_RAM_ADDR);
    for (int i = 0; i < LCD_WIDTH; ++i)
        lcd_send_data(pattern_map[line0_shadow[i]]);

    lcd_send_command(DD_RAM_ADDR2);
    for (int i = 0; i < LCD_WIDTH; ++i)
        lcd_send_data(pattern_map[line1_shadow[i]]);

#ifdef NO_AVR
    lcd_sim_print();
#endif
}

typedef struct {
    int freq;
    int length;
} tune_t;

static tune_t TUNE_START[] = { { 2000, 40 }, { 0, 0 } };
static tune_t TUNE_GOOD[] = { { 3000, 20 }, { 0, 0 } };
static tune_t TUNE_PERFECT[] = { { 4000, 20 }, { 0, 0 } };
static tune_t TUNE_GAMEOVER[] = { { 1000, 200 }, { 1500, 200 }, { 2000, 400 }, { 0, 0 } };


#ifdef NO_AVR
int PORTE = 0b00100000;
#endif
static void play_note(int freq, int len) {
    for (int l = 0; l < len; ++l) {
        int i;
        PORTE = (PORTE & 0b11011111) | 0b00010000;	//set bit4 = 1; set bit5 = 0
        for (i=freq; i; i--);
        PORTE = (PORTE | 0b00100000) & 0b11101111;	//set bit4 = 0; set bit5 = 1
        for (i=freq; i; i--);
    }
}

static void play_tune(tune_t *tune) {
#ifndef NO_AVR
    while (tune->freq != 0) {
        play_note(tune->freq, tune->length);
        ++tune;
    }
#endif
}


// MAIN ENTRY POINT ----------------------------------------------------------

int main(void)
{
#ifdef NO_AVR
    button_sim_init();
#else
    port_init();
#endif
    lcd_init();

    const int buffer_size = MAX_LEN + END_GAME_THRESHOLD;
    unsigned char line0_buffer[buffer_size];
    unsigned char line1_buffer[buffer_size];
    float bestScore = 0.0;
    while(1) {
        unsigned char map[buffer_size];

        Game game;
        game.playerPos = 4;
        game.maxScore = 0;
        game.playerScore = 0;
        game.playerWidth = 3;

        generate_map(map, buffer_size);

        for (int i = 0; i < buffer_size; ++i) {
            line0_buffer[i] = 0;
            line1_buffer[i] = 0;
        }

        for (int i = 0; i < buffer_size; ++i) {
            if (map[i] == 0) continue;
            line0_buffer[i] = (1 << (map[i]-1)) & 0b1111;
            line1_buffer[i] = ((1 << (map[i]-1)) >> 4) & 0b1111;
        }


        init_pattern_map();

        render_player(line0_buffer, line1_buffer, game, 0);

        const int line_len = buffer_size;
        int buffer_index = 0;
        int enable_slide = (line_len - 3) > LCD_WIDTH;

        uint16_t cycle = 0;

        lcd_send_line1("  Gatherer");
        lcd_send_line2("  by N0rbi");
        play_tune(TUNE_START);
        while (cycle++ < 10000 && !button_pressed()) {
#ifdef NO_AVR
            lcd_delay(0);
#else
            lcd_delay(100);
#endif
            button_unlock();
        }

        // Show first frame before loop starts
        show(&line0_buffer[0], &line1_buffer[0], 0, line_len);

        cycle = 0;
        while (cycle++ < UINT16_MAX) {
            if (cycle >= 65000U)
                cycle = 0;

            int button = button_pressed();
            if (button == BUTTON_UP) {
                game.playerPos = MAX(game.playerPos - 1, game.playerWidth / 2);
            } else if (button == BUTTON_DOWN) {
                game.playerPos = MIN(game.playerPos + 1, 7 - game.playerWidth / 2);
            }
#ifdef NO_AVR
            if (button == BUTTON_QUIT)
            break;
#endif

                if (++buffer_index >= line_len - END_GAME_THRESHOLD) {
                    break; //end of game
                    play_tune(TUNE_GAMEOVER);
                }
                render_player(line0_buffer, line1_buffer, game, buffer_index);
                show(&line0_buffer[0], &line1_buffer[0], buffer_index, line_len);
                //update score:
                int current_item_position = map[buffer_index] - 1;
                if (current_item_position != -1) {
                    int start = MAX((game.playerPos - (game.playerWidth / 2)), 0);
                    int end = MIN((game.playerPos + (game.playerWidth / 2)), 7);
                    if (current_item_position < game.playerWidth / 2 || current_item_position > (7 - (game.playerWidth / 2))) {
                        // the player has no chance of getting the food with the center of their body
                        game.maxScore += 3;
                    } else {
                        game.maxScore += 5;
                    }

                    if (start <= current_item_position && end >= current_item_position) {
                        game.playerScore +=3;
                        if (game.playerPos == current_item_position) {

                            game.playerScore += 2;
                            play_tune(TUNE_PERFECT);
                        } else {
                            play_tune(TUNE_GOOD);
                        }
                    }
                }
#ifdef NO_AVR
                lcd_delay(70);
#else
                lcd_delay(10);
#endif


            button_unlock();
        }


        char scoreStr[16];

        float score = 1.0 * game.playerScore / game.maxScore;

        for (int i=0; i<16; i++) {
            scoreStr[i] = ' ';
        }
        int i = 0;
        for (int j=1; j<5; j++) {
            scoreStr[i++] = ((int)(score * power(10, j)) % 10) + '0';
            if (j == 2) {
                scoreStr[i++] = '.';
            }
        }

        scoreStr[i++] = '%';

        for (int i = 0; i < buffer_size; ++i) {
            line0_buffer[i] = 0;
            line1_buffer[i] = 0;
        }

        show(&line0_buffer[0], &line1_buffer[0], 0, line_len);



        bestScore = MAX(bestScore, score);

        i = 16 - 6;
        for (int j=1; j<5; j++) {
            scoreStr[i++] = ((int)(bestScore * power(10, j)) % 10) + '0';
            if (j == 2) {
                scoreStr[i++] = '.';
            }
        }
        scoreStr[i++] = '%';
        scoreStr[i++] = '\0';

        lcd_send_line1("Score:    Best:");
        lcd_send_line2(scoreStr);

        while(1) {
            int button = button_pressed();
            if (button == BUTTON_CENTER) {
                break;
            }
            #ifdef NO_AVR
                if (button == BUTTON_QUIT)
                    goto EXIT_PROGRAM;
            #endif
        }
    }

#ifdef NO_AVR
    EXIT_PROGRAM: button_sim_terminate();
    lcd_sim_terminate();
#endif

    return 0;
}
