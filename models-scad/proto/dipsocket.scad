/*
OpenSCAD Model:  DIP socket

Parameters:
  N  - Number of pins
  P  - Pitch
  TC - Width
  W  - Width/diasmeter of pin
  A  - Body width
  B  - Body length
  H  - Body height
  K  - Elevation
  m  - number of crossbars
  h  - Pin length (for round pin type)
       or edge extension (for rectangular pin type)
  E  - Edge height
  T  - Thickness of pin
  d1 - diameter of middle part of pin
  d2 - diameter of top part of pin
  pincolor - color of pin
  bodycolor - color of plastic body
  simple - 0 = realistic model, 1 = simplified model
*/

module dipsocket(
	N=14,
	P=2.54,
	TC=7.62,
	W=0.7,
	A=10.16,
	B=20.19,
	H=5.08,
	K=0.5,
	m=0,
	h=2.0,
	E=1,
	T=0.3,
	d1=1.2,
	d2=1.6,
	rect_pin=true,
	pincolor=[1,0.85,0.24],
	bodycolor=[0.2,0.2,0.2],
	simple=0
) {

	module dipspinblock()	{
		color(pincolor) for (i=[0:(N/2)-1]) {
			 translate ([0,-((N/2)-1)/2*P+i*P,0]) {
          if (rect_pin) {
				 union () {
					translate([-0.2,0,H-E+0.2]) cube([0.3,2*W,0.5],center=true);
					translate([0.2,0,H-E]) cube([0.3,2*W,0.2],center=true);
					translate([0,0,-h/2+0.05]) cube([T,W,h+0.1],center=true);
				 }
			 } else {
				 union () {
					translate([0,0,-h/2+0.05]) cylinder(r=W/2,h=h+0.1,center=true,$fn=36);
					translate([0,0,K/2+0.05]) cylinder(r=d1/2,h=K+0.1,center=true,$fn=36);
					difference () {
						translate([0,0,H]) cylinder(r=d2/2,h=0.1,center=true,$fn=36);
						translate([0,0,H]) cylinder(r=W/2,h=0.2,center=true,$fn=36);
					}
				 }
          }
			 }
		}
	}

	ew=(A-TC)/2-0.2;
	bw=A-TC;
   BH=(rect_pin)?H-E:H;
   BK=(rect_pin)?0:K;
   nn=(B-bw)/(m+1);
	YY=-(m - 1)/2 * nn;
	color(bodycolor) {
		  union () {
					if (rect_pin) {
						translate([-A/2+ew/2,0,H/2]) cube([ew,B,H],center=true);
						translate([A/2-ew/2,0,H/2]) cube([ew,B,H],center=true);
					}
					translate([-TC/2,0,(BH+BK)/2]) cube([bw,B,BH-BK],center=true);
					translate([TC/2,0,(BH+BK)/2]) cube([bw,B,BH-BK],center=true);
					translate([0,B/2-bw/2,(BH+K)/2]) cube([A,bw,BH-K],center=true);
					translate([0,-B/2+bw/2,(BH+K)/2]) cube([A,bw,BH-K],center=true);
					if (m>0) {
					for (i=[0:m-1]) {
						translate([0,YY+i*nn,(BH+K)/2]) cube([A,bw,BH-K],center=true);
					}
			}
		}
	}

	translate([-TC/2,0,0]) dipspinblock();
	translate([TC/2,0,0]) rotate(180,[0,0,1]) dipspinblock();
}
