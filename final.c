#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "simpletools.h"
#include "abdrive.h"
#include "simpletools.h"
#include "ping.h"
#include "basics.h"

const int SQUARE_LENGTH = 124; // in ticks
const int LR_THRESHOLD = 45; // in LRdis()
const int FRONT_THRESHOLD = 25; // in cm

enum dir {
    up,
    down,
    left,
    right
};

enum label {
    N,
    X,
    OldJunction,
    Empty,
};

struct node {
    int x;
    int y;
    enum label tag;
    struct node *connected[20];
    int counter;
};


enum dir direction = up;
struct node *currentNode = NULL;
struct node *nodes[4][5];

int frontClear() {
    int fd = ping_cm(8);
    printf("Front distance is: %d\n", fd);
    if (fd >= FRONT_THRESHOLD) {
        return 1;
    }
    return 0;
}

int leftClear() {
    float ld = leftDis();
    printf("Left distance is: %f\n", ld);
    if (ld >= LR_THRESHOLD) {
        return 1;
    }
    return 0;
}

int rightClear() {
    float right_dis = rightDis();
    printf("Right distance is: %f\n", right_dis);
    if (right_dis >= LR_THRESHOLD) {
        return 1;
    }
    return 0;
}

void turnRight() {
    drive_goto(26, -25);
    switch (direction) {
        case up:
            direction = right;
            break;
        case right:
            direction = down;
            break;
        case down:
            direction = left;
            break;
        case left:
            direction = up;
    }
}

void turnLeft() {
    drive_goto(-26, 25);
    switch (direction) {
        case up:
            direction = left;
            break;
        case right:
            direction = up;
            break;
        case down:
            direction = right;
            break;
        case left:
            direction = down;
    }
}

void moveForward() {
    drive_goto(SQUARE_LENGTH, SQUARE_LENGTH);
    switch (direction) {
        case up:
            currentNode = nodes[currentNode->x, currentNode->y + 1];
            break;
        case right:
            currentNode = nodes[currentNode->x + 1, currentNode->y];
            break;
        case down:
            currentNode = nodes[currentNode->x, currentNode->y - 1];
            break;
        case left:
            currentNode = nodes[currentNode->x - 1, currentNode->y];
    }
}

int atJunction() {
    if (frontClear() + leftClear() + rightClear() >= 2) {
        return 1;
    }
    return 0;
}

void moveAlongPath() {
    if (frontClear()) {
        moveForward();
    } else if (leftClear()) {
        turnLeft();
        moveForward();
    } else {
        turnRight();
        moveForward();
    }
}

void initialiseNode() {
    for (int x = 0; x < 4; x++) {
        for (int y = 0; y < 5; y++) {
            nodes[x][y] = malloc(sizeof(struct node));
            nodes[x][y]->x = x;
            nodes[x][y]->y = y;
            nodes[x][y]->tag = Empty;
            nodes[x][y]->counter = 0;
            for (int i = 0; i < 20; i++) {
                nodes[x][y]->connected[i] = malloc(sizeof(struct node));
                nodes[x][y]->connected[i] = NULL;
            }
        }
    }
}

void addEdge(struct node *n1, struct node *n2) {
    n1->connected[n1->counter] = n2;
    n1->counter++;
    n2->connected[n2->counter] = n1;
    n2->counter++;
}

int main() { // Trémaux's Algorithm
    currentNode = malloc(sizeof(struct node));
    initialiseNode();
    currentNode = nodes[0, 0];
    drive_goto(30, 30); // initialize to first middle point
    while (1) {
        if (!atJunction()) {
            struct node *preNode = malloc(sizeof(struct node));
            preNode = currentNode;
            moveAlongPath();
            preNode->connected[preNode->counter] = currentNode;
            preNode->counter++;
//            currentNode->connected[currentNode->counter] = preNode;
//            currentNode->counter++;
            printf("preNode counter: %d\ncurrentNode counter: %d\n", preNode->counter, currentNode->counter);
            break;
        }


    }

}
// Tremaux's Algorithm
// 1) Enter new junction: (current_pos != X)
//      a) current_pos = X
//      b) new_passage = N
//      c) march to new_passage
//
// 2) Enter old junction (current_pos == X, previous_junction != current_pos)
//      a) current_passage = N
//      b) turn around and march back
//
// 3) Dead_end (sensor)
//      a) turn around and march back
//
// 4) Return old junction + unlabeled passage (current_pos == X, previous_junction == current_pos)
//      a) new_passage_2 = N
//      b) march to new_passage_2
//
//  5) Return old junction + no unlabeled passage (current_pos == X, previous_juntion == current_pos)
//      a) return to X


//int main() { // Pledge Algorithm
//    drive_goto(30, 30); // initialize to first middle point
//    while (1) {
//        if (counter < 0) {
//            if (rightClear()) {
//                turnRight();
//                moveForward();
//                continue;
//            }
//            if (frontClear()) {
//                moveForward();
//                continue;
//            }
//            turnLeft();
//
//        } else if (counter > 0) {
//            if (leftClear()) {
//                turnLeft();
//                moveForward();
//                continue;
//            }
//            if (frontClear()) {
//                moveForward();
//                continue;
//            }
//            turnRight();
//
//        } else {
//            if (frontClear()) {
//                moveForward();
//            } else {
//                turnRight();
//            }
//        }
//    }
//}


// Pledge algorithm
// 1) reach obstacle in front, start pledge algorithm
// 2) turn right
// 3) for every movement below:
//    (a) Obstacle: LEFT, No Obstacle: FRONT -----> move forward
//    (b) Obstacle: LEFT + FRONT -----------------> turn right
// 4) keep looping until counter reach 0
// 5) end Pledge algorithm

