/*

DIP socket, 0.900mil

*/

include <proto/dipsocket.scad>

translate ([11.43,-39.37,0]) dipsocket(
	N=64,
	P=2.54,
	TC=22.86,
	W=0.51,
	A=25.48,
	B=81.28,
	H=4.19,
	K=1.01,
	m=2,
	h=3.18,
	d1=1.5,
	d2=1.83,
	rect_pin=false
);