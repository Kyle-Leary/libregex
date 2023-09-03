#ifndef ANSI_H
#define ANSI_H

// ANSI Escape Codes for Text Formatting

// Clear the screen
#define ANSI_CLEAR "\033[2J"

// Reset text formatting to the terminal's default
#define ANSI_RESET "\033[0m"

// Foreground Colors
#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN "\033[36m"
#define ANSI_WHITE "\033[37m"
#define ANSI_BLACK "\033[30m"

// Background Colors
#define ANSI_BG_RED "\033[41m"
#define ANSI_BG_GREEN "\033[42m"
#define ANSI_BG_YELLOW "\033[43m"
#define ANSI_BG_BLUE "\033[44m"
#define ANSI_BG_MAGENTA "\033[45m"
#define ANSI_BG_CYAN "\033[46m"
#define ANSI_BG_WHITE "\033[47m"
#define ANSI_BG_BLACK "\033[40m"

#endif // ANSI_H
