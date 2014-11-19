/*
OpenSCAD Model:  Axial polarized capacitor, horizontally mounted

  L  - body length
  D  - outer diameter
  M  - lead distance
  d  - lead diameter
  h  - total height (if 0, default is calculated)
  leadcolor - color of leads
  bodycolor - color of plastic body
  capcolor - color of top/bottom cap
  simple - 0 = realistic model, 1 = simplified model

*/

module axial_cp(
	L = 18,
	D = 8,
	M = 24.5,
	d = 0.8,
	h = 0,
	bodycolor = [0, 0, 0.84],
	leadcolor = [0.9, 0.9, 0.9],
	capcolor = [1, 0.8941, 0.4784],
	simple=0
)
{
	q=[0,0.173648178,0.342020143,0.5,0.64278761,0.766044443,0.866025404,0.939692621,0.984807753,1];

	mh = (h!=0)?h:D;
	c2=D/20;

	module axialcpbody() {
		cx=D/2-c2;
		cy=L/2-c2;
		cy3=cy-c2;
		cy4=cy-5*c2;
		rotate(-90,[0,1,0]) rotate_extrude(convexity=10, $fn=36)
		if (simple) {
		 polygon([
			[0,cy+c2-0.2],[cx,cy+c2-0.2],[cx,c2+cy], [D/2,c2+cy], [D/2,cy3-c2], [cx,cy3-c2], [cx,cy4+c2],[D/2,cy4+c2],[D/2,-c2-cy],[cx,-c2-cy],[cx,-cy-c2+0.2],[0,-cy-c2+0.2]
		]);
		} else {
		 polygon([
			[0,cy+c2-0.2],[cx,cy+c2-0.2],
			[cx+c2*q[0],c2*q[9]+cy],[cx+c2*q[1],c2*q[8]+cy],[cx+c2*q[2],c2*q[7]+cy],[cx+c2*q[3],c2*q[6]+cy],[cx+c2*q[4],c2*q[5]+cy],
			[cx+c2*q[5],c2*q[4]+cy],[cx+c2*q[6],c2*q[3]+cy],[cx+c2*q[7],c2*q[2]+cy],[cx+c2*q[8],c2*q[1]+cy],[cx+c2*q[9],c2*q[0]+cy],

			[cx+c2*q[9],-c2*q[0]+cy3],[cx+c2*q[8],-c2*q[1]+cy3],[cx+c2*q[7],-c2*q[2]+cy3],[cx+c2*q[6],-c2*q[3]+cy3],[cx+c2*q[5],-c2*q[4]+cy3],
			[cx+c2*q[4],-c2*q[5]+cy3],[cx+c2*q[3],-c2*q[6]+cy3],[cx+c2*q[2],-c2*q[7]+cy3],[cx+c2*q[1],-c2*q[8]+cy3],[cx+c2*q[0],-c2*q[9]+cy3],

			[cx+c2*q[0],c2*q[9]+cy4],[cx+c2*q[1],c2*q[8]+cy4],[cx+c2*q[2],c2*q[7]+cy4],[cx+c2*q[3],c2*q[6]+cy4],[cx+c2*q[4],c2*q[5]+cy4],
			[cx+c2*q[5],c2*q[4]+cy4],[cx+c2*q[6],c2*q[3]+cy4],[cx+c2*q[7],c2*q[2]+cy4],[cx+c2*q[8],c2*q[1]+cy4],[cx+c2*q[9],c2*q[0]+cy4],

			[cx+c2*q[9],-c2*q[0]-cy],[cx+c2*q[8],-c2*q[1]-cy],[cx+c2*q[7],-c2*q[2]-cy],[cx+c2*q[6],-c2*q[3]-cy],[cx+c2*q[5],-c2*q[4]-cy],
			[cx+c2*q[4],-c2*q[5]-cy],[cx+c2*q[3],-c2*q[6]-cy],[cx+c2*q[2],-c2*q[7]-cy],[cx+c2*q[1],-c2*q[8]-cy],[cx+c2*q[0],-c2*q[9]-cy],
			[cx,-cy-c2+0.2],[0,-cy-c2+0.2]
		]);
		}
	}

	module axialcppin(t,r) {
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
		color (bodycolor) axialcpbody();
		translate([-L/2+0.15,0,0]) color (capcolor) rotate(90,[0,1,0]) cylinder(h=0.3,r=D/2-c2,center=true,$fn=36);
		translate([L/2-0.15,0,0]) color (capcolor) rotate(90,[0,1,0]) cylinder(h=0.3,r=D/2-c2,center=true,$fn=36);
	}

	color(leadcolor) translate([0,0,D/2+(mh-D)-1.5*d]) union() {
		axialcppin([M/2,0,0],0);
		axialcppin([-M/2,0,0],180);
	}
}
