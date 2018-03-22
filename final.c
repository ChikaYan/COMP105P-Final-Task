// TODO: read your own code and make sure everything works as expected!

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
const int LR_THRESHOLD = 79; // in LRdis()
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

enum moveType {
    init,
    rightTurn,
    leftTurn,
    turn180,
    forwarding
};

struct node {
    int x;
    int y;
    int junction;
    enum label upTag;
    enum label rightTag;
    enum label leftTag;
    enum label downTag;
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
struct queueMember *queue[1000];
int qFront = 1, qRear = 0;
int visited[4][5];
struct queueMember *paths[50];
int pathCounter = 0;

int frontClear() {
    int fd = ping_cm(8);
    //printf("Front distance is: %d\n", fd);
    if (fd >= FRONT_THRESHOLD) {
        return 1;
    }
//    int botReposition = (18 - fd);
//    if (botReposition > 0 || botReposition < 0) { // TODO: needed?
//        botReposition /= 0.325;
//        drive_goto(-botReposition, -botReposition);
//    }
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
//    if (turnLog > 0) {
//        // turn left first then turn around
//        drive_goto(-25, 26);
//        drive_goto(51, -52);
//        turnLog -= 1;
//    } else {
//        drive_goto(26, -25);
//        turnLog += 1;
//    }
    if (turnLog > 0 && turnLog % 2 == 1) { // right turn number is larger than 0 and is odd -- need to overturn
        drive_goto(26, -26);
        printf("\nRight overturning\n");
    } else if (turnLog < 0 &&
               turnLog % 2 == 0) { // left turn number is larger than 0 and is even -- need to reset overturn
        drive_goto(26, -26);
        printf("\nRight overturning\n");
    } else {
        drive_goto(25, -26);
    }
    turnLog += 1;

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
//    if (marchingState == backward) {
//        frontClear();
//    }
}

void turnLeft() {
//    if (turnLog < 0) {
//        drive_goto(26, -25);
//        drive_goto(51, -52);
//        turnLog += 1;
//    } else {
//        drive_goto(-25, 26);
//        turnLog -= 1;
//    }
    if (turnLog < 0 && turnLog % 2 == -1) { // left turn number is larger than 0 and is odd -- need to overturn
        drive_goto(-26, 26);
        printf("\nLeft overturning\n");
    } else if (turnLog > 0 && turnLog % 2 == 0) { // has turned more than one right turns -- need to reset overturn
        drive_goto(-26, 26);
        printf("\nLeft overturning\n");
    } else {
        drive_goto(-25, 26);
    }
    turnLog -= 1;

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
//    if (marchingState == backward) {
//        frontClear();
//    }
}

void turnAround() { // shouldn't be used
    printf("WARNING: Turn Around function is used\n");
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
}

void moveBackward() {
    drive_goto(-SQUARE_LENGTH, -SQUARE_LENGTH);
    switch (currentDir) {
        case up:
            currentNode = nodes[currentNode->x][currentNode->y - 1];
            break;
        case right:
            currentNode = nodes[currentNode->x - 1][currentNode->y];
            break;
        case down:
            currentNode = nodes[currentNode->x][currentNode->y + 1];
            break;
        case left:
            currentNode = nodes[currentNode->x + 1][currentNode->y];
    }
    addEdge(findAdjacent(front), currentNode);
}

void dsTurnRightAfterInit() { // tested: after init
    drive_ramp(128, 64);
    pause(710);
    drive_ramp(128, 128);
    pause(170);
}

void dsTurnRight() { // tested: after forwarding, after turning left
    drive_ramp(128, 64);
    pause(785);
    drive_ramp(128, 128);
    pause(170);
}

void dsTurnRightAfterRightTurn() { // tested: after right turn
    drive_ramp(128, 128);
    pause(100);
    drive_ramp(128, 64);
    pause(628);
    drive_ramp(128, 128);
    pause(170);
}

void dsTurnLeft() { // tested: after forwarding, after turning right, after turning init right
    drive_ramp(64, 128);
    pause(785);
    drive_ramp(128, 128);
    pause(170);
}

void dsTurnLeftAfterLeftTurn() { // tested: after left turn
    drive_ramp(128, 128);
    pause(100);
    drive_ramp(64, 128);
    pause(628);
    drive_ramp(128, 128);
    pause(170);
}

void
dsMoveForward() { // should go less than 122 ticks in order to turn. Should be followed by moveForwardAfterForwarding or turning
    drive_ramp(128, 128);
    pause(825);
}

void dsMoveForwardAfterForwarding() { // tested: after moveForward
    drive_ramp(128, 128);
    pause(940);
}

void dsMoveForwardAfter90Turn() { // tested: after left turn and right turn
    drive_ramp(128, 128);
    pause(900);
}

void dsMoveForwardAfter180Turn() { // tested: after 180 turns
    drive_ramp(128, 128);
    pause(1025);
}

void dsInitMove() { // tested: init
    drive_ramp(128, 128);
    pause(1150);
}

int atJunction() {
    if (frontClear() + leftClear() + rightClear() >= 2) {
        return 1;
    }
    return 0;
}

void moveAlongPath() {
    if (marchingState == forward) {
        if (frontClear()) {
            moveForward();
        } else if (leftClear()) {
            turnLeft();
            moveForward();
        } else {
            turnRight();
            moveForward();
        }
    } else {
        if (leftClear()) {
            turnRight();
            moveBackward();
        } else if (rightClear()) {
            turnLeft();
            moveBackward();
        } else { // marching backwards -- can't be dead end
            moveBackward();
        }
    }

}

void initialiseNode() {
    for (int x = 0; x < 4; x++) {
        for (int y = 0; y < 5; y++) {
            nodes[x][y] = malloc(sizeof(struct node));
            nodes[x][y]->x = x;
            nodes[x][y]->y = y;
            nodes[x][y]->upTag = Empty;
            nodes[x][y]->rightTag = Empty;
            nodes[x][y]->leftTag = Empty;
            nodes[x][y]->downTag = Empty;
            nodes[x][y]->junction = 0;
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

struct node *findAdjacent(enum relativeDir targetDirection) { // unchecked
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

int hasAdjacent(enum relativeDir targetDirection) {// unchecked
    switch (targetDirection) {
        case front:
            switch (currentDir) {
                case up:
                    return currentNode->y + 1 <= 4;
                case right:
                    return currentNode->x + 1 <= 3;
                case down:
                    return currentNode->y - 1 >= 0;
                case left:
                    return currentNode->x - 1 >= 0;
            }
        case rRight:
            switch (currentDir) {
                case up:
                    return currentNode->x + 1 <= 3;
                case right:
                    return currentNode->y - 1 >= 0;
                case down:
                    return currentNode->x - 1 >= 0;
                case left:
                    return currentNode->y + 1 <= 4;
            }
        case back:
            switch (currentDir) {
                case up:
                    return currentNode->y - 1 >= 0;
                case right:
                    return currentNode->x - 1 >= 0;
                case down:
                    return currentNode->y + 1 <= 4;
                case left:
                    return currentNode->x + 1 <= 3;
            }
        case rLeft:
            switch (currentDir) {
                case up:
                    return currentNode->x - 1 >= 0;
                case right:
                    return currentNode->y + 1 <= 4;
                case down:
                    return currentNode->x + 1 <= 3;
                case left:
                    return currentNode->y - 1 >= 0;
            }
    }
}

enum label findAdjacentTag(enum relativeDir targetDirection) {// unchecked
    switch (targetDirection) {
        case front:
            switch (currentDir) {
                case up:
                    return currentNode->upTag;
                case right:
                    return currentNode->rightTag;
                case down:
                    return currentNode->downTag;
                case left:
                    return currentNode->leftTag;
            }
        case rRight:
            switch (currentDir) {
                case up:
                    return currentNode->rightTag;
                case right:
                    return currentNode->downTag;
                case down:
                    return currentNode->leftTag;
                case left:
                    return currentNode->upTag;
            }
        case back:
            switch (currentDir) {
                case up:
                    return currentNode->downTag;
                case right:
                    return currentNode->leftTag;
                case down:
                    return currentNode->upTag;
                case left:
                    return currentNode->rightTag;
            }
        case rLeft:
            switch (currentDir) {
                case up:
                    return currentNode->leftTag;
                case right:
                    return currentNode->upTag;
                case down:
                    return currentNode->rightTag;
                case left:
                    return currentNode->downTag;
            }
    }
}

void updateAdjacentTag(enum relativeDir targetDirection, enum label newTag) {// unchecked
    switch (targetDirection) {
        case front:
            switch (currentDir) {
                case up:
                    currentNode->upTag = newTag;
                    return;
                case right:
                    currentNode->rightTag = newTag;
                    return;
                case down:
                    currentNode->downTag = newTag;
                    return;
                case left:
                    currentNode->leftTag = newTag;
                    return;
            }
        case rRight:
            switch (currentDir) {
                case up:
                    currentNode->rightTag = newTag;
                    return;
                case right:
                    currentNode->downTag = newTag;
                    return;
                case down:
                    currentNode->leftTag = newTag;
                    return;
                case left:
                    currentNode->upTag = newTag;
                    return;
            }
        case back:
            switch (currentDir) {
                case up:
                    currentNode->downTag = newTag;
                    return;
                case right:
                    currentNode->leftTag = newTag;
                    return;
                case down:
                    currentNode->upTag = newTag;
                    return;
                case left:
                    currentNode->rightTag = newTag;
                    return;
            }
        case rLeft:
            switch (currentDir) {
                case up:
                    currentNode->leftTag = newTag;
                    return;
                case right:
                    currentNode->upTag = newTag;
                    return;
                case down:
                    currentNode->rightTag = newTag;
                    return;
                case left:
                    currentNode->downTag = newTag;
                    return;
            }
    }
}

void printTags() {
    printf("Node (%d,%d)\n", currentNode->x, currentNode->y);
    printf("upTag: ");
    switch (currentNode->upTag) {
        case X:
            printf("X\n");
            break;
        case N:
            printf("N\n");
            break;
        case Empty:
            printf("Empty\n");
            break;
    }
    printf("leftTag: ");
    switch (currentNode->leftTag) {
        case X:
            printf("X\n");
            break;
        case N:
            printf("N\n");
            break;
        case Empty:
            printf("Empty\n");
            break;
    }
    printf("rightTag: ");
    switch (currentNode->rightTag) {
        case X:
            printf("X\n");
            break;
        case N:
            printf("N\n");
            break;
        case Empty:
            printf("Empty\n");
            break;
    }
    printf("downTag: ");
    switch (currentNode->downTag) {
        case X:
            printf("X\n");
            break;
        case N:
            printf("N\n");
            break;
        case Empty:
            printf("Empty\n");
            break;
    }
}

void faceExitWithBack() { // unchecked: change clear() to hasAdjacent -- is hasAdjacent even needed?
    printTags();
    if (hasAdjacent(rLeft) && findAdjacentTag(rLeft) == X) {
        turnRight();
        return;
    }
    if (hasAdjacent(rRight) && findAdjacentTag(rRight) == X) {
        turnLeft();
        return;
    }
    if (hasAdjacent(front) && findAdjacentTag(front) == X) { // shouldn't be used
        turnAround();
        printf("WARNING: faceExitWithBack function turns the bot around\n");
    }

    // otherwise the exit is just at back -- no need to do anything
}

void goToEmpty() { // bot should always move forward into empty node
    if (leftClear() && findAdjacentTag(rLeft) == Empty) {
        turnLeft();
        updateAdjacentTag(front, N);
        moveForward();
        return;
    }
    if (frontClear() && findAdjacentTag(front) == Empty) {
        updateAdjacentTag(front, N);
        moveForward();
        return;
    }
    if (rightClear() && findAdjacentTag(rRight) == Empty) {
        turnRight();
        updateAdjacentTag(front, N);
        moveForward();
    }
}

int hasEmptyAdjNode() { // will only be used when marching back into a junction
    if ((leftClear() && findAdjacentTag(rLeft) == Empty) || (frontClear() && findAdjacentTag(front) == Empty) ||
        (rightClear() && findAdjacentTag(rRight) == Empty)) {
        return 1;
    }
    return 0;
}

int atOldJunction() {
    if ((leftClear() && findAdjacentTag(rLeft) != Empty) || (frontClear() && findAdjacentTag(front) != Empty) ||
        (rightClear() && findAdjacentTag(rRight) != Empty)) {
        return 1;
    }
    return 0;
}

void bfs() { // TODO: check change to while loop, change to use loop queue?
    while (qRear < qFront) {
        struct queueMember *current = queue[qRear];
        int x = current->n->x, y = current->n->y;
        qRear++; // now qRead is pointing towards the next node in path

        if (x == 3 && y == 4) { // final destination reached
            current->path[current->counter] = nodes[3][4];
            current->counter++;
            paths[pathCounter] = current;
            pathCounter++;
            // the queueMember that reaches the final destination is needed in paths -- don't free it
            if (pathCounter >= 50) {
                printf("WARNING: paths array is full, quitting bfs\n");
                return;
            }
        } else {
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 5; j++) {
                    if (matrix[x][y][i][j]) { // current node connects to node (i,j)
                        // check if target node is visited already
                        int nodeVisited = 0;
                        for (int k = 0; k < current->counter; k++) {
                            if (current->path[k] == nodes[i][j]) {
                                //printf("Found a visited node\n");
                                nodeVisited = 1;
                                break;
                            }
                        }
                        if (nodeVisited) {
                            continue;
                        }
                        // dynamic queue init
                        queue[qFront] = malloc(sizeof(struct queueMember));
//                        queue[qFront]->n = malloc(sizeof(struct node));
//                        for (int k = 0; k < 30; k++) {
//                            queue[qFront]->path[k] = malloc(sizeof(struct node));
//                            queue[qFront]->path[k] = NULL;
//                        }

                        queue[qFront]->n = nodes[i][j];
                        for (int k = 0; k < current->counter; k++) {
                            queue[qFront]->path[k] = current->path[k]; // inherit the path from its precedence
                        }
                        queue[qFront]->path[current->counter] = nodes[x][y];
                        queue[qFront]->counter = current->counter + 1;

                        // free the last queue member -- can't use?
                        //free(queue[qRear - 1]);

                        qFront++; // qFront now points to next avaliable index
                        if (qFront >= 1000) {
                            printf("WARNING: queue is full, quiting bfs\n");
                            return;
                        }
                    }
                }
            }
        }

//        if (qRear >= qFront) { // queue empty -- qFront always points to the first empty space after qRear
//            return;
//        }
    }
    //bfs(queue[qRear]);
}

struct queueMember *findBestPath() { // unchecked
    int bestPathIndex = 0;
    float bestTime = 99999;
    for (int i = 0; i < pathCounter; i++) {
        struct queueMember *p = paths[i];
        float time = 0;
        int j = 0;
        struct node *simulateNode = nodes[0][0];
        while (j < p->counter) {

            int changeX = p->path[j]->x - simulateNode->x;
            int changeY = p->path[j]->y - simulateNode->y;
            int step = 1;
            if (changeX != 0 && changeY != 0) {
                printf("\nERROR: changX and changY both != 0\nchangeX: %d\nchangeY: %d\n", changeX, changeY);
            }
            if (j + 1 < p->counter) {
                while (changeX != 0 && changeY == 0 && p->path[j + 1]->x - p->path[j]->x == changeX) {
                    step++;
                    j++;
                    if (j + 1 >= p->counter) {
                        break;
                    }
                }
                while (changeY != 0 && changeX == 0 && p->path[j + 1]->y - p->path[j]->y == changeY) {
                    step++;
                    j++;
                    if (j + 1 >= p->counter) {
                        break;
                    }
                }
            }
            if (changeX > 0) {
                if (currentDir != right) {
                    time += 3.264;
                }
            } else if (changeX < 0) {
                if (currentDir != left) {
                    time += 3.264;
                }
            } else if (changeY > 0) {
                if (currentDir != up) {
                    time += 3.264;
                }
            } else if (changeY < 0) {
                if (currentDir != down) {
                    time += 3.264;
                }
            }
            switch (step) {
                case 1:
                    time += 7.622;
                    break;
                case 2:
                    time += 7.674;
                    break;
                case 3:
                    time += 9.582;
                    break;
                case 4:
                    time += 11.312;
            }
            simulateNode = p->path[j];
            j++;
        }
        printf("\nPath %d is:", i);
        for (int k = 0; k < p->counter; k++) {
            printf("(%d, %d)  ", p->path[k]->x, p->path[k]->y);
        }
        printf("\ntime needed is: %f\n", time);
        if (time < bestTime) {
            bestTime = time;
            bestPathIndex = i;
        }
    }
    return paths[bestPathIndex];
}

struct queueMember *findPath() {// unchecked
    // initialise the queue
//    for (int i = 0; i < 1000; i++) {
//        queue[i] = malloc(sizeof(struct queueMember));
//        queue[i]->counter = 0;
//        queue[i]->n = malloc(sizeof(struct node));
//        for (int j = 0; j < 30; j++) {
//            queue[i]->path[j] = malloc(sizeof(struct node));
//            queue[i]->path[j] = NULL;
//        }
//    }

    // initialise paths -- not needed
//    for (int i = 0; i < 50; i++) {
//        paths[i] = malloc(sizeof(struct queueMember));
//        paths[i]->counter = 0;
//        paths[i]->n = malloc(sizeof(struct node));
//        for (int j = 0; j < 30; j++) {
//            paths[i]->path[j] = malloc(sizeof(struct node));
//            paths[i]->path[j] = NULL;
//        }
//    }

    queue[0] = malloc(sizeof(struct queueMember));
    queue[0]->counter = 0;
//    queue[0]->n = malloc(sizeof(struct node));
//    for (int j = 0; j < 30; j++) {
//        queue[0]->path[j] = malloc(sizeof(struct node));
//        queue[0]->path[j] = NULL;
//    }
    queue[0]->n = nodes[0][0];
    bfs();

    return findBestPath();
}

void turnToAbsolute(enum absoluteDir dir) {// unchecked
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
                    turnLeft();
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

void dgRacing() {// unchecked
    struct queueMember *p = findPath();
    currentNode = nodes[0][0];
    printf("\n**********DRIVE_GOTO HEADING BACK**********\n");
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
                if (i + 1 >= p->counter) {
                    break;
                }
            }
            while (changeY != 0 && changeX == 0 && p->path[i + 1]->y - p->path[i]->y == changeY) {
                step++;
                i++;
                printf("Move in y direction for step %d\n", step);
                if (i + 1 >= p->counter) {
                    break;
                }
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

void dsRacing() {// unchecked
    drive_setRampStep(2000);
    struct queueMember *p = findPath();
    currentNode = nodes[0][0];
    drive_goto(-30, -30);
    printf("\n**********DRIVE_SPEED HEADING BACK**********\n");
    enum moveType lastMove = init;
    dsInitMove(); // first move is always going up
    int i = 1;
    while (i < p->counter) {
        if (i >= p->counter - 1) { //reached the end
            printf("Reaching the end\n");
            moveForward();
            return;
        }
        printf("At node (%d,%d)\nFollowing nodes are: (%d,%d)(%d,%d)\n", currentNode->x, currentNode->y,
               p->path[i]->x,
               p->path[i]->y, p->path[i + 1]->x, p->path[i + 1]->y);
//        printf("current i is: %d, counter is: %d\n", i, p->counter);
        if (hasAdjacent(front) &&
            findAdjacent(front) != p->path[i]) { // next node is not in the front -- something went wrong
            printf("dsRacing ERROR\n");
        }
        currentNode = p->path[i];
        i++;
//        printf("Printing path[i]\n");
//        printf("path[i] is (%d,%d)\n", p->path[i]->x, p->path[i]->y);

        if (hasAdjacent(front) && findAdjacent(front) == p->path[i]) { // move front
            switch (lastMove) {
                case init:
                    dsMoveForward();
                    printf("dsMoveForward\n");
                    break;
                case rightTurn:
                    dsMoveForwardAfter90Turn();
                    printf("dsMoveForwardAfter90Turn\n");
                    break;
                case leftTurn:
                    dsMoveForwardAfter90Turn();
                    printf("dsMoveForwardAfter90Turn\n");
                    break;
                case turn180:
                    dsMoveForwardAfter180Turn();
                    printf("dsMoveForwardAfter180Turn\n");
                    break;
                case forwarding:
                    dsMoveForwardAfterForwarding();
                    printf("dsMoveForwardAfterForwarding\n");
            }
            lastMove = forwarding;
        } else if (hasAdjacent(rLeft) && findAdjacent(rLeft) == p->path[i]) { // turn left
            switch (lastMove) {
                case init: // not possible
                    break;
                case rightTurn:
                    dsTurnLeft();
                    lastMove = leftTurn;
                    printf("dsTurnLeft\n");
                    break;
                case leftTurn:
                    dsTurnLeftAfterLeftTurn();
                    lastMove = turn180;
                    printf("dsTurnLeftAfterLeftTurn\n");
                    break;
                case turn180:
                    dsTurnLeft();
                    lastMove = leftTurn;
                    printf("dsTurnLeft\n");
                    break;
                case forwarding:
                    dsTurnLeft();
                    lastMove = leftTurn;
                    printf("dsTurnLeft\n");
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
        } else if (hasAdjacent(rRight) && findAdjacent(rRight) == p->path[i]) { // turn right
            switch (lastMove) {
                case init:
                    dsTurnRightAfterInit();
                    lastMove = rightTurn;
                    printf("dsTurnRightAfterInit\n");
                    break;
                case rightTurn:
                    dsTurnRightAfterRightTurn();
                    lastMove = turn180;
                    printf("dsTurnRightAfterRightTurn\n");
                    break;
                case leftTurn:
                    dsTurnRight();
                    lastMove = rightTurn;
                    printf("dsTurnRight\n");
                    break;
                case turn180:
                    dsTurnRight();
                    lastMove = rightTurn;
                    printf("dsTurnRight\n");
                    break;
                case forwarding:
                    dsTurnRight();
                    lastMove = rightTurn;
                    printf("dsTurnRight\n");
            }
            switch (currentDir) { // change current dirction
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
        }

    }
}

int main() { // Trémaux's Algorithm // unchecked
    initialiseNode();
    simulator_startNewSmokeTrail();
    //TODO: find out if malloc is needed -- seems to be unnecessary
//    currentNode = malloc(sizeof(struct node));
    currentNode = nodes[0][0];
    drive_goto(30, 30); // initialize to first middle point
    while (1) {
        if (currentNode == nodes[0][0] && marchingState == backward) {
            printf("Traversed the maze\n");
            break;
        }
        if ((marchingState == forward && !atJunction()) ||
            (marchingState == backward && currentNode->junction == 0)) {
            printf("At node (%d,%d).\n", currentNode->x, currentNode->y);
            if (leftClear() + rightClear() + frontClear() > 0) { // not dead end
                moveAlongPath();
            } else {
                printf("At node (%d,%d): ", currentNode->x, currentNode->y);
                printf("3. Marching forward into a dead end.\n");
                //turnAround();
                marchingState = backward;
                moveBackward();
            }
            //printMatrix();
            continue;
        }
        // At junction:
        currentNode->junction = 1;
        if (marchingState == forward) {
            if (!atOldJunction()) {
                printf("At node (%d,%d): ", currentNode->x, currentNode->y);
                printf("1. Marching forward into a new junction:\n");
                updateAdjacentTag(back, X);
                goToEmpty();
//                currentNode->tag = N; //???
            } else if (atOldJunction()) {
                printf("At node (%d,%d): ", currentNode->x, currentNode->y);
                printf("2. Marching forward into an old junction:\n");

                updateAdjacentTag(back, N);// findAdjacent(back)->tag = N;
                marchingState = backward;
                moveBackward();
            }
        } else {
            // marching backward -- has to be old junction
            // always face exit with back when marching back to old junction
            faceExitWithBack();
            if (hasEmptyAdjNode()) {
                printf("At node (%d,%d): ", currentNode->x, currentNode->y);
                printf("4. Marching backward into a junction with some unlabeled passages\n");
                goToEmpty();
                marchingState = forward;
            } else {
                printf("At node (%d,%d): ", currentNode->x, currentNode->y);
                printf("5. Marching backward into a junction with no unlabeled passages\n");
                moveBackward();
            }
        }
    }

    printMatrix();
    dsRacing();

}
