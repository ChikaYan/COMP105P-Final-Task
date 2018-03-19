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

int main() {
    int left, right;
    int preL, preR;
    drive_speed(128, 128);
    pause(1100);
    drive_getTicks(&left, &right);
    printf("Travelled for (%d,%d)\n", left - preL, right - preR);

    drive_getTicks(&preL, &preR);
    drive_speed(128, 50);
    pause(650);
    drive_getTicks(&left, &right);
    printf("Travelled for (%d,%d)\n", left - preL, right - preR);

    drive_speed(128, 128);
    while (1);
}
