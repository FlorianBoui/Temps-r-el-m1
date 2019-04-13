bench:
	g++ -Wall -std=c++17 partie1.cpp -o partie1 `pkg-config --cflags --libs opencv` -lpthread
interact:
	g++ -Wall -std=c++17 partie2.cpp -o partie2 `pkg-config --cflags --libs opencv` -lpthread
clean:
	rm partie1
	rm partie2
