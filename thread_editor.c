/**
 * thread_editor
 *
 * @author 	Benjamin Peter <bennyp@users.berlios.de>
 * @date	Wed Nov 10 16:53:07 CET 2004
 * 
 * Cangelog:
 *
 * @if copyright
 *
 * Copyright (C) 2004 Benjamin Peter
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * @endif
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <pthread.h>

#define DEFAULT_FILE "default.map"

#define BUFFLEN 4

// timeout in micro sekonds
#define TIMEOUT 180000

/**
 * The main game/editor loop.
 *
 * @param map The file where the map data is located.
 * @param startpos The position (in characters) where
 * 		  the car/ship/whatever should start.
 * @param size Trackwidth in characters.
 */
void
game(FILE* map, unsigned int startpos, unsigned int size);

/**
 * Sets the terminal attributes. (no icanon, no echo)
 */
void
set_term_attr(void);

/**
 * Fetch an unsigned int from stream
 *
 * @param where to fetch from
 * @return the entered value
 */
unsigned int
getInt(FILE* stream);

/**
 * Unsets the terminal attributes. (no icanon, no echo)
 */
void
unset_term_attr(void);

/**
 * Thread function to check for user input
 */
void*
get_user_input();

pthread_mutex_t m_values = PTHREAD_MUTEX_INITIALIZER;
unsigned int leftmargin;
unsigned int rightmargin;

unsigned int running = 1;
unsigned int xmin = 1;
unsigned int xmax = 1;

int main(int argc, char** argv)
{
	FILE* map;
	unsigned int size = 0;
	int startpos = 0;
	int i;

	if (argc != 2) {
		printf("No map name specified (%s <filename>)\n", argv[0]);
		exit(2);
	}
	else {
		map = fopen(argv[1], "w");
	}

	if (map == NULL) {
		printf("Could not open map file. (%s <filename>)\n", argv[0]);
		unset_term_attr();
		exit(3);
	}


	/* print header */
	printf("CONTROLS: 'j' for left, 'k' for right.\n"\
		   "          's'/'d' move left line left/right\n"\
		   "          'f'/'g' move right line left/right\n"\
		   "          'c'/'v' make track smaller/bigger\n"\
		   "          'Q' quit and save the map.\n");

	printf("Map width (20 - 80): ");
	size = getInt(stdin);

	/* max track width */
	if ((size < 20) || (size > 80)) {
		printf("Please specify a width within (20 - 80).\n");
		unset_term_attr();
		exit(6);
	}

	/* should be the best */
	startpos = size / 2;

	if (fprintf(map, "(%u)(%u)\n", size, startpos) < 6) {
		printf("There was an error writing the map file at line 1. (size)(startpos)\n");
		unset_term_attr();
		exit(3);
	}

	printf("Setting terminal attributes.\n\n");
	set_term_attr();

	printf("(please make sure to have at least %d char width)\n", size);
	putchar('|');
	for (i = 1; i < (size); i++) { putchar('-');}
	putchar('|');
	putchar('\n');
	
	/* time to read */
	sleep(3);
	
	game(map, startpos, size);
	
	fclose(map);
	unset_term_attr();
    return 0;
}

unsigned int
getInt(FILE* stream)
{
	unsigned int ret;
	char buffer[BUFFLEN];
	fgets((char*)&buffer, BUFFLEN, stream);
	
	if (sscanf((const char*)&buffer, "%u", &ret) != 1) {
		printf("You must specify a unsigned number.\n");
		unset_term_attr();
		exit(4);
	}

	return ret;
}

void
set_term_attr(void) {
	struct termios aktuell;
 	if(tcgetattr(STDIN_FILENO, &aktuell) < 0)
    {
    	printf("Couldn't get terminal attributes.\n");
    	exit(1);
    }

	aktuell.c_lflag &= ~(ICANON | ECHO);
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &aktuell) < 0)
    {
    	printf("Couldn't set terminal attributes.\n");
    	exit(1);
    }
}

void
unset_term_attr(void) {
	struct termios aktuell;
 	if(tcgetattr(STDIN_FILENO, &aktuell) < 0)
    {
    	printf("Couldn't get terminal attributes.\n");
    	exit(1);
    }

	aktuell.c_lflag |= (ICANON | ECHO);
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &aktuell) < 0)
    {
    	printf("Couldn't set terminal attributes.\n");
    	exit(1);
    }
}

void*
get_user_input()
{
	char c;
	while(running) {
		c = getchar();

		// move left
		if (c == 'j') {
			pthread_mutex_lock(&m_values);
			if (leftmargin > xmin) {
				leftmargin--;
				rightmargin--;
			}
			pthread_mutex_unlock(&m_values);
		}
		// move right
		else if (c == 'k') {
			pthread_mutex_lock(&m_values);
			if (rightmargin < xmax) {
				leftmargin++;
				rightmargin++;
			}
			pthread_mutex_unlock(&m_values);
		}
		// both smaller
		else if (c == 'c') {
			pthread_mutex_lock(&m_values);
			if ((leftmargin < (rightmargin + 3)) && (rightmargin > (leftmargin + 3))) {
				 leftmargin++;
				 rightmargin--;
			}
			pthread_mutex_unlock(&m_values);
		}
		// both bigger
		else if (c == 'v') {
			pthread_mutex_lock(&m_values);
			if ((leftmargin > xmin) && (rightmargin < xmax)) {
				leftmargin--;
				rightmargin++;
			}
			pthread_mutex_unlock(&m_values);
		}
		// left smaller
		else if (c == 's') {
			pthread_mutex_lock(&m_values);
			if (leftmargin > xmin) {
				leftmargin--;
			}
			pthread_mutex_unlock(&m_values);
		}
		// left bigger
		else if (c == 'd') {
			pthread_mutex_lock(&m_values);
			if (leftmargin < (rightmargin - 3)) {
				leftmargin++;
			}
			pthread_mutex_unlock(&m_values);
		}
		// right smaller
		else if (c == 'f') {
			pthread_mutex_lock(&m_values);
			if (rightmargin > (leftmargin + 3)) {
				rightmargin--;
			}
			pthread_mutex_unlock(&m_values);
		}
		// right bigger
		else if (c == 'g') {
			pthread_mutex_lock(&m_values);
			if (rightmargin < xmax) {
				rightmargin++;
			}
			pthread_mutex_unlock(&m_values);
		}
        /* Picard on holo deck: "Computer, exit!" */
        else if ((c == 'Q') || (c == EOF)) {
        	printf("Saved the map, bye.\n");
			unset_term_attr();
			pthread_mutex_lock(&m_values);
			rightmargin = 0;
			leftmargin = 0;
			pthread_mutex_unlock(&m_values);
			running = 0;
        }
	}
	return NULL;
}

void
game(FILE* map, unsigned int startpos, unsigned int size) {
    char line[size+1U];
	unsigned int nmbr = 2;
	pthread_t pt_input;

	xmin = 1;
	xmax = size - 1;
	leftmargin  = startpos - (size/3);
	rightmargin = startpos + (size/3);

	/* initialize track */
	sprintf(line, "|%*c", size, '|');

	/* starting input thread */
	if ((pt_input = pthread_create( &pt_input, NULL, &get_user_input, NULL))) {
		fprintf(stderr, "Thread creation failed, exiting.\n");
		running = 0;
		unset_term_attr();
		exit(1);
	}

    while(running) {
		pthread_mutex_lock(&m_values);
		/* writing the track, line by line */
		if (fprintf(map, "%u %u\n", leftmargin, rightmargin) < 4) {
			printf("There was an error writing the map file. Line: %d\n", nmbr);
			running = 0;
			unset_term_attr();
			exit(7);
		}
		nmbr++;
		
		if (running) {
			line[leftmargin] = '#';
			line[rightmargin] = '#';
				
			printf("%s\n", line);

			line[leftmargin] = ' ';
			line[rightmargin] = ' ';
		}
		pthread_mutex_unlock(&m_values);

        /* Wait TIMEOUT for new data */
		usleep(TIMEOUT);
    }
}
