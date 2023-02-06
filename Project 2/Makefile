all: np_simple.cpp np_single_proc.cpp np_multi_proc.cpp npshell.cpp obj/utils.o obj/execute.o obj/command.o obj/client.o obj/server_utils.o obj/server_execute.o
	g++ -o np_simple np_simple.cpp npshell.cpp obj/utils.o obj/execute.o obj/command.o
	g++ -o np_single_proc np_single_proc.cpp obj/utils.o obj/command.o obj/client.o obj/server_utils.o  obj/server_execute.o
	g++ -o np_multi_proc -lpthread np_multi_proc.cpp obj/utils.o obj/command.o

obj/utils.o: utils.cpp
	mkdir obj
	g++ -c utils.cpp -o obj/utils.o

obj/execute.o: execute.cpp
	g++ -c execute.cpp -o obj/execute.o

obj/command.o: command.cpp
	g++ -c command.cpp -o obj/command.o

obj/client.o: client.cpp
	g++ -c client.cpp -o obj/client.o

obj/server_utils.o: server_utils.cpp
	g++ -c server_utils.cpp -o obj/server_utils.o

obj/server_execute.o: server_execute.cpp
	g++ -c server_execute.cpp -o obj/server_execute.o

clean:
	rm -rf obj
	rm -rf npshell np_simple np_single_proc np_multi_proc