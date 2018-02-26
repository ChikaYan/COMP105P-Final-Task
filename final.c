#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "simpletools.h"
#include "abdrive.h"
#include "simpletools.h"
#include "ping.h"
#include "basicmove.h"


int counter = 0;

void right_turn_counter(){
    turn_right();
    counter ++;
}

void left_turn_counter(){
    turn_left();
    counter --;
}

int main() {
    drive_goto(150, 150); // initialize to first middle point
    turn_right();
}

