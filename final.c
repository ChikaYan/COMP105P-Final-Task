#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "simpletools.h"
#include "abdrive.h"
#include "simpletools.h"
#include "ping.h"
#include "basics.h"

const int SQUARE_LENGTH = 122; // in ticks (40cm?)
const int LR_THRESHOLD = 39.5; // in LRdis()
const int FRONT_THRESHOLD = 30; // in cm

enum absoluteDir {
    up,
    down,
    left,
    right
};

enum relativeDir {
    front,
    back,
    rLeft,
    rRight
};

enum marchingDir {
    forward,
    backward
};

enum label {
    N,
    X,
    Empty,
};

struct node {
    int x;
    int y;
    enum label tag;
};

struct node *findAdjacent(enum relativeDir targetDirection);

void addEdge(struct node *n1, struct node *n2);

enum absoluteDir direction = up;
enum marchingDir marchingState = forward;
int turnLog = 0;
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
    turnLog += 1;
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
    drive_goto(-25, 26);
    turnLog -= 1;
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

void turnAround() {
    if (turnLog > 0) {
        turnLeft();
        turnLeft();
    } else if (turnLog < 0) {
        turnRight();
        turnRight();
    } else {
        drive_goto(51, -51); // TODO: find more accurate numbers
        switch (direction) {
            case up:
                direction = down;
                break;
            case right:
                direction = left;
                break;
            case down:
                direction = up;
                break;
            case left:
                direction = right;
        }
    }
}

void turnReset() {
    while (turnLog >= 3) {
        turnLeft();
        turnLeft();
        turnLeft();
        turnLeft();
    }
    while (turnLog <= -3) {
        turnRight();
        turnRight();
        turnRight();
        turnRight();
    }

//    if (turnLog == 2){
//        turnLeft();
//        turnLeft();
//        drive_goto(51, -51);
//    }else if (turnLog == -2){
//        turnRight();
//        turnRight();
//        drive_goto(51, -51);
//    }
}

void moveForward() {
    drive_goto(SQUARE_LENGTH, SQUARE_LENGTH);
    switch (direction) {
        case up:
            currentNode = nodes[currentNode->x][currentNode->y + 1];
            break;
        case right:
            currentNode = nodes[currentNode->x + 1][currentNode->y];
            break;
        case down:
            currentNode = nodes[currentNode->x][currentNode->y - 1];
            break;
        case left:
            currentNode = nodes[currentNode->x - 1][currentNode->y];
    }
    addEdge(findAdjacent(back), currentNode);
}

int atJunction() {
    if (frontClear() + leftClear() + rightClear() >= 2) {
        printf("Front clear: %d\n", frontClear());
        printf("Left clear: %d\n", leftClear());
        printf("Right clear: %d\n", rightClear());
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

struct node *findAdjacent(enum relativeDir targetDirection) {
    switch (targetDirection) {
        case front:
            switch (direction) {
                case up:
                    return nodes[currentNode->x][currentNode->y + 1];
                case right:
                    return nodes[currentNode->x + 1][currentNode->y];
                case down:
                    return nodes[currentNode->x][currentNode->y - 1];
                case left:
                    return nodes[currentNode->x - 1][currentNode->y];
            }
        case rRight:
            switch (direction) {
                case up:
                    return nodes[currentNode->x + 1][currentNode->y];
                case right:
                    return nodes[currentNode->x][currentNode->y - 1];
                case down:
                    return nodes[currentNode->x - 1][currentNode->y];
                case left:
                    return nodes[currentNode->x][currentNode->y + 1];
            }
        case back:
            switch (direction) {
                case up:
                    return nodes[currentNode->x][currentNode->y - 1];
                case right:
                    return nodes[currentNode->x - 1][currentNode->y];
                case down:
                    return nodes[currentNode->x][currentNode->y + 1];
                case left:
                    return nodes[currentNode->x + 1][currentNode->y];
            }
        case rLeft:
            switch (direction) {
                case up:
                    return nodes[currentNode->x - 1][currentNode->y];
                case right:
                    return nodes[currentNode->x][currentNode->y + 1];
                case down:
                    return nodes[currentNode->x + 1][currentNode->y];
                case left:
                    return nodes[currentNode->x][currentNode->y - 1];
            }
    }
}

void goToNodeWithTag(enum label tag) {
    if (leftClear() && findAdjacent(rLeft)->tag == tag) {
        turnLeft();
        moveForward();
        return;
    }
    if (frontClear() && findAdjacent(front)->tag == tag) {
        moveForward();
        return;
    }
    if (rightClear() && findAdjacent(rRight)->tag == tag) {
        turnRight();
        moveForward();
    }
}

int hasEmptyAdjNode() {
    if ((leftClear() && findAdjacent(rLeft)->tag == Empty) || (frontClear() && findAdjacent(front)->tag == Empty) ||
        (rightClear() && findAdjacent(rRight)->tag == Empty)) {
        return 1;
    }
    return 0;
}

int atOldJunction() {
    if ((leftClear() && findAdjacent(rLeft)->tag != Empty) || (frontClear() && findAdjacent(front)->tag != Empty) ||
        (rightClear() && findAdjacent(rRight)->tag != Empty)) {
        printf("atOldJunction pass\n");
        return 1;
    }
    return 0;
}

int main() { // Trémaux's Algorithm
    initialiseNode();
    //TODO: find out if malloc is needed
    currentNode = nodes[0][0];
    drive_goto(30, 30); // initialize to first middle point
    while (1) {
        turnReset();
        if (!atJunction()) {
            if (leftClear() + rightClear() + frontClear() > 0) { // not dead end
                moveAlongPath();
//              printf("Current node is (%d,%d)\n", currentNode->x, currentNode->y);
            } else {
                printf("At node (%d,%d): ", currentNode->x, currentNode->y);
                printf("3. Marching forward into a dead end.\n");
                turnAround();
                marchingState = backward;
                moveForward();
            }
            //printMatrix();
            continue;
        }
        // At junction:
        if (marchingState == forward) {
            if (!atOldJunction()) {
                printf("At node (%d,%d): ", currentNode->x, currentNode->y);
                printf("1. Marching forward into a new junction:\n");
                findAdjacent(back)->tag = X;
//                printf("find Adjacent(back) pass\n");
                goToNodeWithTag(Empty);
//                printf("goToNodeWithTag(Empty) pass\n");
            } else if (atOldJunction()) {
                printf("At node (%d,%d): ", currentNode->x, currentNode->y);
                printf("2. Marching forward into an old junction:\n");
                findAdjacent(back)->tag = N;
                turnAround();
                marchingState = backward;
                moveForward();
            }
        } else {
            // marching backward -- has to be old junction
            if (hasEmptyAdjNode()) {
                printf("At node (%d,%d): ", currentNode->x, currentNode->y);
                printf("4. Marching backward into a junction with some unlabeled passages\n");
                findAdjacent(back)->tag = N;
                marchingState = forward;
                goToNodeWithTag(Empty);
                currentNode->tag = N;
            } else {
                printf("At node (%d,%d): ", currentNode->x, currentNode->y);
                printf("5. Marching backward into a junction with no unlabeled passages\n");
                goToNodeWithTag(X);
            }
        }
        //printMatrix();
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

