#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "simpletools.h"
#include "abdrive.h"
#include "simpletools.h"
#include "ping.h"
#include "basics.h"

int counter = 0;

const int SQUARE_LENGTH = 120;
const int LR_THRESHOLD = 30;
const int FRONT_THRESHOLD = 18;


void right_turn_counter() {
    turn_right();
    counter++;
}

void left_turn_counter() {
    turn_left();
    counter--;
}

int front_has_obstacle() {
    if (ping_cm(8) >= FRONT_THRESHOLD) {
        return 0;
    }
    return 1;
}

int left_has_obstacle() {
    if (leftDis() >= LR_THRESHOLD) {
        return 0;
    }
    return 1;
}

int right_has_obstacle() {
    if (rightDis() >= LR_THRESHOLD) {
        return 0;
    }
    return 1;
}

void move_forward() {
    drive_goto(SQUARE_LENGTH, SQUARE_LENGTH);
}


int main() {
    drive_goto(30, 30); // initialize to first middle point
    while (1) {
        if (counter < 0) {
            if (right_has_obstacle()) {
                if (front_has_obstacle()) {
                    left_turn_counter();
                } else {
                    move_forward();
                }
            } else {
                right_turn_counter();
            }
        } else if (counter > 0) {
            if (left_has_obstacle()) {
                if (front_has_obstacle()) {
                    right_turn_counter();
                } else {
                    move_forward();
                }
            } else {
                left_turn_counter();
            }
        } else {
            if (front_has_obstacle() == 0) {
                move_forward();
            } else {
                right_turn_counter();
            }
        }


    }
}
