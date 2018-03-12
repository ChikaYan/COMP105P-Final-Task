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

const int SQUARE_LENGTH = 123; // in ticks (40cm?)
const int LR_THRESHOLD = 39.5; // in LRdis()
const int FRONT_THRESHOLD = 30; // in cm

void printDegree() {
    double pi = 3.1415926, x = 0, y = 0, degree = 0;
    simulator_getPose(&x, &y, &degree);
    printf("Current degree: %f\n", degree * 180.0 / pi);
}

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

struct queueMember {
    struct node *n;
    int counter;
    struct node *path[30];
};

struct node *findAdjacent(enum relativeDir targetDirection);

void addEdge(struct node *n1, struct node *n2);

enum absoluteDir currentDir = up;
enum marchingDir marchingState = forward;
int turnLog = 0;
struct node *currentNode = NULL;
struct node *nodes[4][5];
int matrix[4][5][4][5];
struct queueMember *queue[40];
int qFront = 1, qRear = 0;
int visited[4][5];

int frontClear() {
    int fd = ping_cm(8);
    printf("Front distance is: %d\n", fd);
    if (fd >= FRONT_THRESHOLD) {
        return 1;
    }
    int botReposition = (18 - fd);
    if (botReposition > 1 || botReposition < -1) {
        botReposition /= 0.325;
        drive_goto(-botReposition, -botReposition);
    }
    return 0;
}

int leftClear() {
    float ld = leftDis();
//    printf("Left distance is: %f\n", ld);
    if (ld >= LR_THRESHOLD) {
        return 1;
    }
    return 0;
}

int rightClear() {
    float right_dis = rightDis();
//    printf("Right distance is: %f\n", right_dis);
    if (right_dis >= LR_THRESHOLD) {
        return 1;
    }
    return 0;
}

void turnRight() {
    if (turnLog > 0) {
        // turn left first then turn around
        drive_goto(-25, 26);
        drive_goto(51, -52);
        turnLog -= 1;
    } else {
        drive_goto(26, -25);
        turnLog += 1;
    }

    switch (currentDir) {
        case up:
            currentDir = right;
            break;
        case right:
            currentDir = down;
            break;
        case down:
            currentDir = left;
            break;
        case left:
            currentDir = up;
    }
    printDegree();
}

void turnLeft() {
    if (turnLog < 0) {
        drive_goto(26, -25);
        drive_goto(51, -52);
        turnLog += 1;
    } else {
        drive_goto(-25, 26);
        turnLog -= 1;
    }

    switch (currentDir) {
        case up:
            currentDir = left;
            break;
        case right:
            currentDir = up;
            break;
        case down:
            currentDir = right;
            break;
        case left:
            currentDir = down;
    }
    printDegree();
}

void turnAround() {
    if (turnLog > 0) {
        turnLeft();
        turnLeft();
    } else if (turnLog < 0) {
        turnRight();
        turnRight();
    } else {
        drive_goto(51, -52);
        switch (currentDir) {
            case up:
                currentDir = down;
                break;
            case right:
                currentDir = left;
                break;
            case down:
                currentDir = up;
                break;
            case left:
                currentDir = right;
        }
    }
    printDegree();
}

void moveForward() {
    drive_goto(SQUARE_LENGTH, SQUARE_LENGTH);
    switch (currentDir) {
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
    printDegree();
}

int atJunction() {
    if (frontClear() + leftClear() + rightClear() >= 2) {
//        printf("Front clear: %d\n", frontClear());
//        printf("Left clear: %d\n", leftClear());
//        printf("Right clear: %d\n", rightClear());
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
            visited[4][5] = 0;
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
            switch (currentDir) {
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
            switch (currentDir) {
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
            switch (currentDir) {
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
            switch (currentDir) {
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
        return 1;
    }
    return 0;
}

struct queueMember *bfs(struct queueMember *current) {
    int x = current->n->x, y = current->n->y;\
    visited[x][y] = 1;
    qRear++;

    if (x == 3 && y == 4) { // final destination reached
        current->path[current->counter] = nodes[3][4];
        current->counter++;
        return current;
    }
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 5; j++) {
            if (matrix[x][y][i][j] && !visited[i][j]) { // current node connects to node (i,j)
                queue[qFront]->n = nodes[i][j];
                for (int k = 0; k < current->counter; k++) {
                    queue[qFront]->path[k] = current->path[k];
                }
                queue[qFront]->path[current->counter] = nodes[x][y];
                queue[qFront]->counter = current->counter + 1;
                qFront++;
            }
        }
    }

    return bfs(queue[qRear]);

}

struct queueMember *findPath() {
    for (int i = 0; i < 40; i++) {
        queue[i] = malloc(sizeof(struct queueMember));
        queue[i]->counter = 0;
        queue[i]->n = malloc(sizeof(struct node));
        for (int j = 0; j < 30; j++) {
            queue[i]->path[j] = malloc(sizeof(struct node));
            queue[i]->path[j] = NULL;
        }
    }

    queue[0]->n = nodes[0][0];
    struct queueMember *p = malloc(sizeof(struct queueMember));
    p = bfs(queue[qRear]);
    printf("\nPath is: \n");
    for (int i = 0; i < p->counter; i++) {
        printf("(%d, %d)  ", p->path[i]->x, p->path[i]->y);
    }
    printf("\n");
    return p;
}

void turnToAbsolute(enum absoluteDir dir) {
    switch (dir) {
        case up:
            switch (currentDir) {
                case up:
                    break;
                case right:
                    turnLeft();
                    break;
                case down:
                    turnAround();
                    break;
                case left:
                    turnRight();
            }
            break;
        case right:
            switch (currentDir) {
                case up:
                    printf("turn to absolute right\n");
                    turnRight();
                    break;
                case right:
                    break;
                case down:
                    turnLeft();
                    break;
                case left:
                    turnAround();
            }
            break;
        case down:
            switch (currentDir) {
                case up:
                    turnAround();
                    break;
                case right:
                    turnRight();
                    break;
                case down:
                    break;
                case left:
                    turnRight();
            }
            break;
        case left:
            switch (currentDir) {
                case up:
                    turnLeft();
                    break;
                case right:
                    turnAround();
                    break;
                case down:
                    turnRight();
                    break;
            }
    }
}


void goToNode(struct node *n) {
    if (n->x == findAdjacent(front)->x && n->y == findAdjacent(front)->y) {
        moveForward();
    } else if (n->x == findAdjacent(rRight)->x && n->y == findAdjacent(rRight)->y) {
        turnRight();
        moveForward();
    } else if (n->x == findAdjacent(back)->x && n->y == findAdjacent(back)->y) {
        turnAround(); // usually won't happen
        moveForward();
    } else if (n->x == findAdjacent(rLeft)->x && n->y == findAdjacent(rLeft)->y) {
        turnLeft();
        moveForward();
    } else {
        printf("\nERROR\n");
    }

}

int main() { // Trémaux's Algorithm
    initialiseNode();
    simulator_startNewSmokeTrail();
    //TODO: find out if malloc is needed
    currentNode = malloc(sizeof(struct node));
    currentNode = nodes[0][0];
    drive_goto(30, 30); // initialize to first middle point
    while (1) {
        if (!atJunction()) {
            printf("At node (%d,%d): \n", currentNode->x, currentNode->y);
            if (leftClear() + rightClear() + frontClear() > 0) { // not dead end
                moveAlongPath();
//              printf("Current node is (%d,%d)\n", currentNode->x, currentNode->y);
            } else {
                if (currentNode == nodes[0][0]) {
                    printf("Traversed the maze\n");
                    break;
                }
                printf("At node (%d,%d): \n", currentNode->x, currentNode->y);
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
//               printf("find Adjacent(back) pass\n");
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
    }

    printMatrix();
    struct queueMember *p = malloc(sizeof(struct queueMember));
    p = findPath();

    currentNode = nodes[0][0];
    turnAround();
    printf("\n**********HEADING BACK**********\n");
    int i = 1;
    while (i < p->counter) {
        int changeX = p->path[i]->x - currentNode->x;
        int changeY = p->path[i]->y - currentNode->y;
        int step = 1;
        if (changeX != 0 && changeY != 0) {
            printf("\nERROR: changX and changY both != 0\n");
        }
        if (i + 1 < p->counter) {
            while (changeX != 0 && changeY == 0 && p->path[i + 1]->x - p->path[i]->x == changeX) {
                step++;
                i++;
                printf("Move in x direction for step %d\n", step);
            }
            while (changeY != 0 && changeX == 0 && p->path[i + 1]->y - p->path[i]->y == changeY) {
                step++;
                i++;
                printf("Move in y direction for step %d\n", step);
            }
        }
        if (changeX > 0) {
            printf("Facing right\n");
            turnToAbsolute(right);
        } else if (changeX < 0) {
            printf("Facing left\n");
            turnToAbsolute(left);
        } else if (changeY > 0) {
            printf("Facing up\n");
            turnToAbsolute(up);
        } else if (changeY < 0) {
            printf("Facing down\n");
            turnToAbsolute(down);
        }
        printf("Moving forward for step: %d\n", step);
        drive_goto(SQUARE_LENGTH * step, SQUARE_LENGTH * step);
        currentNode = p->path[i];
        printf("At node (%d,%d)\n", currentNode->x, currentNode->y);
        i++;
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

