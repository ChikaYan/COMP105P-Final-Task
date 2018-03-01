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

const int SQUARE_LENGTH = 122; // in ticks
const int LR_THRESHOLD = 45; // in LRdis()
const int FRONT_THRESHOLD = 25; // in cm


int front_clear() {
    int front_dis = ping_cm(8);
    printf("Front distance is: %d\n", front_dis);
    if (front_dis >= FRONT_THRESHOLD) {
        return 1;
    }
    return 0;
}

int left_clear() {
    float left_dis = leftDis();
    printf("Left distance is: %f\n", left_dis);
    if (left_dis >= LR_THRESHOLD) {
        return 1;
    }
    return 0;
}

int right_clear() {
    float right_dis = rightDis();
    printf("Right distance is: %f\n", right_dis);
    if (right_dis >= LR_THRESHOLD) {
        return 1;
    }
    return 0;
}

void turn_right_counter() {
    turn_right();
    counter++;
}

void turn_left_counter() {
    turn_left();
    counter--;
}

void move_forward() {
    drive_goto(SQUARE_LENGTH, SQUARE_LENGTH);
}


int main() {
    drive_goto(30, 30); // initialize to first middle point
    while (1) {
        if (counter < 0) {
            if (right_clear()) {
                turn_right_counter();
                move_forward();
                continue;
            }
            if (front_clear()) {
                move_forward();
                continue;
            }
            turn_left_counter();

        } else if (counter > 0) {
            if (left_clear()) {
                turn_left_counter();
                move_forward();
                continue;
            }
            if (front_clear()) {
                move_forward();
                continue;
            }
            turn_right_counter();

        } else {
            if (front_clear()) {
                move_forward();
            } else {
                turn_right_counter();
            }
        }
    }
}


// Pledge algorithm
// 1) reach obstacle in front, start pledge algorithm
// 2) turn right
// 3) for every movement below:
//    (a) Obstacle: LEFT, No Obstacle: FRONT -----> move forward
//    (b) Obstacle: LEFT + FRONT -----------------> turn right
// 4) keep looping until counter reach 0
// 5) end Pledge algorithm