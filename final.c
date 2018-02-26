#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "simpletools.h"
#include "abdrive.h"
#include "simpletools.h"
#include "ping.h"
#include "basics.h"

const int SQUARE_LENGTH = 120;
const int LR_THRESHOLD = 30;
const int FRONT_THRESHOLD = 18;
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
int main() {

}