all:
	del main.exe
	ccpsx -O -G 0 -comments-c++ -Xo$80010000 \psx\lib\none3.obj main.c -omain.cpe,main.sym
	cpe2x /ce main.cpe
	del main.cpe
