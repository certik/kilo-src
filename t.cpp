#include "terminal.h"

int main() {
  // We must put all code in try/catch block, otherwise destructors are not
  // being called when exception happens and the terminal is not put into
  // correct state.
  try {
      Terminal term;
      std::cout << "OK" << "\r\n";
      int rows, cols;
      term.get_term_size(rows, cols);
      std::cout << "Dimension:" << cols << " " << rows << "\r\n";
  } catch(...) {
      throw;
  }
  return 0;
}
