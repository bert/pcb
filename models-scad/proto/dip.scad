/*
OpenSCAD Model:  DIP package

Parameters:
  N  - Number of pins
  P  - Pitch
  TC - Width
  W  - Width of pin
  T  - Thickness of pin
  A  - Body length
  B  - Body width
  H  - Body height
  K  - Elevation
  h  - Pin length
  pincolor - color of pin
  bodycolor - color of plastic body
  simple - 0 = realistic model, 1 = simplified model
*/

module dip(
	N=14,
	P=2.54,
	TC=7.62,
	W=0.45,
	H=5.33,
	T=0.375,
	A=6.35,
	B=19.2,
	K=0.38,
	h=2.0,
	pincolor=[0.9,0.9,0.9],
	bodycolor=[0.2,0.2,0.2],
	simple=0
) {

	module dippinblock()	{
		q=[0,0.173648178,0.342020143,0.5,0.64278761,0.766044443,0.866025404,0.939692621,0.984807753,1];

		x=0.01;
		t2=T/2;
		hk=(H-K)/2+K;
		dt=1.2*T;
		dt2=dt-T;
		cx=dt-t2;
		cy= hk+t2-dt;
		color(pincolor) for (i=[0:(N/2)-1]) {
			 union () {
					translate ([0,-((N/2)-1)/2*P+i*P+1.5*W,0]) rotate(90,[1,0,0]) linear_extrude(height=3*W) {
					if (simple) {
						polygon([
							[-t2,0], [-t2, hk+t2-dt], 	[dt-t2,hk+t2], [(TC-A)/2,hk+t2],[(TC-A)/2,hk-t2], [dt-t2,hk-t2], [t2,hk+t2-dt], [t2,0]]
						);
					} else {
						 polygon([
							[-t2,0],
							[cx-dt*q[9],dt*q[0]+cy],[cx-dt*q[8],dt*q[1]+cy],[cx-dt*q[7],dt*q[2]+cy],[cx-dt*q[6],dt*q[3]+cy],[cx-dt*q[5],dt*q[4]+cy],
							[cx-dt*q[4],dt*q[5]+cy],[cx-dt*q[3],dt*q[6]+cy],[cx-dt*q[2],dt*q[7]+cy],[cx-dt*q[1],dt*q[8]+cy],[cx-dt*q[0],dt*q[9]+cy],
							[dt-t2,hk+t2],
							[(TC-A)/2,hk+t2],[(TC-A)/2,hk-t2],
							[cx-dt2*q[0],dt2*q[9]+cy],[cx-dt2*q[1],dt2*q[8]+cy],[cx-dt2*q[2],dt2*q[7]+cy],[cx-dt2*q[3],dt2*q[6]+cy],[cx-dt2*q[4],dt2*q[5]+cy],
							[cx-dt2*q[5],dt2*q[4]+cy],[cx-dt2*q[6],dt2*q[3]+cy],[cx-dt2*q[7],dt2*q[2]+cy],[cx-dt2*q[8],dt2*q[1]+cy],[cx-dt2*q[9],dt2*q[0]+cy],
							[t2,0]]
						);
					}
					}
				translate ([0,-((N/2)-1)/2*P+i*P,-1]) cube([T,W,2],center=true);
			}
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
			translate([0,B/2-1,(H-K)/2]) cube([1,2,0.5],center=true);
			}
		}
	}

	translate([-TC/2,0,0]) dippinblock();
	translate([TC/2,0,0]) rotate(180,[0,0,1]) dippinblock();
}
