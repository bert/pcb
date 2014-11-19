/*
OpenSCAD Model:  Axial resistor, horizontally mounted

  L  - body length
  D  - outer diameter
  M  - lead distance
  d  - lead diameter
  H  - inner diameter (if 0, default is calculated)
  C  - cap width (if 0, default is calculated)
  SW - stripe width (if 0, default is calculated)
  SD - stripe distance (if 0, default is calculated)
  SP - hirst stripe offset (if 0, default is calculated)
  h  - total height (if 0, default is calculated)
  leadcolor - color of leads
  bodycolor - color of plastic body
  stripecolor1 - color of 1st stripe
  stripecolor2 - color of 2nd stripe
  stripecolor3 - color of 3rd stripe
  stripecolor4 - color of 4th stripe
  simple - 0 = realistic model, 1 = simplified model

*/

module axial_r(
	L = 6.8,
	D = 2.5,
	M = 9.9,
	d = 0.6,
	H = 0,
	C = 0,
	SW = 0,
	SD = 0,
	SP = 0,
	h = 0,
	bodycolor = [1, 0.8941, 0.4784],
	leadcolor = [0.9, 0.9, 0.9],
	stripecolor1="",
	stripecolor2=[0,1,0],
	stripecolor3=[0,0,1],
	stripecolor4=[1, 0.85, 0.24],
	simple=0
)
{
	q=[0,0.173648178,0.342020143,0.5,0.64278761,0.766044443,0.866025404,0.939692621,0.984807753,1];

	mH = (H!=0)?H:0.9*D;
	mC = (C!=0)?C:0.2 * L;
	mSW = (SW!=0)?SW:( L - 2 * mC ) / 10;
	mSD = (SD!=0)?SD:( L - 2 * mC ) / 4;
	mSP = (SP!=0)?SP:-( mSD + mSD / 2. );
	mh = (h!=0)?h:D;

	module axialrcap(t) {
		c2=(D-mH);
		cx=D/2-c2;
		cy=mC/2-c2;
		translate(t) rotate(90,[0,1,0]) rotate_extrude(convexity=10, $fn=36) polygon([
			[0,cy+c2],
			[cx+c2*q[0],c2*q[9]+cy],[cx+c2*q[1],c2*q[8]+cy],[cx+c2*q[2],c2*q[7]+cy],[cx+c2*q[3],c2*q[6]+cy],[cx+c2*q[4],c2*q[5]+cy],
			[cx+c2*q[5],c2*q[4]+cy],[cx+c2*q[6],c2*q[3]+cy],[cx+c2*q[7],c2*q[2]+cy],[cx+c2*q[8],c2*q[1]+cy],[cx+c2*q[9],c2*q[0]+cy],

			[cx+c2*q[9],-c2*q[0]-cy],[cx+c2*q[8],-c2*q[1]-cy],[cx+c2*q[7],-c2*q[2]-cy],[cx+c2*q[6],-c2*q[3]-cy],[cx+c2*q[5],-c2*q[4]-cy],
			[cx+c2*q[4],-c2*q[5]-cy],[cx+c2*q[3],-c2*q[6]-cy],[cx+c2*q[2],-c2*q[7]-cy],[cx+c2*q[1],-c2*q[8]-cy],[cx+c2*q[0],-c2*q[9]-cy],
			[0,-cy-c2]
		]);
	}

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
		color (bodycolor) axialrcap([-L/2+mC/2,0,0]);
		color (bodycolor) axialrcap([L/2-mC/2,0,0]);
		color (bodycolor) rotate(90,[0,1,0]) cylinder(h=L-2*mC+0.2,r=mH/2,center=true,$fn=36);
		if (stripecolor1 != "" ) {
			color (stripecolor1) translate([mSP,0,0]) rotate(90,[0,1,0]) cylinder(h=mSW,r=mH/2+0.01,center=true,$fn=36);
			color (stripecolor2) translate([mSP+mSD,0,0]) rotate(90,[0,1,0]) cylinder(h=mSW,r=mH/2+0.01,center=true,$fn=36);
			color (stripecolor3) translate([mSP+2*mSD,0,0]) rotate(90,[0,1,0]) cylinder(h=mSW,r=mH/2+0.01,center=true,$fn=36);
			color (stripecolor4) translate([mSP+3*mSD,0,0]) rotate(90,[0,1,0]) cylinder(h=mSW,r=mH/2+0.01,center=true,$fn=36);
		}
	}

	color(leadcolor) translate([0,0,D/2+(mh-D)-1.5*d]) union() {
		axialrpin([M/2,0,0],0);
		axialrpin([-M/2,0,0],180);
	}



}
