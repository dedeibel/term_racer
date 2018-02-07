/**
 * term_racer
 *
 * @author 	Benjamin Peter <bennyp@users.berlios.de>
 * @date	Sun Nov  7 00:53:06 CET 2004
 * 
 * Cangelog:
 *
 * Fixed a bug causing bash not printing any characters anymore
 * Fixed some crash when an invalid map name was specified (thanks to donm)
 * Added term_editor as a track creator tool (thanks to donm for inspiration)
 *
 * @if copyright
 *
 * term_racer
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
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>

#define DEFAULT_FILE "default.map"

// timeout in micro sekonds
#define TIMEOUT 120000

/**
 * The main game loop.
 *
 * @param map The file where the map data is located.
 * @param startpos The position (in characters) where
 * 		  the car/ship/whatever should start.
 * @param size Trackwidth in characters.
 *
 * @return true if the goal was reached, else false.
 */
int
game(FILE* map, unsigned int startpos, unsigned int size);

/**
 * Sets the terminal attributes. (no icanon, no echo)
 */
void
set_term_attr(void);


/**
 * Unsets the terminal attributes. (no icanon, no echo)
 */
void
unset_term_attr(void);

int main(int argc, char** argv)
{
	FILE* map;
	unsigned int size = 0;
	int startpos = 0;
	int i;

	printf("Setting terminal attributes.\n\n");
	set_term_attr();

	if (argc != 2) {
		printf("No map specified, using default.map (%s <filename>)\n\n", argv[0]);
		map = fopen(DEFAULT_FILE, "r");
	}
	else {
		map = fopen(argv[1], "r");
	}

	if (map == NULL) {
		printf("Could not open map file. (%s <filename>)\n", argv[0]);
		unset_term_attr();
		exit(3);
	}

	if (fscanf(map, "(%u)(%u)", &size, &startpos) != 2) {
		printf("There was an error in the map file at line 1. (size)(startpos)\n");
		unset_term_attr();
		exit(3);
	}

	/* print header */
	printf("CONTROLS: 'j' for left, 'k' for right.\n"\
		   "(please make sure to have at least %d char width)\n", size);
	
	putchar('|');
	for (i = 1; i < (size); i++) { putchar('-');}
	putchar('|');
	putchar('\n');
	
	/* time to read */
	sleep(3);
	
	/* start the game */
	if (game(map, startpos, size)) {
		printf("################################# GOAL #############################\n\n");
		printf("Congratulations, you reached the Goal.\n");
	}
	else {
		printf("******************************** CRASH ****************************\n\n");
		printf("Sorry, but you left the road, please try again.\n");
	}
	
	unset_term_attr();
    return 0;
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

int
game(FILE* map, unsigned int startpos, unsigned int size) {
	char c;
    char line[size+1U];
	int xpos  = startpos;
	int result  = 0;
    unsigned int running = 1;
	unsigned int xmin = 1;
	unsigned int xmax = size - 1;
	unsigned int leftmargin  = xmin;
	unsigned int rightmargin = xmax;
	unsigned int nmbr = 1;

    fd_set inset;
    struct timeval timeout;

	/* initialize track */
	sprintf(line, "|%*c", size, '|');

    while(running) {
        /* wait for timeout, has to be set new everytime */
        timeout.tv_sec  = 0;
        timeout.tv_usec = TIMEOUT;
        
        /* what stream should be monitored */
        FD_ZERO(&inset);
        FD_SET(fileno(stdin), &inset);

		/* getting the track, line by line */
		result = fscanf(map, "%u %u", &leftmargin, &rightmargin);
		if (result == EOF) {
    		FD_CLR(fileno(stdin), &inset);
			return 1;
		}
		else if (result != 2) {
			printf("There was an error in the map file. Line: %d\n", nmbr);
			unset_term_attr();
    		FD_CLR(fileno(stdin), &inset);
			exit(1);
		}
		nmbr++;

		if ((leftmargin < xmin) || (leftmargin > xmax)) {
			printf("There was an error in the map file. Line: %d\n", nmbr);
			unset_term_attr();
    		FD_CLR(fileno(stdin), &inset);
			exit(1);
		}

		if ((rightmargin < xmin) || (rightmargin > xmax)) {
			printf("There was an error in the map file. Line: %d\n", nmbr);
			unset_term_attr();
    		FD_CLR(fileno(stdin), &inset);
			exit(1);
		}

        /* Wait TIMEOUT for new data */
        result = select(fileno(stdin)+1, &inset, NULL, NULL, &timeout);
        if (result == -1) {
            perror("select");
			unset_term_attr();
    		FD_CLR(fileno(stdin), &inset);
            exit(1);
        }
        
		/* isset is necessary, even if we know that there is input */
        if (result && FD_ISSET(fileno(stdin), &inset)) {
			c = getchar();

			if (c == 'j') {
				xpos--;
			}
			else if (c == 'k') {
				xpos++;
			}

            /* Picard on holo deck: "Computer, exit!" */
            if ((c == 'Q') || (c == EOF)) {
            	printf("Oh, and I shall quit, bye!\n");
				unset_term_attr();
    			FD_CLR(fileno(stdin), &inset);
				exit(0);
            }
        }
		
		if (running) {
			line[leftmargin] = '#';
			line[rightmargin] = '#';
				
			line[xpos] = 'V';

			printf("%s\n", line);

			line[xpos] = ' ';
			line[leftmargin] = ' ';
			line[rightmargin] = ' ';
			
			/* Stay on the track */
			if ((xpos <= leftmargin) || (xpos >= rightmargin)) {
				line[xpos] = 'X';
				printf("%s\n", line);

				return 0;
			}
		}
    }

	return 1;
}
