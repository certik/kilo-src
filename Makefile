all: t term

t: t.cpp terminal.h
	$(CXX) t.cpp -o t -Wall -Wextra --std=c++14

term: term.cpp terminal.h
	$(CXX) term.cpp -o term -Wall -Wextra --std=c++14
