#include "terminal.h"

int main() {
    try {
        Terminal term(false);
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
        } catch(...) {
            throw;
        }
    return 0;
}
