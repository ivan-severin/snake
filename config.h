#include <time.h>

// Colomns
#define COLS 50
// Rows
#define ROWS 25

// symbols: body, space, apple
// static const char* symbols = "#@*";
static const char* symbols = "#@ ";

// [left top, right top, left bottom, right bottom, left bottom, horisontal, vertical]
// static const char* frameSymbols = "┌┐└┘─│";
static const char* frameSymbols = "++++-|";

// Speed
static const struct itimerspec ts = {
    .it_interval.tv_sec = 0,
    .it_interval.tv_nsec = 100000000,
    .it_value.tv_sec = 0,
    .it_value.tv_nsec = 100000000,
};
