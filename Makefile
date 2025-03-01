CXXFLAGS = -O3
CXX = g++

build: main.o HTMLFile.o
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f *.o build

winclean:
	del *.o build.exe