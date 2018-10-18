a.out:
	g++ --std=c++17 -pthread -O3 -g main.cpp

clean:
	-rm a.out
	-rm *.pgm

#draw: clean a.out
#	./a.out 1000 > out.pgm
#	xdg-open out.pgm

video: a.out
	./a.out 1000
	./compile_video.sh

.PHONY = clean draw
