#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdexcept>
#include <iostream>
#include <memory>
#include <termios.h>
#include <unistd.h>

#define CTRL_KEY(k) ((k) & 0x1f)

enum class style {
    reset     = 0,
    bold      = 1,
    dim       = 2,
    italic    = 3,
    underline = 4,
    blink     = 5,
    rblink    = 6,
    reversed  = 7,
    conceal   = 8,
    crossed   = 9
};

enum class fg {
    black   = 30,
    red     = 31,
    green   = 32,
    yellow  = 33,
    blue    = 34,
    magenta = 35,
    cyan    = 36,
    gray    = 37,
    reset   = 39
};

enum class bg {
    black   = 40,
    red     = 41,
    green   = 42,
    yellow  = 43,
    blue    = 44,
    magenta = 45,
    cyan    = 46,
    gray    = 47,
    reset   = 49
};

enum class fgB {
    black   = 90,
    red     = 91,
    green   = 92,
    yellow  = 93,
    blue    = 94,
    magenta = 95,
    cyan    = 96,
    gray    = 97
};

enum class bgB {
    black   = 100,
    red     = 101,
    green   = 102,
    yellow  = 103,
    blue    = 104,
    magenta = 105,
    cyan    = 106,
    gray    = 107
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

// If an attempt is made to move the cursor to the right of the right margin, the cursor stops at the right margin.
std::string move_cursor_right(int col)
{
    return "\x1b[" + std::to_string(col) + "C";
}

// If an attempt is made to move the cursor below the bottom margin, the cursor stops at the bottom margin.
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
  ARROW_LEFT,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL,
  HOME,
  END,
  PAGE_UP,
  PAGE_DOWN,
  ESC
};

class Terminal {
private:
    struct termios orig_termios;
public:
    Terminal() {
        if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
            throw std::runtime_error("tcgetattr() failed");
        }

        // Put terminal in raw mode
        struct termios raw = orig_termios;
        raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        raw.c_oflag &= ~(OPOST);
        raw.c_cflag |= (CS8);
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;

        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
            throw std::runtime_error("tcsetattr() failed");
        }

        write("\0337"); // save current cursor position
        write("\033[?47h"); // save screen
    }

    ~Terminal() {
        write("\033[?47l"); // restore screen
        write("\0338"); // restore current cursor position
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
            std::cout << "tcsetattr() failed in destructor, terminating."
                << std::endl;
            exit(1);
        }
    }

    void write(const std::string &s) const {
        if (::write(STDOUT_FILENO, s.c_str(), s.size()) != (int) s.size()) {
            throw std::runtime_error("write() failed");
        };
    }

    // Returns true if a character is read, otherwise immediately returns false
    bool read_raw(char *s) const {
        int nread = read(STDIN_FILENO, s, 1);
        if (nread == -1 && errno != EAGAIN) {
            throw std::runtime_error("read() failed");
        }
        return (nread == 1);
    }

    // Waits for a key press, translates escape codes
    int read_key() const {
        int key;
        while ((key = read_key0()) == 0) {}
        return key;
    }

    // If there was a key press, returns the translated key from escape codes,
    // otherwise returns 0.
    int read_key0() const {
      char c;
      if (!read_raw(&c)) return 0;

      if (c == '\x1b') {
        char seq[3];

        if (!read_raw(&seq[0])) return Key::ESC;
        if (!read_raw(&seq[1])) return Key::ESC;

        if (seq[0] == '[') {
          if (seq[1] >= '0' && seq[1] <= '9') {
            if (!read_raw(&seq[2])) return Key::ESC;
            if (seq[2] == '~') {
              switch (seq[1]) {
                case '1': return Key::HOME;
                case '3': return Key::DEL;
                case '4': return Key::END;
                case '5': return Key::PAGE_UP;
                case '6': return Key::PAGE_DOWN;
                case '7': return Key::HOME;
                case '8': return Key::END;
              }
            }
          } else {
            switch (seq[1]) {
              case 'A': return Key::ARROW_UP;
              case 'B': return Key::ARROW_DOWN;
              case 'C': return Key::ARROW_RIGHT;
              case 'D': return Key::ARROW_LEFT;
              case 'H': return Key::HOME;
              case 'F': return Key::END;
            }
          }
        } else if (seq[0] == 'O') {
          switch (seq[1]) {
            case 'H': return Key::HOME;
            case 'F': return Key::END;
          }
        }

        return Key::ESC;
      } else {
        switch (c) {
          case '\r': return Key::ENTER;
          case 127: return Key::BACKSPACE;
        }
        return c;
      }
    }

    void get_term_size(int &rows, int &cols) const {
        char buf[32];
        unsigned int i = 0;
        write(move_cursor_right(999) + move_cursor_down(999)
                + cursor_position_report());
        while (i < sizeof(buf) - 1) {
            while (!read_raw(&buf[i])) {};
            if (buf[i] == 'R') break;
            i++;
        }
        buf[i] = '\0';
        if (i < 7) {
            throw std::runtime_error("get_term_size(): too short response");
        }
        if (buf[0] != '\x1b' || buf[1] != '[') {
            throw std::runtime_error("get_term_size(): Invalid response");
        }
        if (sscanf(&buf[2], "%d;%d", &rows, &cols) != 2) {
            throw std::runtime_error("get_term_size(): Invalid response");
        }
    }
};

#endif // TERMINAL_H
