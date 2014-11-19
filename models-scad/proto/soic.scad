/*
OpenSCAD Model:  Small Outline IC

  N  - number of pins
  L  - width, including pads
  P  - pitch
  W  - width of pad
  H  - total height
  T  - toe length
  A  - body width
  B  - body height
  K  - elevation
  padcolor - color of pads
  bodycolor - color of plastic body
  simple - 0 = realistic model, 1 = simplified model

*/

module soic(
	N=14,
	L=6.4,
	P=0.65,
	W=0.25,
	H=1.2,
	T=0.624,
	A=4.4,
	B=5,
	K=0.1,
	padcolor=[0.9,0.9,0.9],
	bodycolor=[0.2,0.2,0.2],
	simple=0
) {

	module soicpadblock(n)	{
		x=0.01;
		color(padcolor) for (i=[0:n-1]) {
			translate ([0,-(n-1)/2*P+i*P+W/2,0])  rotate(90,[1,0,0]) linear_extrude(height=W) polygon(
				[[0,0],[0,0.1],[T-x,0.1],[T+((L-A)/2-T)/2,(H-K)/2+0.05+K],[(L-A)/2+0.05,(H-K)/2+0.05+K],
				 [(L-A)/2+0.05,(H-K)/2-0.05+K],[T+((L-A)/2-T)/2+x,(H-K)/2-0.05+K],[T,0]]
			);
		}
	}

	color(bodycolor) {
		translate([0,0,(H-K)/2+K]) {
			difference() {
			if (simple) {
				cube([A,B,H-K],center=true);
			} else {
				polyhedron(
					[[-A/2+0.2,B/2-0.2,(H-K)/2],[A/2-0.2,B/2-0.2,(H-K)/2],[A/2-0.2,-B/2+0.2,(H-K)/2],[-A/2+0.2,-B/2+0.2,(H-K)/2],
					 [-A/2,B/2,0.1],[A/2,B/2,0.1],[A/2,-B/2,0.1],[-A/2,-B/2,0.1],
					 [-A/2,B/2,-0.1],[A/2,B/2,-0.1],[A/2,-B/2,-0.1],[-A/2,-B/2,-0.1],
					 [-A/2+0.2,B/2-0.2,-(H-K)/2],[A/2-0.2,B/2-0.2,-(H-K)/2],[A/2-0.2,-B/2+0.2,-(H-K)/2],[-A/2+0.2,-B/2+0.2,-(H-K)/2]],
					[
					[0,1,2],[2,3,0],[14,13,12],[12,15,14],
					[0,4,5],[5,1,0],[1,5,6],[6,2,1],[2,6,7],[7,3,2],[3,7,4],[4,0,3],
					[4,8,9],[9,5,4],[5,9,10],[10,6,5],[6,10,11],[11,7,6],[11,8,4],[4,7,11],
					[8,12,13],[13,9,8],[9,13,14],[14,10,9],[10,14,15],[15,11,10],[11,15,12],[12,8,11],
					]
				);
			}
			translate([-A/2+1,B/2-1,(H-K)/2+0.3]) sphere(r=0.5,$fn=30);
			}
		}
	}

	translate([-L/2,0,0]) soicpadblock(N/2);
	translate([L/2,0,0]) rotate(180,[0,0,1]) soicpadblock(N/2);
}
