// bubbleshooter.cpp - Bubble Shooter: Combo Chaos (DSA project). The whole program is in this file.
//
// Game window : move the mouse to aim, left click to shoot. Buttons: New Game, Undo.
// Text window : press H S F D T B in the game window to see the data structures here
//               (history, sort, find, delete, BST traversals, binary search).
//
// Build: run build.bat (needs MSYS2 g++ and SFML 3)
#include <SFML/Graphics.hpp>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
using namespace std;

// #############################################################################################
// PART 1 - GAME LOGIC AND DATA STRUCTURES (no graphics in this part)
// #############################################################################################

// ============================ CONSTANTS & GLOBALS ============================
const int ROWS = 10;
const int COLS = 10;
const int QUEUE_SIZE = 10;
const int STACK_SIZE = 20;
const int COLORS = 5;
const int START_ROWS = 4;        // rows filled with bubbles when a game starts
const int START_QUEUE = 5;       // how many upcoming bubbles the queue holds
const int MIN_MATCH = 3;         // smallest connected group that gets popped
const int DANGER_ROW = ROWS - 2; // any bubble on this row or below = game over
int board[ROWS][COLS];
bool visited[ROWS][COLS];
int score = 0;
int shotsTaken = 0;
int currentBubble = 1;
int nextGameID = 1;
bool gameActive = false;

// ============================ QUEUE (upcoming bubbles) ============================
class Queue {
  public:
    int bubbles[QUEUE_SIZE];
    int front = 0;
    int rear = -1;
    int count = 0;
};
Queue bubbleQueue;
bool isQueueEmpty() { return bubbleQueue.count == 0; }
bool isQueueFull() { return bubbleQueue.count == QUEUE_SIZE; }
void enqueue(int color) {
    if (isQueueFull()) {
        cout << "Queue is full!\n";
        return;
    }
    bubbleQueue.rear = (bubbleQueue.rear + 1) % QUEUE_SIZE;
    bubbleQueue.bubbles[bubbleQueue.rear] = color;
    bubbleQueue.count++;
}
int dequeue() {
    if (isQueueEmpty()) {
        return -1;
    }
    int color = bubbleQueue.bubbles[bubbleQueue.front];
    bubbleQueue.front = (bubbleQueue.front + 1) % QUEUE_SIZE;
    bubbleQueue.count--;
    return color;
}
int getFront() {
    if (isQueueEmpty())
        return -1;
    return bubbleQueue.bubbles[bubbleQueue.front];
}
int randomColor() { return rand() % COLORS + 1; } // any colour 1-5 (used to fill the starting board)
bool colorOnBoard(int color) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (board[i][j] == color)
                return true;
        }
    }
    return false;
}
// only colours still on the board, so every bubble can help clear it (no endless games)
int generateRandomBubble() {
    int available[COLORS];
    int n = 0;
    for (int color = 1; color <= COLORS; color++) {
        if (colorOnBoard(color))
            available[n++] = color;
    }
    if (n == 0) // empty board (game already won): any colour
        return randomColor();
    return available[rand() % n];
}
void initializeQueue() {
    bubbleQueue.front = 0;
    bubbleQueue.rear = -1;
    bubbleQueue.count = 0;
    for (int i = 0; i < START_QUEUE; i++) {
        enqueue(generateRandomBubble());
    }
}
// take the next colour out of the queue as the current bubble and refill the queue
void nextBubble() {
    // a colour may have been cleared from the board since it was queued: replace it
    for (int i = 0; i < bubbleQueue.count; i++) {
        int index = (bubbleQueue.front + i) % QUEUE_SIZE;
        if (!colorOnBoard(bubbleQueue.bubbles[index]))
            bubbleQueue.bubbles[index] = generateRandomBubble();
    }
    currentBubble = dequeue();
    if (currentBubble == -1) {
        currentBubble = generateRandomBubble();
    }
    enqueue(generateRandomBubble());
}

// ============================ STACK (undo) ============================
class GameState {
  public:
    int savedBoard[ROWS][COLS];
    int savedScore;
    int savedShots;
    int savedCurrentBubble;
    Queue savedQueue;
};
class Stack {
  public:
    GameState states[STACK_SIZE];
    int top = -1;
};
Stack undoStack;
void initializeStack() { undoStack.top = -1; }
bool isStackEmpty() { return undoStack.top == -1; }
bool isStackFull() { return undoStack.top == STACK_SIZE - 1; }
void push() {
    if (isStackFull()) {
        // history is full: throw away the OLDEST state (bottom of the stack)
        // so the most recent moves can still be undone one at a time
        for (int i = 0; i < STACK_SIZE - 1; i++) {
            undoStack.states[i] = undoStack.states[i + 1];
        }
        undoStack.top--;
    }
    undoStack.top++;
    GameState &state = undoStack.states[undoStack.top];
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            state.savedBoard[i][j] = board[i][j];
        }
    }
    state.savedScore = score;
    state.savedShots = shotsTaken;
    state.savedCurrentBubble = currentBubble;
    state.savedQueue = bubbleQueue;
}
bool pop(GameState &state) {
    if (isStackEmpty())
        return false;
    state = undoStack.states[undoStack.top--];
    return true;
}
bool peek(GameState &state) {
    if (isStackEmpty())
        return false;
    state = undoStack.states[undoStack.top];
    return true;
}
void undoMove() {
    if (!gameActive) {
        cout << "No active game to undo!\n";
        return;
    }
    GameState state;
    if (!pop(state)) {
        cout << "Nothing to undo!\n";
        return;
    }
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            board[i][j] = state.savedBoard[i][j];
        }
    }
    score = state.savedScore;
    shotsTaken = state.savedShots;
    currentBubble = state.savedCurrentBubble;
    bubbleQueue = state.savedQueue;
    cout << "Previous move restored!\n";
}

// ============================ BOARD (2D array) ============================
bool isValidCell(int row, int col) { return row >= 0 && row < ROWS && col >= 0 && col < COLS; }
bool isEmpty(int row, int col) { return isValidCell(row, col) && board[row][col] == 0; }
void initializeBoard() {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            board[i][j] = 0;
        }
    }
    for (int i = 0; i < START_ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            board[i][j] = randomColor();
        }
    }
}
void resetBoard() { initializeBoard(); }
int countBubbles() {
    int count = 0;
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (board[i][j] != 0)
                count++;
        }
    }
    return count;
}
void placeBubble(int row, int col, int color) {
    if (isValidCell(row, col) && isEmpty(row, col)) {
        board[row][col] = color;
    }
}
void removeBubble(int row, int col) {
    if (isValidCell(row, col)) {
        board[row][col] = 0;
    }
}
bool isBoardEmpty() {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (board[i][j] != 0)
                return false;
        }
    }
    return true;
}
// a shot bubble flies upward, so it can only stop when it touches the top wall
// or a bubble above / beside it - it can never hang in empty space
bool isAttached(int row, int col) {
    if (row == 0)
        return true;
    if (board[row - 1][col] != 0)
        return true;
    if (col > 0 && board[row][col - 1] != 0)
        return true;
    if (col < COLS - 1 && board[row][col + 1] != 0)
        return true;
    return false;
}

// ============================ SCORING ============================
int calculateScore(int matches) { return matches * 10; }
void updateScore(int points) { score += points; }
void resetScore() { score = 0; }

// ============================ RECURSION (matching) ============================
int matchRows[ROWS * COLS];
int matchCols[ROWS * COLS];
int matchCount = 0;
void resetVisited() {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            visited[i][j] = false;
        }
    }
}
bool isMatchingColor(int row, int col, int color) {
    return isValidCell(row, col) && !visited[row][col] && color != 0 && board[row][col] == color;
}
void checkConnected(int row, int col, int color) {
    if (!isMatchingColor(row, col, color))
        return;
    visited[row][col] = true;
    matchRows[matchCount] = row;
    matchCols[matchCount] = col;
    matchCount++;
    checkConnected(row - 1, col, color);
    checkConnected(row + 1, col, color);
    checkConnected(row, col - 1, color);
    checkConnected(row, col + 1, color);
}
int findMatches(int row, int col) {
    resetVisited();
    matchCount = 0;
    if (!isValidCell(row, col) || isEmpty(row, col))
        return 0;
    int color = board[row][col];
    checkConnected(row, col, color);
    return matchCount;
}
int countMatches() { return matchCount; }
// removes the group found by findMatches() only if it is big enough; returns how many popped
int removeMatches() {
    if (matchCount < MIN_MATCH)
        return 0;
    for (int i = 0; i < matchCount; i++) {
        removeBubble(matchRows[i], matchCols[i]);
    }
    cout << matchCount << " bubbles removed!\n";
    return matchCount;
}
void processMatches(int row, int col) {
    int totalMatches = findMatches(row, col);
    cout << "Connected bubbles: " << totalMatches << endl;
    int removed = removeMatches();
    if (removed > 0) {
        updateScore(calculateScore(removed));
        cout << "Score updated: " << score << endl;
    } else {
        cout << "No matching cluster found.\n";
    }
}

// ============================ WIN / LOSE ============================
bool checkWin() { return isBoardEmpty(); }
bool checkLose() {
    for (int i = DANGER_ROW; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (board[i][j] != 0)
                return true;
        }
    }
    return false;
}

// ============================ SINGLY LINKED LIST (game history) ============================
class GameRecord {
  public:
    int gameID;
    int score;
    int shots;
    bool won;
    GameRecord *next;
};
GameRecord *head = NULL;
void insertRecord(int id, int points, int shots, bool result) {
    GameRecord *newNode = new GameRecord;
    newNode->gameID = id;
    newNode->score = points;
    newNode->shots = shots;
    newNode->won = result;
    newNode->next = NULL;
    if (head == NULL) {
        head = newNode;
        return;
    }
    GameRecord *temp = head;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->next = newNode;
}
void displayRecords() {
    if (head == NULL) {
        cout << "No game records found!\n";
        return;
    }

    GameRecord *temp = head;
    cout << "\n========== GAME HISTORY ==========\n";
    while (temp != NULL) {
        cout << "Game ID: " << temp->gameID << " | Score: " << temp->score
             << " | Shots: " << temp->shots << " | Result: " << (temp->won ? "Win" : "Loss")
             << endl;
        temp = temp->next;
    }
}
GameRecord *searchRecord(int id) {
    GameRecord *temp = head;
    while (temp != NULL) {
        if (temp->gameID == id)
            return temp;
        temp = temp->next;
    }
    return NULL;
}
void deleteRecord(int id) {
    if (head == NULL)
        return;
    if (head->gameID == id) {
        GameRecord *temp = head;
        head = head->next;
        delete temp;
        cout << "Record deleted!\n";
        return;
    }
    GameRecord *temp = head;
    while (temp->next != NULL && temp->next->gameID != id) {
        temp = temp->next;
    }
    if (temp->next == NULL) {
        cout << "Record not found!\n";
        return;
    }
    GameRecord *del = temp->next;
    temp->next = del->next;
    delete del;
    cout << "Record deleted!\n";
}
int countRecords() {
    int count = 0;
    GameRecord *temp = head;
    while (temp != NULL) {
        count++;
        temp = temp->next;
    }
    return count;
}
GameRecord *getMiddle(GameRecord *start) {
    if (start == NULL)

        return NULL;
    GameRecord *slow = start;
    GameRecord *fast = start->next;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
    }
    return slow;
}
GameRecord *mergeLists(GameRecord *a, GameRecord *b) {
    if (a == NULL)
        return b;
    if (b == NULL)
        return a;
    GameRecord *result = NULL;
    if (a->score >= b->score) {
        result = a;
        result->next = mergeLists(a->next, b);
    } else {
        result = b;
        result->next = mergeLists(a, b->next);
    }
    return result;
}
GameRecord *mergeSort(GameRecord *start) {
    if (start == NULL || start->next == NULL)
        return start;
    GameRecord *middle = getMiddle(start);
    GameRecord *secondHalf = middle->next;
    middle->next = NULL;
    GameRecord *left = mergeSort(start);
    GameRecord *right = mergeSort(secondHalf);
    return mergeLists(left, right);
}
void sortRecords() {
    head = mergeSort(head);
    cout << "Game records sorted by score!\n";
}

// ============================ BST (leaderboard) ============================
class TreeNode {
  public:
    int score;
    int gameID;
    TreeNode *left;
    TreeNode *right;
};
TreeNode *root = NULL;
bool comesBefore(int score1, int id1, int score2, int id2) {
    if (score1 != score2)
        return score1 < score2;
    return id1 < id2;
}
TreeNode *insertNode(TreeNode *node, int points, int id) {
    if (node == NULL) {
        TreeNode *newNode = new TreeNode;
        newNode->score = points;
        newNode->gameID = id;
        newNode->left = NULL;
        newNode->right = NULL;
        return newNode;
    }
    if (comesBefore(points, id, node->score, node->gameID)) {
        node->left = insertNode(node->left, points, id);
    } else {
        node->right = insertNode(node->right, points, id);
    }
    return node;
}
TreeNode *searchNode(TreeNode *node, int points, int id) {
    if (node == NULL)
        return NULL;
    if (node->score == points && node->gameID == id)
        return node;
    if (comesBefore(points, id, node->score, node->gameID))

        return searchNode(node->left, points, id);
    return searchNode(node->right, points, id);
}
TreeNode *findMin(TreeNode *node) {
    if (node == NULL)
        return NULL;
    while (node->left != NULL) {
        node = node->left;
    }
    return node;
}
TreeNode *findMax(TreeNode *node) {
    if (node == NULL)
        return NULL;
    while (node->right != NULL) {
        node = node->right;
    }
    return node;
}
void inorder(TreeNode *node) {
    if (node == NULL)
        return;
    inorder(node->left);
    cout << "Game " << node->gameID << " | Score: " << node->score << endl;
    inorder(node->right);
}
void preorder(TreeNode *node) {
    if (node == NULL)
        return;
    cout << node->score << " ";
    preorder(node->left);
    preorder(node->right);
}
void postorder(TreeNode *node) {
    if (node == NULL)
        return;
    postorder(node->left);

    postorder(node->right);
    cout << node->score << " ";
}
TreeNode *deleteNode(TreeNode *node, int points, int id) {
    if (node == NULL)
        return NULL;
    if (comesBefore(points, id, node->score, node->gameID)) {
        node->left = deleteNode(node->left, points, id);
    } else if (comesBefore(node->score, node->gameID, points, id)) {
        node->right = deleteNode(node->right, points, id);
    } else {
        if (node->left == NULL) {
            TreeNode *temp = node->right;
            delete node;
            return temp;
        }
        if (node->right == NULL) {
            TreeNode *temp = node->left;
            delete node;
            return temp;
        }
        TreeNode *successor = findMin(node->right);
        node->score = successor->score;
        node->gameID = successor->gameID;
        node->right = deleteNode(node->right, successor->score, successor->gameID);
    }
    return node;
}
int height(TreeNode *node) {
    if (node == NULL)
        return 0;

    int leftHeight = height(node->left);
    int rightHeight = height(node->right);
    if (leftHeight > rightHeight)
        return leftHeight + 1;
    return rightHeight + 1;
}
int countNodes(TreeNode *node) {
    if (node == NULL)
        return 0;
    return 1 + countNodes(node->left) + countNodes(node->right);
}
// highest score ever recorded = right-most node of the BST
int getHighScore() {
    TreeNode *best = findMax(root);
    if (best == NULL)
        return 0;
    return best->score;
}

// ============================ SEARCHING (binary search) ============================
// inorder traversal of a BST visits keys in ascending order -> fills a sorted array
void storeInorder(TreeNode *node, int arr[], int &index) {
    if (node == NULL)
        return;
    storeInorder(node->left, arr, index);
    arr[index++] = node->score;
    storeInorder(node->right, arr, index);
}
int binarySearch(int arr[], int size, int key) {
    int low = 0;
    int high = size - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (arr[mid] == key)
            return mid;
        if (arr[mid] < key)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return -1;
}

// ============================ RECORDS + LEADERBOARD (kept in sync) ============================
void updateRecord(int id, int newScore) {
    GameRecord *temp = searchRecord(id);
    if (temp == NULL) {
        cout << "Record not found!\n";
        return;
    }
    // the BST is ordered by score, so the old key must be removed and the new one inserted
    root = deleteNode(root, temp->score, temp->gameID);
    temp->score = newScore;
    root = insertNode(root, temp->score, temp->gameID);

    cout << "Record updated!\n";
}
void saveGameResult(bool won) {
    int id = nextGameID++;
    insertRecord(id, score, shotsTaken, won);
    root = insertNode(root, score, id);
    cout << "\nGame saved successfully!\n";
    cout << "Game ID: " << id << endl;
    cout << "Final score: " << score << endl;
    cout << "Best score so far: " << getHighScore() << endl;
}

// ============================ GAME CONTROL ============================
bool shootBubble(int row, int col) {
    if (!gameActive) {
        cout << "Start a game first!\n";
        return false;
    }
    if (!isValidCell(row, col) || !isEmpty(row, col)) {
        cout << "Invalid position!\n";
        return false;
    }
    if (!isAttached(row, col)) {
        cout << "A bubble must touch the top wall or another bubble (above or beside it)!\n";
        return false;
    }
    push();
    placeBubble(row, col, currentBubble);
    shotsTaken++;
    cout << "Bubble placed: " << currentBubble << endl;
    processMatches(row, col);
    nextBubble();
    return true;
}
void startGame() {
    initializeBoard();
    initializeQueue();

    initializeStack();
    resetScore();
    shotsTaken = 0;
    nextBubble();
    gameActive = true;
    cout << "\nNew game started!\n";
    cout << "Current bubble: " << currentBubble << endl;
}
void restartGame() {
    cout << "\nCurrent game abandoned (it is not saved).\n";
    startGame();
}
void endGame(bool won) {
    gameActive = false;
    saveGameResult(won);
    initializeStack(); // a finished game is final - its undo history is cleared
}
void clearRecords() {
    while (head != NULL) {
        GameRecord *temp = head;
        head = head->next;
        delete temp;
    }
}
void clearTree(TreeNode *node) {
    if (node == NULL)
        return;
    clearTree(node->left);
    clearTree(node->right);
    delete node;
}
void quitGame() {
    clearRecords();
    clearTree(root);
    root = NULL;
}

// #############################################################################################
// PART 2 - THE WINDOW (SFML): aiming, the flying bubble, mouse, keyboard and drawing.
// It only calls the Part 1 functions above - all game rules stay in Part 1.
// #############################################################################################

// ============================ LAYOUT (pixels) ============================
const float CELL = 50.f;                  // one board cell
const float RADIUS = CELL / 2.f - 2.f;    // bubble radius
const float BOARD_X = 20.f;               // top-left corner of the board
const float BOARD_Y = 20.f;
const float BOARD_W = COLS * CELL;
const float BOARD_H = ROWS * CELL;
const float LAUNCHER_X = BOARD_X + BOARD_W / 2.f;
const float LAUNCHER_Y = BOARD_Y + BOARD_H + 60.f;
const float PANEL_X = BOARD_X + BOARD_W + 25.f; // side panel with score, buttons...
const unsigned WINDOW_W = 790;
const unsigned WINDOW_H = 650;
const float SHOT_SPEED = 900.f;           // pixels per second
const float STEP = 4.f;                   // the bubble moves at most this far between collision checks
const float HIT_DISTANCE = CELL * 0.85f;  // centre-to-centre distance that counts as touching
const float PI = 3.14159265f;

// ============================ SHOOTING STATE ============================
struct MovingBubble {
    float x, y;
    float velocityX, velocityY;
    int color;
    bool isMoving;
};
MovingBubble shot = {LAUNCHER_X, LAUNCHER_Y, 0.f, 0.f, 0, false};
float aimX = 0.f; // normalised aim direction (starts straight up)
float aimY = -1.f;

// what the screen shows besides the board
string statusText = "";
bool gameOver = false;
bool lastGameWon = false;
int lastGameID = 0;
char asking = 0;          // 'F', 'D' or 'B' while waiting for the player to type a number
string typedNumber = "";  // the digits typed so far

struct Button {
    float x, y, width, height;
    string label;
};
const Button newGameButton = {PANEL_X, 248.f, 200.f, 38.f, "New Game"};
const Button undoButton = {PANEL_X, 294.f, 200.f, 38.f, "Undo"};

sf::Vector2f cellCenter(int row, int col) {
    return {BOARD_X + col * CELL + CELL / 2.f, BOARD_Y + row * CELL + CELL / 2.f};
}

// ============================ SHOOTING & COLLISION (sheet Q5) ============================
// direction from the launcher to the mouse, kept pointing upwards so a shot can't go down
void aimLauncher(sf::Vector2f mouse) {
    float angle = atan2(mouse.y - LAUNCHER_Y, mouse.x - LAUNCHER_X);
    const float minAngle = -PI + 0.15f; // almost flat to the left
    const float maxAngle = -0.15f;      // almost flat to the right
    if (angle > maxAngle && angle <= PI / 2.f)
        angle = maxAngle;
    else if (angle > PI / 2.f || angle < minAngle)
        angle = minAngle;
    aimX = cos(angle);
    aimY = sin(angle);
}
// sheet name: shootBubble() - renamed because shootBubble(row, col) already places a bubble
void launchBubble() {
    if (!gameActive || shot.isMoving)
        return; // only one bubble in the air at a time
    shot.x = LAUNCHER_X;
    shot.y = LAUNCHER_Y;
    shot.velocityX = aimX * SHOT_SPEED;
    shot.velocityY = aimY * SHOT_SPEED;
    shot.color = currentBubble;
    shot.isMoving = true;
}
void checkWallCollision() {
    float left = BOARD_X + RADIUS;
    float right = BOARD_X + BOARD_W - RADIUS;
    if (shot.x < left) {
        shot.x = left;
        shot.velocityX = fabs(shot.velocityX); // bounce to the right
    } else if (shot.x > right) {
        shot.x = right;
        shot.velocityX = -fabs(shot.velocityX); // bounce to the left
    }
}
// touching the top wall or any bubble on the board?
bool checkCollision() {
    if (shot.y - RADIUS <= BOARD_Y)
        return true;
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (board[i][j] == 0)
                continue;
            sf::Vector2f c = cellCenter(i, j);
            float dx = shot.x - c.x;
            float dy = shot.y - c.y;
            if (dx * dx + dy * dy < HIT_DISTANCE * HIT_DISTANCE)
                return true;
        }
    }
    return false;
}
// the empty cell (touching the ceiling or a bubble) whose centre is closest to the shot
bool findNearestCell(int &bestRow, int &bestCol) {
    float bestDistance = -1.f;
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (!isEmpty(i, j) || !isAttached(i, j))
                continue;
            sf::Vector2f c = cellCenter(i, j);
            float distance = (shot.x - c.x) * (shot.x - c.x) + (shot.y - c.y) * (shot.y - c.y);
            if (bestDistance < 0.f || distance < bestDistance) {
                bestDistance = distance;
                bestRow = i;
                bestCol = j;
            }
        }
    }
    return bestDistance >= 0.f;
}
// stop, place the bubble, pop matches, update score, check win/lose, load the next bubble
void finishShot(int row, int col) {
    shot.isMoving = false;
    int scoreBefore = score;
    if (!shootBubble(row, col)) // Part 1: undo snapshot, place, match, score, next bubble
        return;
    int gained = score - scoreBefore;
    if (gained > 0)
        statusText = "Popped " + to_string(countMatches()) + " bubbles!  +" + to_string(gained);
    else
        statusText = "";
    if (checkWin() || checkLose()) {
        lastGameWon = checkWin();
        endGame(lastGameWon); // saves the record (linked list) and score (BST)
        lastGameID = nextGameID - 1;
        gameOver = true;
        statusText = "";
    }
}
void snapBubbleToGrid() {
    shot.isMoving = false;
    int row = -1, col = -1;
    if (findNearestCell(row, col))
        finishShot(row, col);
}
void updateBubblePosition(float dt) {
    if (!shot.isMoving)
        return;
    // move in small steps so a fast bubble can never jump through another bubble
    int steps = (int)ceil(SHOT_SPEED * dt / STEP);
    if (steps < 1)
        steps = 1;
    float stepTime = dt / steps;
    for (int i = 0; i < steps; i++) {
        shot.x += shot.velocityX * stepTime;
        shot.y += shot.velocityY * stepTime;
        checkWallCollision();
        if (checkCollision()) {
            snapBubbleToGrid();
            return;
        }
    }
}

// ============================ MOUSE & KEYBOARD ============================
bool isInside(const Button &b, sf::Vector2f p) {
    return p.x >= b.x && p.x <= b.x + b.width && p.y >= b.y && p.y <= b.y + b.height;
}
bool canUndo() { return gameActive && !shot.isMoving && !isStackEmpty(); }
void newGame() {
    shot.isMoving = false;
    if (gameActive)
        restartGame();
    else
        startGame();
    gameOver = false;
    statusText = "New game - good luck!";
}
void pressUndo() {
    if (!canUndo()) {
        statusText = "Nothing to undo";
        return;
    }
    undoMove();
    statusText = "Last shot undone";
}
void handleClick(sf::Vector2f p) {
    if (isInside(newGameButton, p))
        newGame();
    else if (isInside(undoButton, p))
        pressUndo();
    else if (p.x < PANEL_X - 10.f)
        launchBubble(); // a click anywhere on the play area shoots
}
// keys that show the data structures - the results are printed in the text window
void runCommand(char key, int number) {
    if (key == 'H') { // linked list traversal
        displayRecords();
        cout << "Total games: " << countRecords() << endl;
    } else if (key == 'S') { // merge sort on the linked list
        sortRecords();
        displayRecords();
    } else if (key == 'F') { // linear search
        GameRecord *record = searchRecord(number);
        if (record == NULL)
            cout << "Game " << number << " not found!\n";
        else
            cout << "Game " << record->gameID << " | Score: " << record->score << " | Shots: " << record->shots
                 << " | Result: " << (record->won ? "Win" : "Loss") << endl;
    } else if (key == 'D') { // delete from the linked list AND the BST
        GameRecord *record = searchRecord(number);
        if (record == NULL) {
            cout << "Game " << number << " not found!\n";
        } else {
            root = deleteNode(root, record->score, record->gameID); // BST first (it needs the score)
            deleteRecord(number);
        }
    } else if (key == 'T') { // BST traversals
        cout << "\n========== LEADERBOARD (BST) ==========\n";
        inorder(root);
        cout << "Preorder: ";
        preorder(root);
        cout << "\nPostorder: ";
        postorder(root);
        cout << "\nHeight: " << height(root) << endl;
        if (root != NULL)
            cout << "Lowest: " << findMin(root)->score << "   Highest: " << findMax(root)->score << endl;
    } else if (key == 'B') { // BST -> sorted array -> binary search
        int total = countNodes(root);
        if (total == 0) {
            cout << "No scores yet!\n";
        } else {
            int *scores = new int[total];
            int index = 0;
            storeInorder(root, scores, index);
            cout << "Sorted scores: ";
            for (int i = 0; i < total; i++)
                cout << scores[i] << " ";
            int position = binarySearch(scores, total, number);
            if (position != -1)
                cout << "\nScore " << number << " found at index " << position << "!\n";
            else
                cout << "\nScore " << number << " not found!\n";
            delete[] scores;
        }
    }
    statusText = "Results are in the text window";
}
// H, S, T run at once. F, D, B first ask for a number, which is typed in the game window.
void handleTyping(char32_t c) {
    if (asking == 0) {
        if (c >= 'a' && c <= 'z')
            c = c - 'a' + 'A'; // lower case works too
        if (c == 'H' || c == 'S' || c == 'T') {
            runCommand((char)c, 0);
        } else if (c == 'F' || c == 'D' || c == 'B') {
            asking = (char)c;
            typedNumber = "";
        }
        return;
    }
    if (c >= '0' && c <= '9' && typedNumber.size() < 6) {
        typedNumber += (char)c;
    } else if (c == 8 && !typedNumber.empty()) { // Backspace
        typedNumber.pop_back();
    } else if ((c == 13 || c == 10) && !typedNumber.empty()) { // Enter
        runCommand(asking, stoi(typedNumber));
        asking = 0;
    } else if (c == 27) { // Esc cancels
        asking = 0;
        statusText = "";
    }
}

// reverse inorder (right, root, left) visits the BST from the highest score down
void collectTopScores(TreeNode *node, int scores[], int ids[], int &n, int maxCount) {
    if (node == NULL || n >= maxCount)
        return;
    collectTopScores(node->right, scores, ids, n, maxCount);
    if (n < maxCount) {
        scores[n] = node->score;
        ids[n] = node->gameID;
        n++;
    }
    collectTopScores(node->left, scores, ids, n, maxCount);
}

// ============================ DRAWING ============================
sf::Color bubbleColor(int color) {
    switch (color) {
    case 1:
        return sf::Color(220, 60, 60); // red
    case 2:
        return sf::Color(60, 110, 230); // blue
    case 3:
        return sf::Color(60, 180, 80); // green
    case 4:
        return sf::Color(235, 200, 50); // yellow
    case 5:
        return sf::Color(150, 80, 200); // purple
    }
    return sf::Color(128, 128, 128);
}
void drawBubble(sf::RenderTarget &target, sf::Vector2f center, float radius, int color) {
    sf::CircleShape circle(radius);
    circle.setOrigin({radius, radius});
    circle.setPosition(center);
    sf::Color fill = bubbleColor(color);
    circle.setFillColor(fill);
    circle.setOutlineThickness(2.f);
    circle.setOutlineColor(sf::Color(fill.r / 2, fill.g / 2, fill.b / 2));
    target.draw(circle);
}
void drawText(sf::RenderTarget &target, const sf::Font *font, const string &str, float x, float y,
              unsigned size, sf::Color color, bool centered = false) {
    if (font == NULL || str.empty())
        return; // no font found: the game still works, just without text
    sf::Text text(*font, str, size);
    text.setFillColor(color);
    if (centered) {
        sf::FloatRect bounds = text.getLocalBounds();
        x -= bounds.position.x + bounds.size.x / 2.f;
    }
    text.setPosition({x, y});
    target.draw(text);
}
void drawButton(sf::RenderTarget &target, const sf::Font *font, const Button &b, bool hovered,
                bool enabled) {
    sf::RectangleShape box({b.width, b.height});
    box.setPosition({b.x, b.y});
    sf::Color fill(70, 76, 95);
    if (!enabled)
        fill = sf::Color(48, 50, 58);
    else if (hovered)
        fill = sf::Color(95, 105, 140);
    box.setFillColor(fill);
    box.setOutlineThickness(1.f);
    box.setOutlineColor(sf::Color(130, 135, 150));
    target.draw(box);
    sf::Color textColor = enabled ? sf::Color::White : sf::Color(110, 110, 110);
    drawText(target, font, b.label, b.x + b.width / 2.f, b.y + 8.f, 18, textColor, true);
}
void drawGame(sf::RenderTarget &target, const sf::Font *font, sf::Vector2f mouse) {
    const sf::Color white(235, 235, 235);
    const sf::Color gray(150, 150, 160);
    target.clear(sf::Color(28, 30, 38));

    // board background and danger line
    sf::RectangleShape boardBox({BOARD_W, BOARD_H});
    boardBox.setPosition({BOARD_X, BOARD_Y});
    boardBox.setFillColor(sf::Color(42, 46, 58));
    boardBox.setOutlineThickness(2.f);
    boardBox.setOutlineColor(sf::Color(90, 95, 110));
    target.draw(boardBox);
    sf::RectangleShape dangerLine({BOARD_W, 2.f});
    dangerLine.setPosition({BOARD_X, BOARD_Y + DANGER_ROW * CELL});
    dangerLine.setFillColor(sf::Color(200, 60, 60));
    target.draw(dangerLine);
    drawText(target, font, "danger line", BOARD_X + BOARD_W - 85.f, BOARD_Y + DANGER_ROW * CELL + 4.f, 13,
             sf::Color(200, 60, 60));

    // bubbles on the board
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (board[i][j] != 0)
                drawBubble(target, cellCenter(i, j), RADIUS, board[i][j]);
        }
    }

    // aim guide, launcher and the current bubble
    bool canShoot = gameActive && !shot.isMoving;
    if (canShoot) {
        for (int i = 1; i <= 8; i++) {
            float d = 30.f + i * 18.f;
            sf::CircleShape dot(3.f);
            dot.setOrigin({3.f, 3.f});
            dot.setPosition({LAUNCHER_X + aimX * d, LAUNCHER_Y + aimY * d});
            dot.setFillColor(sf::Color(220, 220, 220, 170));
            target.draw(dot);
        }
    }
    sf::CircleShape base(RADIUS + 8.f);
    base.setOrigin({RADIUS + 8.f, RADIUS + 8.f});
    base.setPosition({LAUNCHER_X, LAUNCHER_Y});
    base.setFillColor(sf::Color(58, 62, 76));
    base.setOutlineThickness(2.f);
    base.setOutlineColor(sf::Color(90, 95, 110));
    target.draw(base);
    if (canShoot)
        drawBubble(target, {LAUNCHER_X, LAUNCHER_Y}, RADIUS, currentBubble);
    if (shot.isMoving)
        drawBubble(target, {shot.x, shot.y}, RADIUS, shot.color);
    drawText(target, font, "Move the mouse to aim, left click to shoot", LAUNCHER_X, LAUNCHER_Y + 40.f,
             14, gray, true);

    // side panel
    float x = PANEL_X;
    drawText(target, font, "BUBBLE SHOOTER", x, 20.f, 24, white);
    drawText(target, font, "Score: " + to_string(score), x, 64.f, 20, white);
    drawText(target, font, "Shots: " + to_string(shotsTaken), x, 90.f, 20, white);
    drawText(target, font, "Bubbles left: " + to_string(countBubbles()), x, 116.f, 20, white);
    drawText(target, font, "Best: " + to_string(getHighScore()), x, 142.f, 20, white);
    drawText(target, font, "Upcoming:", x, 180.f, 18, gray);
    for (int i = 0; i < bubbleQueue.count; i++) { // walk the circular queue from front to rear
        int index = (bubbleQueue.front + i) % QUEUE_SIZE;
        drawBubble(target, {x + 16.f + i * 38.f, 222.f}, 15.f, bubbleQueue.bubbles[index]);
    }
    drawButton(target, font, newGameButton, isInside(newGameButton, mouse), true);
    drawButton(target, font, undoButton, isInside(undoButton, mouse), canUndo());

    // status line, or the question while the player types a number
    string message = statusText;
    if (asking == 'F')
        message = "Find game ID: " + typedNumber + "_";
    else if (asking == 'D')
        message = "Delete game ID: " + typedNumber + "_";
    else if (asking == 'B')
        message = "Search score: " + typedNumber + "_";
    drawText(target, font, message, x, 342.f, 16, sf::Color(240, 210, 90));

    drawText(target, font, "Top scores (BST)", x, 378.f, 18, gray);
    int topScores[5], topIds[5], n = 0;
    collectTopScores(root, topScores, topIds, n, 5);
    if (n == 0)
        drawText(target, font, "no finished games yet", x, 404.f, 16, gray);
    for (int i = 0; i < n; i++) {
        string line = to_string(i + 1) + ".  " + to_string(topScores[i]) + "   (game " + to_string(topIds[i]) + ")";
        drawText(target, font, line, x, 404.f + i * 22.f, 16, white);
    }

    // keyboard help
    drawText(target, font, "Keys (output in text window):", x, 530.f, 15, gray);
    const char *keys[] = {"H  history", "S  sort", "F  find game", "D  delete game", "T  BST traversals", "B  binary search"};
    for (int i = 0; i < 6; i++)
        drawText(target, font, keys[i], x + (i % 2) * 126.f, 554.f + (i / 2) * 21.f, 14, white);

    // win / lose message on top of the board
    if (gameOver) {
        sf::RectangleShape shade({BOARD_W, BOARD_H});
        shade.setPosition({BOARD_X, BOARD_Y});
        shade.setFillColor(sf::Color(0, 0, 0, 170));
        target.draw(shade);
        float cx = BOARD_X + BOARD_W / 2.f;
        drawText(target, font, lastGameWon ? "YOU WIN!" : "GAME OVER", cx, 180.f, 44,
                 lastGameWon ? sf::Color(90, 210, 110) : sf::Color(230, 80, 80), true);
        drawText(target, font, "Final score: " + to_string(score), cx, 255.f, 22, white, true);
        drawText(target, font, "Saved as game #" + to_string(lastGameID), cx, 290.f, 18, gray, true);
        drawText(target, font, "Click  New Game  to play again", cx, 335.f, 18, white, true);
    }
}

// SFML has no built-in font: use font.ttf next to the exe if present, otherwise a Windows font
bool loadFont(sf::Font &font) {
    const char *paths[] = {"font.ttf", "C:/Windows/Fonts/arial.ttf", "C:/Windows/Fonts/segoeui.ttf",
                           "C:/Windows/Fonts/tahoma.ttf", "C:/Windows/Fonts/calibri.ttf"};
    for (const char *path : paths) {
        ifstream file(path);
        if (file.good() && font.openFromFile(path))
            return true;
    }
    return false;
}

int main() {
    srand(time(0));
    sf::RenderWindow window(sf::VideoMode({WINDOW_W, WINDOW_H}), "Bubble Shooter",
                            sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);
    sf::Font font;
    bool fontLoaded = loadFont(font);
    if (!fontLoaded)
        cout << "No font found - the game works, but text will not be shown.\n";
    cout << "BUBBLE SHOOTER - this text window shows the data structures.\n";
    cout << "Press H, S, F, D, T or B in the game window to use them.\n";

    startGame();
    sf::Clock clock;
    while (window.isOpen()) {
        while (const optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            } else if (const auto *click = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (click->button == sf::Mouse::Button::Left)
                    handleClick(window.mapPixelToCoords(click->position));
            } else if (const auto *typed = event->getIf<sf::Event::TextEntered>()) {
                handleTyping(typed->unicode);
            }
        }
        float dt = clock.restart().asSeconds();
        if (dt > 0.05f)
            dt = 0.05f; // e.g. after dragging the window: don't let the bubble jump
        sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        if (!shot.isMoving)
            aimLauncher(mouse);
        updateBubblePosition(dt);
        drawGame(window, fontLoaded ? &font : NULL, mouse);
        window.display();
    }
    quitGame();
    return 0;
}
