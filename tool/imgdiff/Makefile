
CXXFLAGS=-std=c++11 `pkg-config --cflags --libs elementary`

imgdiffEvas.exe: imgdiffEvas.cpp
	g++ imgdiffEvas.cpp -o imgdiffEvas.exe $(CXXFLAGS)

imgdiff.exe: imgdiff.cpp
	g++ imgdiff.cpp -o imgdiff.exe $(CXXFLAGS) -lpng

clean:
	rm -f imgdiff.exe imgdiffEvas.exe
