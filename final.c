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
};


enum dir direction = up;
struct node *currentNode = NULL;
struct node *nodes[4][5];
int matrix[4][5][4][5];

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
            currentNode = *nodes[currentNode->x, currentNode->y + 1];
            break;
        case right:
            currentNode = *nodes[currentNode->x + 1, currentNode->y];
            break;
        case down:
            currentNode = *nodes[currentNode->x, currentNode->y - 1];
            break;
        case left:
            currentNode = *nodes[currentNode->x - 1, currentNode->y];
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
        }
    }
    for (int x1 = 0; x1 < 4; x1++) {
        for (int y1 = 0; y1 < 5; y1++) {
            for (int x2 = 0; x2 < 4; x2++) {
                for (int y2 = 0; y2 < 5; y2++) {
                    matrix[x1][y1][x2][y2] = 0;
                }
            }
        }
    }
    //printf("%d\n", nodes[0][0]->x);
}

void addEdge(struct node *n1, struct node *n2) {
    matrix[n1->x][n1->y][n2->x][n2->y] = 1;
    matrix[n2->x][n2->y][n1->x][n1->y] = 1;
}

void printMatrix() {
    printf("\n");
    printf("      ");
    for (int x = 0; x < 4; x++) {
        for (int y = 0; y < 5; y++) {
            printf(" (%d,%d)", x, y);
        }
    }
    printf("\n");
    for (int x1 = 0; x1 < 4; x1++) {
        for (int y1 = 0; y1 < 5; y1++) {
            char nodeString[6];
            nodeString[0] = '(';
            nodeString[1] = x1 + '0';
            nodeString[2] = ',';
            nodeString[3] = y1 + '0';
            nodeString[4] = ')';
            printf("%6s", nodeString);
            for (int x2 = 0; x2 < 4; x2++) {
                for (int y2 = 0; y2 < 5; y2++) {
                    printf("%6d", matrix[x1][y1][x2][y2]);
                }
            }
            printf("\n");
        }
    }
}

int main() { // Trémaux's Algorithm
    currentNode = malloc(sizeof(struct node));
    initialiseNode();
    currentNode = *nodes[0, 0];
    printf("%d\n", currentNode->x);
    currentNode -> x = 10;
    printf("%d\n", currentNode->x);
    printf("%d\n", (*nodes[0][0]).x);
    drive_goto(30, 30); // initialize to first middle point
    while (1) {
        if (!atJunction()) {
            struct node *preNode = malloc(sizeof(struct node));
            preNode = currentNode;

            moveAlongPath();

            printMatrix();
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

