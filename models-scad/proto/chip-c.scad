/*
OpenSCAD Model:  chip capacitor

  L  - body length
  T  - pad length
  W  - width
  H  - height
  padcolor - color of metal leads
  bodycolor - color of body
  simple - 0 = realistic model, 1 = simplified model
*/

module chip_c(
	L=2.00,
	T=0.50,
	W=1.25,
	H=1.20,
	padcolor=[0.9,0.9,0.9],
	bodycolor=[0.722,0.541,0.0],
	simple=1
) {

	module chippad() {
		if (simple) {
			color(padcolor)cube([T,W,H],center=true);
		} else {
			color(padcolor) union () {
				cube([T-0.1,W-0.1,H    ],center=true);
				cube([T-0.1,W    ,H-0.1],center=true);
				cube([T    ,W-0.1,H-0.1],center=true);
				translate([-T/2+0.05,-W/2+0.05,0]) cylinder(r=0.05,h=H-0.1,$fn=30,center=true);
				translate([T/2-0.05,-W/2+0.05,0]) cylinder(r=0.05,h=H-0.1,$fn=30,center=true);
				translate([-T/2+0.05,W/2-0.05,0]) cylinder(r=0.05,h=H-0.1,$fn=30,center=true);
				translate([T/2-0.05,W/2-0.05,0]) cylinder(r=0.05,h=H-0.1,$fn=30,center=true);
				translate([-T/2+0.05,0,H/2-0.05]) rotate(90,[1,0,0])cylinder(r=0.05,h=W-0.1,$fn=30,center=true);
				translate([T/2-0.05,0,H/2-0.05]) rotate(90,[1,0,0])cylinder(r=0.05,h=W-0.1,$fn=30,center=true);
				translate([-T/2+0.05,0,-H/2+0.05]) rotate(90,[1,0,0])cylinder(r=0.05,h=W-0.1,$fn=30,center=true);
				translate([T/2-0.05,0,-H/2+0.05]) rotate(90,[1,0,0])cylinder(r=0.05,h=W-0.1,$fn=30,center=true);
				translate([0,-W/2+0.05,H/2-0.05]) rotate(90,[0,1,0])cylinder(r=0.05,h=T-0.1,$fn=30,center=true);
				translate([0,W/2-0.05,H/2-0.05]) rotate(90,[0,1,0])cylinder(r=0.05,h=T-0.1,$fn=30,center=true);
				translate([0,-W/2+0.05,-H/2+0.05]) rotate(90,[0,1,0])cylinder(r=0.05,h=T-0.1,$fn=30,center=true);
				translate([0,W/2-0.05,-H/2+0.05]) rotate(90,[0,1,0])cylinder(r=0.05,h=T-0.1,$fn=30,center=true);
				translate([-T/2+0.05,-W/2+0.05,-H/2+0.05]) sphere(r=0.05, $fn=30);
				translate([-T/2+0.05,-W/2+0.05,H/2-0.05]) sphere(r=0.05, $fn=30);
				translate([T/2-0.05,-W/2+0.05,-H/2+0.05]) sphere(r=0.05, $fn=30);
				translate([T/2-0.05,-W/2+0.05,H/2-0.05]) sphere(r=0.05, $fn=30);
				translate([-T/2+0.05,W/2-0.05,-H/2+0.05]) sphere(r=0.05, $fn=30);
				translate([-T/2+0.05,W/2-0.05,H/2-0.05]) sphere(r=0.05, $fn=30);
				translate([T/2-0.05,W/2-0.05,-H/2+0.05]) sphere(r=0.05, $fn=30);
				translate([T/2-0.05,W/2-0.05,H/2-0.05]) sphere(r=0.05, $fn=30);
			}
		}
	}
	color(bodycolor) translate([0,0,H/2.]) cube([L-2*T+0.1,W-0.1,H-0.1],center=true);
	translate ([-L/2.+T/2.,0,H/2.]) chippad();
	translate ([L/2.-T/2.,0,H/2.]) chippad();
}
