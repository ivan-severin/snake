#include "config.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/timerfd.h>
#include <termios.h>
#include <unistd.h>

struct Pos {
    int x;
    int y;
};

struct Apple {
    struct Pos pos;
};

void genRandomPos(struct Pos* pos) {
    pos->x = rand() % COLS;
    pos->y = rand() % ROWS;
}

enum Dir { UP, DOWN, LEFT, RIGHT };

struct Snake {
    struct Pos parts[COLS * ROWS];
    enum Dir dir;
    int front;
    int back;
};

void pushFront(struct Snake* s) {
    int n = (s->front) % (ROWS * COLS - 1) + 1;
    // printf("\nfornt: %d (n = %d)\t", s->front, n);
    switch (s->dir) {
    case UP:
        s->parts[n].x = s->parts[n - 1].x;
        s->parts[n].y =
            (s->parts[n - 1].y - 1 >= 0) ? (s->parts[n - 1].y - 1) % ROWS : ROWS - 1;
        break;
    case DOWN:
        s->parts[n].x = s->parts[n - 1].x;
        s->parts[n].y = (s->parts[n - 1].y + 1) % ROWS;
        break;
    case RIGHT:
        s->parts[n].x = (s->parts[n - 1].x + 1) % COLS;
        s->parts[n].y = s->parts[n - 1].y;
        break;
    case LEFT:
        s->parts[n].x =
            (s->parts[n - 1].x - 1 >= 0) ? (s->parts[n - 1].x - 1) % COLS : COLS - 1;
        s->parts[n].y = s->parts[n - 1].y;
        break;
    }
    s->front = n;
}

void popBack(struct Snake* s) {
    // printf("back: %d \n", s->back);
    s->back = (s->back + 1) % (ROWS * COLS - 1);
}

int isBody(const struct Snake* s, int x, int y) {
    for (int i = s->back; i != s->front; (++i % ROWS * COLS)) {
        if (x == s->parts[i].x && y == s->parts[i].y) {
            return 1;
        }
    }

    return 0;
}

void move(struct Snake* s) {
    pushFront(s);
    popBack(s);
}

void setDir(struct Snake* s, char c) {
    switch (c) {
    case 'k':
    case 'w':
        s->dir = UP;
        break;
    case 'j':
    case 's':
        s->dir = DOWN;
        break;
    case 'h':
    case 'a':
        s->dir = LEFT;
        break;
    case 'l':
    case 'd':
        s->dir = RIGHT;
        break;
    }
}

void makeHead(struct Snake* s) {
    s->parts[0].x = COLS / 2 - 1;
    s->parts[0].y = ROWS / 2;
    s->dir = DOWN;
}
int isApple(const struct Apple* s, int x, int y) {
    return x == s->pos.x && y == s->pos.y;
}

void draw(const struct Snake* s, const struct Apple* a) { drawWithText(s, a, NULL, 0); }

void drawWithText(const struct Snake* s, const struct Apple* a, const char* text,
                  const int len) {

    // write(STDOUT_FILENO, "\n\n\n\n\n\n\n\n", 6);
    system("clear");
    // printf("head: x: %d, y: %d\t", s->parts[s->front].x, s->parts[s->front].y);
    // printf("tail: x: %d, y: %d\n", s->parts[s->back].x, s->parts[s->back].y);
    // Darw top!
    const char leftTop = frameSymbols[0];
    const char rightTop = frameSymbols[1];
    const char leftBottom = frameSymbols[2];
    const char rightBottom = frameSymbols[3];
    const char horisontal = frameSymbols[4];
    const char vertical = frameSymbols[5];
    const char snakeSym = symbols[0];
    const char appleSym = symbols[1];
    const char spaceSym = symbols[2];

    write(STDOUT_FILENO, &leftTop, 1);
    for (int l = 0; l < COLS; ++l)
        write(STDOUT_FILENO, &horisontal, 1);
    write(STDOUT_FILENO, &rightTop, 1);
    write(STDOUT_FILENO, "\n", 1);

    for (int i = 0; i < ROWS; ++i) {
        write(STDOUT_FILENO, &vertical, 1);
        for (int j = 0; j < COLS; ++j) {
            if (isBody(s, j, i)) {
                write(STDOUT_FILENO, &snakeSym, 1);
            } else if (isApple(a, j, i)) {
                write(STDOUT_FILENO, &appleSym, 1);
            } else {
                write(STDOUT_FILENO, &spaceSym, 1);
            }
        }
        write(STDOUT_FILENO, &vertical, 1);
        write(STDOUT_FILENO, "\n", 1);
    }

    write(STDOUT_FILENO, &leftBottom, 1);
    for (int l = 0; l < COLS; ++l)
        write(STDOUT_FILENO, &horisontal, 1);
    write(STDOUT_FILENO, &rightBottom, 1);

}

void enterKeyInptProcessing() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~ICANON;
    term.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void exitKeyInptProcessing() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= ICANON;
    term.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void gameLoop() {
    struct Snake s = {};
    makeHead(&s);
    pushFront(&s);
    pushFront(&s);

    struct Apple a = {};
    genRandomPos(&(a.pos));

    int tfd = -1;
    if (-1 == (tfd = timerfd_create(CLOCK_MONOTONIC, 0))) {
        // printf("timerfd_create error");
        return -1;
    }

    if (-1 == timerfd_settime(tfd, 0, &ts, NULL)) {
        // printf("timerfd_create error");
        return -1;
    }

    fd_set fds;
    draw(&s, &a);
    while (1) {
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        FD_SET(tfd, &fds);
        int ret = select(tfd + 1, &fds, NULL, NULL, NULL);
        if (ret == -1) {
            // printf("error");
            continue;
        }

        if (FD_ISSET(STDIN_FILENO, &fds)) {
            char b = 0;
            read(STDIN_FILENO, &b, sizeof(char));
            // printf("got: \'%d\'\n", b);
            setDir(&s, b);
        }

        if (FD_ISSET(tfd, &fds)) {
            if (isApple(&a, s.parts[s.front].x, s.parts[s.front].y)) {
                pushFront(&s);
                genRandomPos(&(a.pos));
            } else if (isBody(&s, s.parts[s.front].x, s.parts[s.front].y)) {
                system("clear");
                write(STDOUT_FILENO, "\n\n\n\t\t\tGame over!\n\n\n\n", 20);
                exitKeyInptProcessing();
                return;
            } else {
                move(&s);
            }

            draw(&s, &a);
            setDir(&s, s.dir);

            // timer is handled!
            uint64_t shit = 0;
            (void)read(tfd, &shit, sizeof(uint64_t));
        }
    }
}

int main() {

    enterKeyInptProcessing();
    gameLoop();
    exitKeyInptProcessing();

    return 0;
}
