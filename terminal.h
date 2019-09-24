#ifndef TERMINAL_H
#define TERMINAL_H

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#ifdef _WIN32

#include <windows.h>

#else

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#endif

#define CTRL_KEY(k) ((k)&0x1f)

enum class style {
    reset = 0,
    bold = 1,
    dim = 2,
    italic = 3,
    underline = 4,
    blink = 5,
    rblink = 6,
    reversed = 7,
    conceal = 8,
    crossed = 9
};

enum class fg {
    black = 30,
    red = 31,
    green = 32,
    yellow = 33,
    blue = 34,
    magenta = 35,
    cyan = 36,
    gray = 37,
    reset = 39
};

enum class bg {
    black = 40,
    red = 41,
    green = 42,
    yellow = 43,
    blue = 44,
    magenta = 45,
    cyan = 46,
    gray = 47,
    reset = 49
};

enum class fgB {
    black = 90,
    red = 91,
    green = 92,
    yellow = 93,
    blue = 94,
    magenta = 95,
    cyan = 96,
    gray = 97
};

enum class bgB {
    black = 100,
    red = 101,
    green = 102,
    yellow = 103,
    blue = 104,
    magenta = 105,
    cyan = 106,
    gray = 107
};

template <typename T>
std::string color(T const value)
{
    return "\033[" + std::to_string(static_cast<int>(value)) + "m";
}

std::string cursor_off()
{
    return "\x1b[?25l";
}

std::string cursor_on()
{
    return "\x1b[?25h";
}

// If an attempt is made to move the cursor out of the window, the result is
// undefined.
std::string move_cursor(int row, int col)
{
    return "\x1b[" + std::to_string(row) + ";" + std::to_string(col) + "H";
}

// If an attempt is made to move the cursor to the right of the right margin,
// the cursor stops at the right margin.
std::string move_cursor_right(int col)
{
    return "\x1b[" + std::to_string(col) + "C";
}

// If an attempt is made to move the cursor below the bottom margin, the cursor
// stops at the bottom margin.
std::string move_cursor_down(int row)
{
    return "\x1b[" + std::to_string(row) + "B";
}

std::string cursor_position_report()
{
    return "\x1b[6n";
}

std::string erase_to_eol()
{
    return "\x1b[K";
}

enum Key {
    BACKSPACE = 1000,
    ENTER,
    TAB,
    ARROW_LEFT,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    NUMERIC_5,
    DEL,
    HOME,
    INSERT,
    END,
    PAGE_UP,
    PAGE_DOWN,
    ESC,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
};

/* Note: the code that uses Terminal must be inside try/catch block, otherwise
 * the destructors will not be called when an exception happens and the
 * terminal will not be left in a good state. Terminal uses exceptions when
 * something goes wrong.
 */
class Terminal {
private:
#ifdef _WIN32
    HANDLE hout;
    HANDLE hin;
    DWORD dwOriginalOutMode;
    DWORD dwOriginalInMode;
#else
    struct termios orig_termios;
#endif
    bool restore_screen;

public:
    Terminal(bool disable_ctrl_c=true)
        : restore_screen{ false }
    {
#ifdef _WIN32
        hout = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hout == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("GetStdHandle(STD_OUTPUT_HANDLE) failed");
        }
        hin = GetStdHandle(STD_INPUT_HANDLE);
        if (hin == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("GetStdHandle(STD_INPUT_HANDLE) failed");
        }

        if (!GetConsoleMode(hout, &dwOriginalOutMode)) {
            throw std::runtime_error("GetConsoleMode() failed");
        }
        if (!GetConsoleMode(hin, &dwOriginalInMode)) {
            throw std::runtime_error("GetConsoleMode() failed");
        }

        DWORD dwRequestedOutModes = ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
        DWORD dwRequestedInModes = ENABLE_VIRTUAL_TERMINAL_INPUT;

        DWORD dwOutMode = dwOriginalOutMode | dwRequestedOutModes;
        if (!SetConsoleMode(hout, dwOutMode)) {
            throw std::runtime_error("SetConsoleMode() failed");
        }
        DWORD dwInMode = dwOriginalInMode | dwRequestedInModes;
        if (!SetConsoleMode(hin, dwInMode)) {
            throw std::runtime_error("SetConsoleMode() failed");
        }
#else
        if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
            throw std::runtime_error("tcgetattr() failed");
        }

        // Put terminal in raw mode
        struct termios raw = orig_termios;
        raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        // This disables output post-processing, requiring explicit \r\n. We
        // keep it enabled, so that in C++, one can still just use std::endl
        // for EOL instead of "\r\n".
        //raw.c_oflag &= ~(OPOST);
        raw.c_cflag |= (CS8);
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);
        if (disable_ctrl_c) {
            raw.c_lflag &= ~(ISIG);
        }
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;

        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
            throw std::runtime_error("tcsetattr() failed");
        }
#endif
    }

    ~Terminal() noexcept(false)
    {
        if (restore_screen) {
            write("\033[?47l"); // restore screen
            write("\0338"); // restore current cursor position
        }
#ifdef _WIN32
        if (!SetConsoleMode(hout, dwOriginalOutMode)) {
            throw std::runtime_error("SetConsoleMode() failed");
        }
        if (!SetConsoleMode(hin, dwOriginalInMode)) {
            throw std::runtime_error("SetConsoleMode() failed");
        }
#else
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
            std::cout << "tcsetattr() failed in destructor, terminating."
                      << std::endl;
            exit(1);
        }
#endif
    }

    void save_screen()
    {
        restore_screen = true;
        write("\0337"); // save current cursor position
        write("\033[?47h"); // save screen
    }

    void write(const std::string& s) const
    {
        std::cout << s << std::flush;
    }

    // Returns true if a character is read, otherwise immediately returns false
    bool read_raw(char* s) const
    {
#ifdef _WIN32
        char buf[1];
        DWORD nread;
        if (!ReadFile(hin, buf, 1, &nread, nullptr)) {
            throw std::runtime_error("ReadFile() failed");
        }
        if (nread == 1) {
            *s = buf[0];
            return true;
        }
        return false;
#else
        int nread = read(STDIN_FILENO, s, 1);
        if (nread == -1 && errno != EAGAIN) {
            throw std::runtime_error("read() failed");
        }
        return (nread == 1);
#endif
    }

    // Waits for a key press, translates escape codes
    int read_key() const
    {
        int key;
        while ((key = read_key0()) == 0) {
        }
        return key;
    }

    // If there was a key press, returns the translated key from escape codes,
    // otherwise returns 0. If the escape code is not supported, returns a
    // negative number.
    int read_key0() const
    {
        char c;
        if (!read_raw(&c))
            return 0;

        if (c == '\x1b') {
            char seq[4];

            if (!read_raw(&seq[0]))
                return Key::ESC;
            if (!read_raw(&seq[1])) {
                return -1;
            }

            if (seq[0] == '[') {
                if (seq[1] >= '0' && seq[1] <= '9') {
                    if (!read_raw(&seq[2])) {
                        return -2;
                    }
                    if (seq[2] == '~') {
                        switch (seq[1]) {
                        case '1':
                            return Key::HOME;
                        case '2':
                            return Key::INSERT;
                        case '3':
                            return Key::DEL;
                        case '4':
                            return Key::END;
                        case '5':
                            return Key::PAGE_UP;
                        case '6':
                            return Key::PAGE_DOWN;
                        case '7':
                            return Key::HOME;
                        case '8':
                            return Key::END;
                        }
                    } else {
                        if (seq[2] >= '0' && seq[2] <= '9') {
                            if (!read_raw(&seq[3])) {
                                // Invalid escape code
                                return -3;
                            }
                            if (seq[3] == '~') {
                                if (seq[1] == '1') {
                                    switch (seq[2]) {
                                    case '5':
                                        return Key::F5;
                                    case '7':
                                        return Key::F6;
                                    case '8':
                                        return Key::F7;
                                    case '9':
                                        return Key::F8;
                                    }
                                } else if (seq[1] == '2') {
                                    switch (seq[2]) {
                                    case '0':
                                        return Key::F9;
                                    case '1':
                                        return Key::F10;
                                    case '3':
                                        return Key::F11;
                                    case '4':
                                        return Key::F12;
                                    }
                                }
                            }
                        }
                    }
                } else {
                    switch (seq[1]) {
                    case 'A':
                        return Key::ARROW_UP;
                    case 'B':
                        return Key::ARROW_DOWN;
                    case 'C':
                        return Key::ARROW_RIGHT;
                    case 'D':
                        return Key::ARROW_LEFT;
                    case 'E':
                        return Key::NUMERIC_5;
                    case 'H':
                        return Key::HOME;
                    case 'F':
                        return Key::END;
                    }
                }
            } else if (seq[0] == 'O') {
                switch (seq[1]) {
                case 'F':
                    return Key::END;
                case 'H':
                    return Key::HOME;
                case 'P':
                    return Key::F1;
                case 'Q':
                    return Key::F2;
                case 'R':
                    return Key::F3;
                case 'S':
                    return Key::F4;
                }
            }

            //std::cout << "Unsupported escape sequence:" << std::endl;
            //std::cout << seq[0] << seq[1] << seq[2] << seq[3] << std::endl;
            return -4;
        } else {
            switch (c) {
            case 9:
                return Key::TAB;
            case 13:
                return Key::ENTER;
            case 127:
                return Key::BACKSPACE;
            }
            return c;
        }
    }

    void get_cursor_position(int& rows, int& cols) const
    {
        char buf[32];
        unsigned int i = 0;
        write(cursor_position_report());
        while (i < sizeof(buf) - 1) {
            while (!read_raw(&buf[i])) {
            };
            if (buf[i] == 'R')
                break;
            i++;
        }
        buf[i] = '\0';
        if (i < 5) {
            throw std::runtime_error("get_cursor_position(): too short response");
        }
        if (buf[0] != '\x1b' || buf[1] != '[') {
            throw std::runtime_error("get_cursor_position(): Invalid response");
        }
        if (sscanf(&buf[2], "%d;%d", &rows, &cols) != 2) {
            throw std::runtime_error("get_cursor_position(): Invalid response");
        }
    }

    // This function takes about 23ms, so it should only be used as a fallback
    void get_term_size_slow(int& rows, int& cols) const
    {
        struct CursorOff {
            const Terminal& term;
            CursorOff(const Terminal& term)
                : term{ term }
            {
                term.write(cursor_off());
            }
            ~CursorOff()
            {
                term.write(cursor_on());
            }
        };
        CursorOff cursor_off(*this);
        int old_row, old_col;
        get_cursor_position(old_row, old_col);
        write(move_cursor_right(999) + move_cursor_down(999));
        get_cursor_position(rows, cols);
        write(move_cursor(old_row, old_col));
    }

    // This function is immediate if ioctl succeeds, otherwise falls back on
    // the slow version
    void get_term_size(int& rows, int& cols) const
    {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO inf;
        GetConsoleScreenBufferInfo(hout, &inf);
        cols = inf.srWindow.Right - inf.srWindow.Left + 1;
        rows = inf.srWindow.Bottom - inf.srWindow.Top + 1;
#else
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
            get_term_size_slow(rows, cols);
        } else {
            cols = ws.ws_col;
            rows = ws.ws_row;
        }
#endif
    }
};

#endif // TERMINAL_H
