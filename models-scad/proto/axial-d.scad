/*
OpenSCAD Model:  Axial diode, horizontally mounted

  L  - body length
  D  - outer diameter
  M  - lead distance
  d  - lead diameter
  SW - stripe width (if 0, default is calculated)
  SP - hirst stripe offset (if 0, default is calculated)
  h  - total height (if 0, default is calculated)
  leadcolor - color of leads
  bodycolor - color of plastic body
  stripecolor - color of 1st stripe
*/

module axial_d(
	L = 3.8,
	D = 1.8,
	M = 6.5,
	d = 0.5,
	SW = 0,
	SP = 0,
	h = 0,
	bodycolor = [0.1, 0.1, 0.1],
	leadcolor = [0.9, 0.9, 0.9],
	stripecolor = [1, 1, 1],
	simple=0
)
{
	q=[0,0.173648178,0.342020143,0.5,0.64278761,0.766044443,0.866025404,0.939692621,0.984807753,1];

	mSW = (SW!=0)?SW:0.7;
	mSP = (SP!=0)?SP:0.3;
	mh = (h!=0)?h:D;


	module axialrpin(t,r) {
		translate(t) rotate(r,[0,0,1]) union() {
			translate([-1.5*d,0,0]) rotate(90,[1,0,0]) intersection() {
				rotate_extrude(convexity=10, $fn=36) translate([1.5*d,0,0]) circle(r=d/2,$fn=36);
				translate([0,0,-d/2]) cube([1.5*d+d/2,1.5*d+d/2,d]);
			}
			rotate(180,[1,0,0]) cylinder(r=d/2,h=(D/2)-1.5*d+2,$fn=36);
			translate([-1.5*d,0,1.5*d]) rotate(-90,[0,1,0]) cylinder(r=d/2,h=(M-L)/2-1.5*d,$fn=36);
		}
	}

	translate([0,0,D/2+(mh-D)]) union () {
		color (bodycolor) rotate(90,[0,1,0]) cylinder(h=L,r=D/2,center=true,$fn=36);
		color (stripecolor) translate([-L/2+mSP+mSW/2,0,0]) rotate(90,[0,1,0]) cylinder(h=mSW,r=D/2+0.02,center=true,$fn=36);
	}

	color(leadcolor) translate([0,0,D/2+(mh-D)-1.5*d]) union() {
		axialrpin([M/2,0,0],0);
		axialrpin([-M/2,0,0],180);
	}



}
