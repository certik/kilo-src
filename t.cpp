#include "terminal.h"

void render(const Terminal &term, int rows, int cols, int pos)
{
    std::string scr;
    scr.reserve(16*1024);

    scr.append(cursor_off());
    scr.append(move_cursor(1, 1));

    for (int i=1; i <= rows; i++) {
        if (i == pos) {
            scr.append(color(fg::red));
            scr.append(color(bg::gray));
            scr.append(color(style::bold));
        } else {
            scr.append(color(fg::blue));
            scr.append(color(bg::green));
        }
        scr.append(std::to_string(i) + ": item");
        scr.append(color(bg::reset));
        scr.append(color(fg::reset));
        scr.append(color(style::reset));
        if (i < rows) scr.append("\n");
    }

    scr.append(move_cursor(rows / 2, cols / 2));

    scr.append(cursor_on());

    term.write(scr);
}

int main(int argc, char *argv[]) {
    try {
        Terminal term(false);
        if (argc == 1) {
            std::cout << "OK" << std::endl;;
            int rows, cols;
            term.get_term_size(rows, cols);
            std::cout << "Dimension:" << cols << " " << rows << std::endl;
            std::cout << "Press any key:" << std::endl;
            while (true) {
                int key = term.read_key();
                std::cout << "Got:" << key << std::endl;
                if (key == 'q') break;
            }
        } else {
            term.save_screen();
            int rows, cols;
            term.get_term_size(rows, cols);
            int pos = 5;
            bool on = true;
            while (on) {
                render(term, rows, cols, pos);
                int key = term.read_key();
                switch (key) {
                    case Key::ARROW_UP: if (pos > 1) pos--; break;
                    case Key::ARROW_DOWN: if (pos < rows) pos++; break;
                    case 'q':
                    case Key::ESC:
                          on = false; break;
                }
            }
        }
        } catch(...) {
            throw;
        }
    return 0;
}
