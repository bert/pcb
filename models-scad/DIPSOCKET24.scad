/*

DIP socket, 0.600mil

*/

include <proto/dipsocket.scad>

translate ([7.62,-13.97,0]) dipsocket(
	N=24,
	P=2.54,
	TC=15.24,
	W=0.51,
	A=17.78,
	B=30.48,
	H=4.19,
	K=1.01,
	m=1,
	h=3.18,
	d1=1.5,
	d2=1.83,
	rect_pin=false
);