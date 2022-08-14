#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "lcd.h"
#include "buttons.h"
#include <util/delay.h>
#include <avr/eeprom.h>

char seconds = 0;
void timer(void);
int init(void);
void introduction(void);


// A timer in seconds
void timer(void){
	seconds++;
}

// Funktio generoi s numeron sarjan
int generatecode(char* code, int s)
{
	// Merkit‰‰n taulukon loppumerkki
	code[s] = '\0';

	// M‰‰ritell‰‰n siemenluku ajastimelta
	int seed = TCNT1;

	// Alustetaan random generaattori
	srand(seed);

	// M‰‰ritell‰‰n nelj‰numeroinen koodi
	for (int i=0; i < s; i++)
	{
		code[i] = '0' + (rand() % 6 + 1);
	}

	return 0;
}

// Tulostaa history-taulun m‰‰ritellyn kierroksen
// TAULUN RAKENNE: ARVAUS+OIKEAT+VƒƒRƒT
void print_history(char *history, int kierros, int code_size)
{
		// Tulostetaan historia-taulun arvaus
		for(int i=0; i < (code_size); i++)
		{
			lcd_write_data(history[i+(kierros*(code_size+2))]);
		}

		// Tulostetaan v‰li coden j‰lkeen
		lcd_write_data(' ');

		// Tulostetaan oikealla paikalla olevat numerot
		for(int i=code_size; i < (code_size+1); i++)
		{
			lcd_write_data(history[i+(kierros*(code_size+2))]);
		}

		// Tulostetaan v‰li
		lcd_write_data(' ');

		// Tulostetaan v‰‰r‰ll‰ paikalla olevat numerot
		for(int i=(code_size+1); i < (code_size+2); i++)
		{
			lcd_write_data(history[i+(kierros*(code_size+2))]);
		}	
}


/* Set cursor location */
void set_cursor_location(unsigned char x, unsigned char y)
{
	if (y==0) lcd_write_ctrl(LCD_DDRAM | (0x00+x));
	else lcd_write_ctrl(LCD_DDRAM | (0x40+x));
}

// Funktio tarkistaa merkit oikealla paikalla ja v‰‰rill‰ paikoilla
// ja lis‰‰ historiariveille tiedon t‰st‰
// Jos arvaus on oikeain, palautetaan arvo 1. Muutoin palautetaan arvo 0.
int check_guess(char *code, char *guess, int code_size, char *history, int *ptr)
{
		int right_place=0;
		int wp=0;

		int kierros = *ptr;

		// M‰‰ritell‰‰n v‰liaikaistaulu, jota v‰‰r‰ll‰ paikalla olevien numeroiden m‰‰ritt‰iseen
		char code_temp[code_size+1];
		for(int i=0; i < code_size; i++)
		{
			code_temp[i]=code[i];
		}

		/* Verrataan montako on oikealla paikalla ja montako v‰‰r‰ll‰ */
		for (int i=0; i < code_size; i++)
		{
			if (guess[i]==code[i])
			{
			 	right_place++;
				// Jos numero lˆytyy, poistetaan v‰liaikaistaulusta
				code_temp[i]='0';
			}

			else
			{
				for (int k=0; k < code_size; k++)
				{
					if((guess[i]==code_temp[k]) && (guess[k]!=code_temp[k]))
					{
						wp++;
						// Jos numero lˆytyy, poistetaan v‰liaikaistaulusta
						code_temp[k]='0';
						// Rikotaan looppi numeron lˆydytty‰ duplikaattien v‰ltt‰miseksi
						break;
					}
				}
			}
		
		}


		set_cursor_location(10,1);
		for(int i=0; i < code_size; i++)
		{
			lcd_write_data(code[i]);
		}

		// Tallennetaan tiedot arvauksesta history-tauluun
		for(int i=0; i < code_size; i++)
		{
			history[i+(kierros*(code_size+2))]=guess[i];
		}		
		for(int i=code_size; i < (code_size+2); i++)
		{
			history[code_size+(kierros*(code_size+2))]='0'+right_place;
			history[code_size+(kierros*(code_size+2))+1]='0'+wp;
		}

		// Tulostetaan historyn viimeinen rivi yl‰riville
		set_cursor_location(1,0);
		print_history(history, kierros, code_size);
		
		// Kasvatetaan kierrosluku
		++(*ptr);

		// Funktio palauttaa arvon 1, mik‰li ratkaisu on oikein
		if(right_place==code_size) return(1);
		// Muuten palautetaan arvo 0
		else return(0);
}

void browse_history(int kierros, int code_size, char *history)
{
	// Set off a blinking cursor
	lcd_write_ctrl(LCD_ON | 0x00);
	kierros=kierros-1;
	int kierros_max=kierros;
	
	while(!BUTTON4_DOWN)
	{
		set_cursor_location(0,1);

		// Tulostetaan kierroksen s historia rivi yl‰riville
		set_cursor_location(1,0);
		print_history(history, kierros, code_size);

		// Tulostetaan k‰ynniss‰ oleva kierros
		set_cursor_location(14,0);
		lcd_write_data('0'+((kierros+1)/10));
		lcd_write_data('0'+((kierros+1)%10));

		if(BUTTON1_DOWN)
		{
			if(kierros!=0) kierros--;
		}

		if(BUTTON5_DOWN)
		{
			if(kierros!=kierros_max) kierros++;
		}

		_delay_ms(400);
	}

	// Blinking back on
	lcd_write_ctrl(LCD_ON | 0x01);
}

void scores(int kierros, char *scores_old)
{
	// Alustetaan muuttuja pisteille
	int scores=1000;

	// Laskukaava 1000p - k‰ytetty aika - 10p per arvaus
	scores = scores - seconds - (kierros*10);

	// Pisteet eiv‰t voi menn‰ minukselle
	if(scores < 0) scores = 0;


	// Tulostetaan pisteet alariville
	char scores_text[4]= "000";
	scores_text[0]='0'+scores/100;
	scores_text[1]='0'+(scores%100)/10;
	scores_text[2]='0'+(scores%10);

	for(int i=0; i < 3; i++)
	{
		lcd_write_data(scores_text[i]);
	}

	// Write data to eeprom
	if((scores > atoi(scores_old)) || (atoi(scores_old) > 1000))
	{
		eeprom_write_block(scores_text, (const void*)10, 4);
	}

}

// Funktio arvauksen syˆtt‰miseen
void insert_guess(char *code, int code_size, char *history, char *scores_text)
{
	// M‰‰ritell‰‰n min ja max, johon asti arvoa voi muuttaa
	char min='1';
	char max='6';

	// Alustetaan muuttujia
	int kierros=0;
	int cursor_location = 1;
	char guess[] = "1111";

	// Nollataan ajanlasku ja tyhjennet‰‰n n‰yttˆ
	seconds = 0;
	lcd_write_ctrl(LCD_CLEAR);

	do
	{
		_delay_ms(500);
		// History on k‰ytˆss‰ ensimm‰isen kierroksen j‰lkeen
		if (kierros != 0)
		{
			// Tulostetaan nuoli ylˆs
			set_cursor_location(0,0);
			lcd_write_data(0x00);
			// Tulostetaan nuoli alas
			set_cursor_location(0,1);
			lcd_write_data(0x01);
		}

	
		// Let's print the basic guess
		set_cursor_location(1,2);
		int i=0;
		while (guess[i])
		{
			lcd_write_data(guess[i]);
			i++;
		}

		// Set cursor in a blinking mode
		lcd_write_ctrl(LCD_ON | 0x01);

		// Initialize cursor position 
		set_cursor_location(cursor_location,1);

		//int key_pressed=0;


		// Let's accept the code with the middle button
		// Accept changes after middle button (!BUTTON3_DOWN)
		while (!BUTTON3_DOWN)
		{
			/* Button 2 moves a cursor to left */
			if (BUTTON2_DOWN) if (cursor_location!=0) 
			{
				cursor_location--;
				set_cursor_location(cursor_location,1);
				_delay_ms(200);
			}
			/* Browse history at left */
			if ((cursor_location==0) && (kierros==0)) cursor_location++;
			if (cursor_location==0)
			{
				browse_history(kierros, code_size, history);
				cursor_location=1;
				set_cursor_location(cursor_location,1);
				_delay_ms(200);
			}
			/* Button 4 moves a cursor to right */
			if (BUTTON4_DOWN) if (cursor_location!=code_size)
			{
				cursor_location++;
				set_cursor_location(cursor_location,1);
				_delay_ms(200);
			}
			/* Button 1 increases a value */
			if (BUTTON1_DOWN)
			{
				if (guess[cursor_location-1]==max);
				else guess[cursor_location-1]=guess[cursor_location-1]+1;
				lcd_write_data(guess[cursor_location-1]);
				set_cursor_location(cursor_location,1);
				_delay_ms(200);
			}
			/* Button 5 decreases a value */
			if (BUTTON5_DOWN)
			{
				if (guess[cursor_location-1]==min);
				else guess[cursor_location-1]=guess[cursor_location-1]-1;
				lcd_write_data(guess[cursor_location-1]);
				_delay_ms(200);
				set_cursor_location(cursor_location,1);
			}
			set_cursor_location(cursor_location,1);
		}
		_delay_ms(200);

	// Tulostetaan kierroluku
	set_cursor_location(14,0);
	lcd_write_data('0'+((kierros+1)/10));
	lcd_write_data('0'+((kierros+1)%10));
		

	} while(!check_guess(code, guess, code_size, history, &kierros)); // Toistetaan, kunnes arvataan oikein

	lcd_write_ctrl(LCD_CLEAR); /* tyhjent‰‰ n‰ytˆn */

	char text[]="Vaitit!";
	for (int i=0; i < 7; i++)
	{
		lcd_write_data(text[i]);
	}

	scores(kierros, scores_text);

		//Timer3, musiikki
		cli();
	
		DDRE |= (1 << PE4) | (1 << PE5);
		PORTE |= (1 << PE5);
		PORTE &= ~(1 << PE4);
		TCCR3A &= ~( (1 << WGM31) | (1 << WGM30) );
  		TCCR3B |=    (1 << WGM32);
  		TCCR3B &=   ~(1 << WGM33);
   		/* enable Output Compare A Match Interrupt */
  		//ETIMSK |= (1 << OCIE3A);
    	/* set OCR1A register value to 0x003e (corresponds to ~250hz) */
   		OCR3AH = 0x00;
   		OCR3AL = 0x3e;
		sei();
   		/* start the counter (16 000 000 / 1024) Hz */
  		TCCR3B |= (1 << CS32) | (1 << CS30);
		




}

// Print text to position x,y
// If z is 1, clear screen
void print_text(unsigned char x, unsigned char y, int z, char *ptr)
{
	if(z==1) lcd_write_ctrl(LCD_CLEAR);
	set_cursor_location(x,y);
	int i=0;
	
	while(ptr[i])
	{
		lcd_write_data(ptr[i]);
		i++;
	}
}

void menu(char* name, char* scores_text)
{
	/* Welcome texts */
	char str1[] = "Welcome to      ";
	char str2[] = "MasterMind      ";
	print_text(0,0,1,str1);
	print_text(0,1,0,str2);

	/* Press the middle button to continue */
	while(!BUTTON3_DOWN);

	char str3[] = "Highscore:";
	print_text(0,0,1,str3);

	set_cursor_location(0,1);
	/* Playername */
	lcd_write_data(name[0]);
	lcd_write_data(name[1]);
	lcd_write_data(name[2]);
	lcd_write_data(':');
	/* Scores */
	for(int i=0; i < 3; i++)
	{
		lcd_write_data(scores_text[i]);
	}

	//print_text(0,1,0,str4);
	_delay_ms(3000);
	introduction();
}


void introduction()
{
	int aika=0;
	int screen=0;

	/* Define screen texts */
	char text1[] = "Move cursor";
	char text2[] = "Change value";
	char text3[] = "At left side:";
	char text4[] = "Read history";
	char text5[] = "B3 accepts guess";
	char text6[] = "and starts game!";

	/* Start timer */
	TCCR1B |= (1 << CS12) | (1 << CS10);

	/* Sallitaan keskeytykset */
	SREG |= (1 << 7);

	/* Loop instructions */
	while(!BUTTON3_DOWN)
	{
		aika = seconds%6;

		if((aika == 0) && (screen!=1))
		{
			/* First screen */
			screen=1;
			lcd_write_ctrl(LCD_CLEAR);
			set_cursor_location(0,0);
			lcd_write_data(0x02);
			lcd_write_data(0x03);
			print_text(3,0,0,text1);
			set_cursor_location(0,1);
			lcd_write_data(0x00);
			lcd_write_data(0x01);
			print_text(3,1,0,text2);
			_delay_ms(2000);
			set_cursor_location(15,0);
			lcd_write_data('0'+aika);
		}


		if((aika == 2) && (screen!=2))
		{
			/* Second screen */
			screen=2;
			lcd_write_ctrl(LCD_CLEAR);
			set_cursor_location(0,0);
			print_text(0,0,0,text3);
			set_cursor_location(0,1);
			lcd_write_data(0x00);
			lcd_write_data(0x01);
			print_text(3,1,0,text4);
		}

		if((aika == 4) && (screen!=3))
		{
			/* Third screen */
			screen=3;
			print_text(0,0,1,text5);
			print_text(0,1,0,text6);
		}


	}
}


void getname(char *name)
{
	lcd_write_ctrl(LCD_CLEAR);
	char str[] = "Insert name";
	lcd_write_ctrl(LCD_ON | 0x01);
	print_text(0,0,1,str);
	_delay_ms(1000);

	int cursor_location=0;
	int name_length=2;
	print_text(0,1,0,name);

	// Let's accept the name with the middle button
	while (!BUTTON3_DOWN)
	{	
		/* Sets A-Z and 0-9 to name */
		if (BUTTON2_DOWN) if (cursor_location!=0) 
		{
			cursor_location--;
			_delay_ms(200);
		}
		if (BUTTON4_DOWN) if (cursor_location!=name_length) 
		{
			cursor_location++;
			_delay_ms(200);
		}
		if (BUTTON1_DOWN)
		{
			_delay_ms(200);
			if ((int)name[cursor_location]==57) name[cursor_location]=65; // From 0 --> A
			else if ((int)name[cursor_location]==90);
			else name[cursor_location]=(int)name[cursor_location]+1;
			lcd_write_data(name[cursor_location]);
		}
		if (BUTTON5_DOWN)
		{
			_delay_ms(200);
			if ((int)name[cursor_location]==65) name[cursor_location]=57; // From A --> 0
			else if ((int)name[cursor_location]==48);
			else name[cursor_location]=(int)name[cursor_location]-1;
			lcd_write_data(name[cursor_location]);
		}
		set_cursor_location(cursor_location,1);
	}
	_delay_ms(200);

	eeprom_write_block(name, (const void*)1, 4); 
}





// P‰‰ohjelma
int main(void)
{
	while(1)
	{
		// Alustetaan piirilevy
		init();

		// Load data from eeprom
  		uint8_t name[4]; 
    	eeprom_read_block((void*)&name, (const void*)1, 4);
		uint8_t scores_text[4]; 
    	eeprom_read_block((void*)&scores_text, (const void*)10, 4);

		// K‰ynnistet‰‰n valikko
		menu(name,scores_text);



		// Kysyt‰‰n nimi
		uint8_t newname[4] = "AAA";
		getname(newname);

		// M‰‰ritell‰‰n ratkaistavan koodin pituus
		int code_size=4;

		// Alustetaan muuttuja ratkaistavalle koodille
		char code[code_size+1];

		// Alustetaan historia array, joka sis‰lt‰‰ ratkaisuyritykset sek‰ oikeiden ja v‰‰rien merkkien m‰‰r‰n
		char history[100*(code_size+2)+1];

		// Generoidaan ratkaistava koodi pituudeltaan 4 merkki‰
		generatecode(code,code_size);


		// Aloitetaan arvailu
		insert_guess(code, code_size, history, scores_text);

		/* Start timer */
		TCCR1B |= (1 << CS12) | (1 << CS10);

		// Pys‰ytt‰‰ ja resettaa timerin
		//TCCR1B  = 0x00;
		//TCNT1 = 0x0000;

		_delay_ms(5000);
	}
}

void load_char(char *character)
{
		
		for(int i=0; i<8; i++)
		{
			lcd_write(character[i],1);
		}
		
}

int init()
{
		/* Alustetaan lcd-n‰yttˆ */
	   	lcd_init();
	   	lcd_write_ctrl(LCD_ON); 
	   	lcd_write_ctrl(LCD_CLEAR); /* tyhjent‰‰ n‰ytˆn */

		/* M‰‰ritell‰‰n kustomoidut merkit */
		char char1[8] = {0x00, 0x04, 0x0e, 0x1f, 0x04, 0x04, 0x04, 0x00};
		char char2[8] = {0x00, 0x04, 0x04, 0x04, 0x1f, 0x0e, 0x04, 0x00};
		char char3[8] = {0x00, 0x04, 0x0c, 0x1f, 0x0c, 0x04, 0x00, 0x00};
		char char4[8] = {0x00, 0x04, 0x06, 0x1f, 0x06, 0x04, 0x00, 0x00};
		char char5[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		char char6[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		char char7[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		char char8[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

		/* Ladataan kustomoidut merkit */
		lcd_write_ctrl(0x40);
		load_char(char1);
		load_char(char2);
		load_char(char3);
		load_char(char4);
		load_char(char5);
		load_char(char6);
		load_char(char7);
		load_char(char8);
		lcd_write_ctrl(0x80);

		/* Estet‰‰n keskeytykset */
		SREG &= ~(1 <<7);

		/* Initialixe timer */
		OCR1AH = 0x3d;  /* 15625 steps */
		OCR1AL = 0x09;  /* 1 sec */
		//OCR1AH = 0x3d;  /* 100 kertaa nopeampi sykli */
		//OCR1AL = 0x09; 
		TCCR1A &= ~( (1 << WGM11) | (1 << WGM10) ); // CTC mode
		TCCR1B  =    (1 << WGM12);
		TCCR1B &=   ~(1 << WGM13);



	    /* enable Output Compare A Match Interrupt */
	    TIMSK |= (1 << OCIE1A);

		return 0;
}

ISR(TIMER1_COMPA_vect)
{
	timer();
}

ISR(TIMER3_COMPA_vect)
{
	PORTE ^= (1 << PE4) | (1 << PE5);
}
