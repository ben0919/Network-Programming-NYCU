all: npshell.cpp utils.o execute.o command.o
	g++ -o npshell npshell.cpp utils.o execute.o command.o

utils.o: utils.cpp
	g++ -c utils.cpp

execute.o: execute.cpp
	g++ -c execute.cpp

command.o: command.cpp
	g++ -c command.cpp

clean:
	rm -rf *.o
	rm -rf *.txt
	rm -rf npshell
