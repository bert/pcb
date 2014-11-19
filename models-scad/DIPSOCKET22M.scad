/*

DIP socket, 0.400mil

*/

include <proto/dipsocket.scad>

translate ([5.08,-12.7,0]) dipsocket(
	N=22,
	P=2.54,
	TC=10.16,
	W=0.51,
	A=12.70,
	B=27.54,
	H=4.19,
	K=1.01,
	m=1,
	h=3.18,
	d1=1.5,
	d2=1.83,
	rect_pin=false
);