/*	Partner(s) Name & E-mail: NA
 *	Lab Section: B21
 *	Assignment: Final Project
 *	Exercise Description: [optional - include for your own benefit]
 *	
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "io.h"
#include "io.c"
#include "scheduler.h"

//All Global Variables
//========================

enum State1 {start, wait, scroll, select} state;
enum State2 {init3, up, down} state2;
enum State3 {init4, up2, charge, down2,} state3;
enum State4 {init2, calc, death} state5;

const unsigned char Explorer_sprite[] = {0x0E,0x1F,0x0E,0x0E,0x05,0x0F,0x14,0x0A};
const unsigned char Cannibal_sprite[] = {0x17,0x17,0x17,0x12,0x1E,0x13,0x12,0x15};
unsigned char sprites[];
unsigned char LCD_pos1 = 0x00;
unsigned char LCD_pos2 = 0x00;
unsigned char position = 0x00;

unsigned short logic_period = 500;
unsigned short player_period = 100;

unsigned char total_score = 0x00;
unsigned char hundred = 0x00;
unsigned char ten = 0x00;
unsigned char one = 0x00;
unsigned char final_score[];
unsigned char Death = 0x00;
	
	
unsigned char menu_selected = 0x00;
unsigned char initiate = 0x00;
unsigned char player_num = 0x00;
unsigned char game_over = 0x00;
static unsigned char spawn_time = 0x00;;

unsigned char GCD = 100;
volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.

unsigned char tmpA = 0x00;

unsigned char lightPattherns[] = {0x01, 0x02, 0x04, 0x02};

unsigned char maxPatterns = 4;

unsigned char nextPattern = 0;

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s

	// AVR output compare register OCR1A.
	OCR1A = 125;	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	// AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	//Enable global interrupts
	SREG |= 0x80; // 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}

// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

//Menu selecting number of players
void menu_select () {
	switch(state) {
		case start:
			LCD_Cursor(1);
			LCD_DisplayString(1, "Select Players");
			if(PORTB == 0x00)
			{
				state = wait;
			}
			else if(PORTB == 0x00)
			{
				state = wait;
			}
			else {
				state = start;
			}
			break;
		
		case wait:
			if(PORTB == 0x6)
			{
				initiate = 0x01;
				state = scroll;
			}
			else if(PORTB == 0x05)
			{
				if(initiate == 0x01)
				state = select;
			}
			else {
				state = wait;
			}
			break;
		//allows user to scroll through environment menu options
		case scroll:
			if(player_num == 0x00) {
				LCD_DisplayString(1, "1 Player");
				player_num = 0x01;
				state = wait;
			}
			else if (player_num == 0x01) {
				LCD_DisplayString(1, "2 Player");
				player_num = 0x00;
				state = wait;
			}
			break;
		case select:
			menu_selected = 0x01;
			state = wait;
			break;
		default:
			state = start;
			break;
	}
	
}
void calc_score(){
	hundred = total_score / 100;	
	total_score = total_score - hundred * 100;
	ten = total_score / 10;
	total_score = total_score - ten * 10;
	one = total_score;
	final_score[0] = 'S';
	final_score[1] = 'C';
	final_score[2] = 'O';
	final_score[3] = 'R';
	final_score[4] = 'E';
	final_score[5] = ' ';
	final_score[6] = '=';
	final_score[7] = hundred;
	final_score[8] = ten;
	final_score[9] = one;
}

//player 1 control and logic
int cannibal (int state3) {
	switch (state3) {
		case init4:
		if (~PINC & 0x04 == 0x04){
			state3 = up2;
		}
		else if (~PINC & 0x01 == 0x01){
			state3 = charge;
		}
		else if (~PINC & 0x02 == 0x02){
			state3 = down2;
		}
		else {
			if (spawn_time < 10){
				spawn_time++;
			}
			state3 = init4;
		}
		break;
		case up2:
		if (LCD_pos2 == 32){
			LCD_pos2 = 16;
			LCD_Cursor(32); LCD_WriteData(' ');
			LCD_Cursor(16); LCD_WriteData(3);
		}
		if (~PINC & 0x04 == 0x04){
			state = down2;
		}
		else {
			state3 = init4;
		}
		break;
		case down2:
		if (LCD_pos2 == 16){
			LCD_pos2 = 32;
			LCD_Cursor(16); LCD_WriteData(' ');
			LCD_Cursor(32); LCD_WriteData(3);
		}
		if (~PINC & 0x01 == 0x01){
			state = up2;
		}
		else {
			state3 = init4;
		}
		break;
		case charge:
		if (spawn_time < 10){
			spawn_time++;
		}
		else if (spawn_time >= 10) {
			spawn_time = 0;
			sprites[position] = LCD_pos2;
			position++;
		}
		state3 = init4;
		break;
		default:
		state3 = init4;
		break;
	}
	return state3;
}
//player2 controls and logic
int explorer (int state2) {
	switch (state2) {
		case init3:
			if (~PINB & 0x01 == 0x01){
				state2 = up;
			}
			else if (~PINB & 0x04 == 0x04){
				state2 = down;
			}
			else{
				state2 = init3;
			}
			break;
		case up:
			LCD_pos1 = 1;
			LCD_Cursor(17); LCD_WriteData(' ');
			LCD_Cursor(1); LCD_WriteData(1);
			if (~PINB & 0x04 == 0x04) {
				state2 = down;
			}
			else { state2 = up;}
			break;
		case down:
			LCD_pos1 = 17;
			LCD_Cursor(1); LCD_WriteData(' ');
			LCD_Cursor(17); LCD_WriteData(1);
			if (~PINB & 0x01 == 0x01) {
				state2 = up;
			}
			else { state2 = down; }
			break;
		default:
			state2 = init3;
			break;
	}
	return state2;
}

//Runs the game intro, calculates movement of attacking sprites and declares game over
int Logic (int state5) {
	switch (state5) {
		//init displays introduction
		case init4:
			position = 0;
			total_score = 0;
			LCD_WriteCommand(0x0C);
			LCD_Custom_Char(1, Explorer_sprite);
			LCD_Custom_Char(3, Cannibal_sprite);
			LCD_WriteCommand(0x80);
			
			LCD_ClearScreen();
			LCD_DisplayString(1, "Let's Play");
			delay_ms(3000);
			LCD_DisplayString(1, "AMAZON EXPLORER");
			delay_ms(3000);
			LCD_DisplayString(1, "AVOID CANNIBALS");
			delay_ms(3000);
			LCD_ClearScreen();
			state5 = calc;
			break;
		//
		case calc:
			if(Death == 0x01) {
				state5 = death;
			}
			for (unsigned char i = 0; i < position; i++){
				if (LCD_pos1 == sprites[i]){
					Death = 0x01;
				}
				else if ((sprites[i]-2) % 16 == 0){
					total_score++;
					calc_score();
				}
				sprites[i]--;
				if (sprites[i] % 16 == 0){
					for (unsigned char j = i; j < position; j++){
						sprites[j] = sprites[j+1];
					}
					i--;
					position--;
				}
				if (sprites[i] < 33){
					LCD_Cursor(sprites[i]); LCD_WriteData(3);
					LCD_Cursor(sprites[i]-16); LCD_WriteData(' ');
				}
				else if (sprites[i] < 17){
					LCD_Cursor(sprites[i]+1); LCD_WriteData(' ');
					LCD_Cursor(sprites[i]); LCD_WriteData(3);
				}
				else {
					LCD_Cursor(sprites[i]-16); LCD_WriteData(3);
					LCD_Cursor(sprites[i]-15); LCD_WriteData(' ');
				}
			}

			break;
		case death:
			LCD_DisplayString(1, final_score);
			delay_ms(5000);
			LCD_DisplayString(1, "GAME OVER!!!");
			delay_ms(5000);
			state5 = death;
			break;
		default:
			state5 = init4;
			break;
		}
	return state5;
}

int main(void)
{
	DDRA = 0xFF; PORTA = 0x00;
	DDRB = 0x00; PORTB = 0xFF;
	DDRC = 0x00; PORTC = 0xFF;
	DDRD = 0xFF; PORTD = 0x00;
	
	LCD_init();
	TimerSet(GCD);
	TimerOn();
	LCD_pos1 = 17;
	LCD_pos2 = 32;
	
	 static task task1, task2, task3;
	 task *tasks[] = { &task1, &task2, &task3};
		 
	
	task1.state = init2;
	task1.period = logic_period;
	task1.elapsedTime = logic_period;
	task1.TickFct = &Logic;
	
	task2.state = init4;
	task2.period = player_period;
	task2.elapsedTime = player_period;
	task2.TickFct = &cannibal;
	
	task3.state = init3;
	task3.period = player_period;
	task3.elapsedTime = player_period;
	task3.TickFct = &explorer;

	
	while (1) {
// 		if (menu_selected == 0x00) {
// 			menu_select();
// 		}
// 		if(B == 0x03) {
// 			menu_selected = 0x00;
// 			display_intro = 0x00;
// 			initiate = 0x00;
// 		}
// 		if(menu_selected == 0x01 && display_intro == 0x00) {
// 			intro();
// 			display_intro = 0x01;
	}
}
