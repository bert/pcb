/*
OpenSCAD Model:  chip resistor

  L  - body length
  T  - pad length
  W  - width
  H  - height
  padcolor - color of metal leads
  bodycolor - color of body
  topcolor - color body top
  simple - 0 = realistic model, 1 = simplified model
*/

module chip_r(
	L=2.05,
	T=0.35,
	W=1.25,
	H=0.50,
	padcolor=[0.9,0.9,0.9],
	bodycolor=[1,1,0.9],
	topcolor=[0.0,0.0,0.0],
	labelcolor=[1,1,1],
	label="330",
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
	color(topcolor) translate([0,0,H-0.05]) cube([L-2*T,W-0.2,0.05],center=true);
	translate ([-L/2.+T/2.,0,H/2.]) chippad();
	translate ([L/2.-T/2.,0,H/2.]) chippad();
}
