/*

DIP 32, 0.600mil

*/

include <proto/dip.scad>

translate ([7.62,-19.05,0]) dip(
	N=32,
	P=2.54,
	TC=15.24,
	W=0.45,
	T=0.32,
	A=13.525,
	B=40.8,
	H=6.35,
	K=0.38,
	h=5.08
);