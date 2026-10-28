# Name: Miriam Valenzuela
# Red ID: 131786119

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -g3

OBJS = main.o pagetable.o log.o tracereader.o

PROGRAM = gpummu

$(PROGRAM): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(PROGRAM) $(OBJS)

main.o: main.cpp pagetable.h log.h
	$(CXX) $(CXXFLAGS) -c main.cpp

pagetable.o: pagetable.cpp pagetable.h
	$(CXX) $(CXXFLAGS) -c pagetable.cpp

log.o: log.cpp log.h
	$(CXX) $(CXXFLAGS) -c log.cpp

tracereader.o: tracereader.cpp tracereader.h
	$(CXX) $(CXXFLAGS) -c tracereader.cpp

clean:
	rm -f *.o $(PROGRAM)