/*
OpenSCAD Model:  LCD module

Parameters:
  W   - module width
  H   - module height
  t   - DPS thickness
  BW  - display body width
  BH  - display body height
  DW  - display area width
  DH  - display area height
  th  - total height
  dh  - distance from module botom  to top of DPS
  cpd - corner hole copper diameter
  chd - corner hole diameter
  cx  - horizontal distance of corner holes
  cy  - vertical distance of corner holes
  px  - distance of 1st pad center from left
  py  - distance of 1st pad center from yop
  ppd - connection pad diameter
  phd - connection pad hole diameter
  pdt - connection pads pitch
  n   - number of connection pads
*/

module lcd_module (
		W=80,
		H=36,
		t=1.76,
		BW=71.2,
		BH=26.2,
		DW=66,
		DH=16,
		th=8.9,
		dh=4.8,
		cpd=5,
		chd=2.5,
		cx=75,
		cy=31,
		px=8,
		py=2,
		ppd=2,
		phd=1,
		pdt=2.54,
		n=16
	) {
	module cpad(tr) {
		color ([1,1,0]) translate (tr) difference () {
			cylinder(r=cpd/2-0.1,h=t+0.02,center=true,$fn=36);
			cylinder(r=chd/2,h=t+0.2,center=true,$fn=36);
		}
	}

	module pad(tr) {
		color ([1,1,0]) translate (tr) difference () {
			union () {
				translate([0,(py-0.1)/2,0]) cube([ppd,py-0.1,t+0.02],center=true);
				cylinder(r=ppd/2,h=t+0.02,center=true,$fn=36);
			}
			cylinder(r=phd/2,h=t+0.2,center=true,$fn=36);
		}
	}

	translate([0,0,(th-dh)-t/2]) union () {
		color ([0,0.5,0])  difference () {
			cube([W,H,t],center=true);
			union () {
				translate([cx/2,cy/2,0]) cylinder(r=chd/2+0.1,h=t+0.2,center=true,$fn=36);
				translate([-cx/2,cy/2,0]) cylinder(r=chd/2+0.1,h=t+0.2,center=true,$fn=36);
				translate([-cx/2,-cy/2,0]) cylinder(r=chd/2+0.1,h=t+0.2,center=true,$fn=36);
				translate([cx/2,-cy/2,0]) cylinder(r=chd/2+0.1,h=t+0.2,center=true,$fn=36);
				if (n>0) {for (i=[1:n]) {
					translate([-W/2+px+(i-1)*pdt,H/2-py,0]) cylinder(r=(phd/2)+0.1,h=t+0.2,center=true,$fn=36);
				}}
			}
		}
		cpad([cx/2,cy/2,0]);
		cpad([-cx/2,cy/2,0]);
		cpad([-cx/2,-cy/2,0]);
		cpad([cx/2,-cy/2,0]);
		if (n>0) { for (i=[1:n]) {
			pad([-W/2+px+(i-1)*pdt,H/2-py,0]);
		}}
		color ([0.9,0.9,0.9]) translate ([0,0,dh/2+t/2])
		difference () {
		 cube([BW,BH,dh],center=true);
			union () {
				translate([0,0,dh/2-0.45]) cube([DW,DH,1],center=true);
				translate([0,(DH+BH)/4,dh/2]) rotate(90,[0,1,0]) cylinder(r=0.5,h=DW,center=true,$fn=36);
				translate([0,-(DH+BH)/4,dh/2]) rotate(90,[0,1,0]) cylinder(r=0.5,h=DW,center=true,$fn=36);
			}
		}
		color ([0.2,0.3,0])  translate([0,0,dh+t/2-0.55]) cube([DW,DH,1],center=true);
		color ([0,0,0]) translate ([0,BH/2-1,-((th-dh)-t)/2-t/2]) cube([DW,2,((th-dh)-t)],center=true);
		color ([0,0,0]) translate ([0,-(BH/2-1),-((th-dh)-t)/2-t/2]) cube([DW,2,((th-dh)-t)],center=true);
	}
}
