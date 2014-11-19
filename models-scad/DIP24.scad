/*

DIP 24, 0.600mil

*/

include <proto/dip.scad>

translate ([7.62,-13.97,0]) dip(
	N=24,
	P=2.54,
	TC=15.24,
	W=0.45,
	T=0.32,
	A=13.525,
	B=31.05,
	H=6.35,
	K=0.38,
	h=5.08
);