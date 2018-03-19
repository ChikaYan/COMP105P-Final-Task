//
// Created by ChikamaYan on 3/19/2018.
//
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "simpletools.h"
#include "abdrive.h"
#include "simpletools.h"
#include "ping.h"
#include "basics.h"
#include "simulator.h"

int left, right;
int preL, preR;

void turnRightAfterForwarding(){
    drive_getTicks(&preL, &preR);
    drive_ramp(128, 64);
    pause(850);
    drive_getTicks(&left, &right);
    printf("Travelled for (%d,%d)\n", left - preL, right - preR);
}

void turnLeftAfterTurningRight(){
    drive_getTicks(&preL, &preR);
    drive_ramp(60, 128);
    pause(750);
    drive_getTicks(&left, &right);
    printf("Travelled for (%d,%d)\n", left - preL, right - preR);
}

int main() {
    drive_setRampStep(2000);
    simulator_startNewSmokeTrail();

    drive_ramp(128, 128);
    pause(1150);
    pause(1000);
    drive_getTicks(&left, &right);
    printf("Travelled for (%d,%d)\n", left - preL, right - preR);

    turnRightAfterForwarding();

//    turnLeftAfterTurningRight();


//    drive_getTicks(&preL, &preR);
//    drive_ramp(128, 0);
//    pause(750);
//    drive_getTicks(&left, &right);
//    printf("Travelled for (%d,%d)\n", left - preL, right - preR);
//    drive_ramp(128, 128);
//    pause(150);

    drive_ramp(128, 128);
    while (1);
}
