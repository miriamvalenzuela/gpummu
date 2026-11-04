# Name: Miriam Valenzuela, Jesus Hernandez
# Red ID: 131786119, 131481477

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -g3

OBJS = main.o pagetable.o log.o tracereader.o translationlookasidebuffer.o

PROGRAM = gpummu

$(PROGRAM): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(PROGRAM) $(OBJS)

main.o: main.cpp pagetable.h translationlookasidebuffer.h log.h
	$(CXX) $(CXXFLAGS) -c main.cpp

pagetable.o: pagetable.cpp pagetable.h
	$(CXX) $(CXXFLAGS) -c pagetable.cpp

log.o: log.cpp log.h
	$(CXX) $(CXXFLAGS) -c log.cpp

tracereader.o: tracereader.cpp tracereader.h
	$(CXX) $(CXXFLAGS) -c tracereader.cpp

translationlookasidebuffer.o: translationlookasidebuffer.cpp translationlookasidebuffer.h
	$(CXX) $(CXXFLAGS) -c translationlookasidebuffer.cpp

clean:
	rm -f *.o $(PROGRAM)