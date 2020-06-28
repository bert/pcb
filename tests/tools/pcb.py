#!/bin/python

class pcb_pcb(object):
  def __init__(self, x=2500, y=2000, name=None):
    self.units = "mil"
    self.name = "python pcb" if name is None else name
    self.x = x
    self.y = y
    self.grid = "Grid[1.00mil 0.0000 0.0000 1]"
    self.poly_area = "PolyArea[3100]"
    self.thermal = "Thermal[0.5]"
    self.drc = "DRC[5mil 5mil 5mil 5mil 10mil 10mil]"
    self.flags = 'Flags("nameonpcb,clearnew,snappin")'
    self.groups = 'Groups("1,c:2:3:4:5:6,s:7:8")'
    self.styles = 'Styles["Signal,10.00mil,30.00mil,10.00mil,1.00mil:Power,25.00mil,60.00mil,35.00mil,10.00mil:Fat,40.00mil,60.00mil,35.00mil,10.00mil:Skinny,6.00mil,24.02mil,11.81mil,6.00mil"]'

    self.layers = []


  def __str__(self):
    s = "" + self.header() + "\n"
    for l in self.layers:
      s += str(l)

    return s


  def header(self):
    s = 'PCB["{}" {}{} {}{}]\n'.format(self.name, self.x, self.units,
                                                  self.y, self.units)
    for item in [self.grid, self.poly_area, self.thermal, self.drc,
                 self.flags, self.groups, self.styles]:
      s += item + "\n"

    return s


  def add_layer(self, n=None, name=None, ltype=None):
    n = len(self.layers) + 1 if n is None else n
    self.layers.append(pcb_layer(n, name, ltype))
    return self.layers[-1]


class pcb_line(object):
  def __init__(self, x1=0, y1=0, x2=None, y2=None, th=10, cl=2, fl=None):
    self.units = "mil"
    self.x1 = x1
    self.y1 = y1
    self.x2 = x1 if x2 is None else x2
    self.y2 = y1 + 50 if y2 is None else y2
    self.thickness = th
    self.clearance = cl
    self.flags = ["clearline"] if fl is None else fl


  def __str__(self):
    s = "  Line["
    for val in [self.x1, self.y1,
                self.x2, self.y2,
                self.thickness, self.clearance]:
      s += "{}{} ".format(val, self.units)

    s += '"' + ",".join(self.flags) + '"'
    s += "]\n"
    return s


class pcb_arc(object):
  def __init__(self, x=0, y=0, r=50, th=10, cl=2, sa=-90, da=90, fl=None):
    self.units = "mil"
    self.center_x = x
    self.center_y = y
    self.radius = r
    self.thickness = th
    self.clearance = cl
    self.start_angle = sa
    self.delta_angle = da
    self.flags = ["clearline"] if fl is None else fl


  def __str__(self):
    s = "  Arc["
    for val in [self.center_x, self.center_y,
                self.radius, self.radius,
                self.thickness, self.clearance]:
      s += "{}{} ".format(val, self.units)

    for val in [self.start_angle, self.delta_angle]:
      s += "{} ".format(val)

    s += '"' + ",".join(self.flags) + '"'
    s += "]\n"
    return s


class pcb_layer(object):
  def __init__(self, n, name, ltype="copper"):
    self.n = n
    self.name = name
    self.ltype = ltype

    self.objects = []


  def __str__(self):
    s = 'Layer({} "{}" "{}")\n(\n'.format(self.n, self.name, self.ltype)
    for o in self.objects:
      s += str(o)

    s += ")\n"
    return s


class pcb_point(object):
  def __init__(self, x, y):
    self.units = "mil"
    self.x = x
    self.y = y


  def __str__(self):
    s = "[{}{} {}{}]".format(self.x, self.units, self.y, self.units)
    return s


class pcb_polygon(object):
  def __init__(self):
    self.units = "mil"
    self.points = []
    self.holes = []
    self.flags = ["clearpoly"]


  def __str__(self):
    s = '  Polygon("{}")\n  (\n    '.format(",".join(self.flags))
    s += " ".join([str(x) for x in self.points])
    s += "\n"
    s += "\n".join([str(h) for h in self.holes])
    s += "  )\n"
    return s


  def add_point(self, x, y):
    self.points.append(pcb_point(x, y))


  def rect(self, x0, y0, dx, dy):
    self.points = []
    self.add_point(x0, y0)
    self.add_point(x0 + dx, y0)
    self.add_point(x0 + dx, y0 + dy)
    self.add_point(x0, y0 + dy)


  def new_hole(self):
    self.holes.append(pcb_polyhole())
    return self.holes[-1]


class pcb_polyhole(object):
  def __init__(self):
    self.units = "mil"
    self.points = []


  def __str__(self):
    s = "    Hole (\n      "
    s += " ".join([str(x) for x in self.points])
    s += "\n    )\n"
    return s


  def add_point(self, x, y):
    self.points.append(pcb_point(x, y))


  def rect(self, x0, y0, dx, dy):
    self.points = []
    self.add_point(x0, y0)
    self.add_point(x0 + dx, y0)
    self.add_point(x0 + dx, y0 + dy)
    self.add_point(x0, y0 + dy)


class pcb_text(object):
  def __init__(self, x=None, y=None, txt=None, scale=100):
    self.units = "mil"
    self.x = x
    self.y = y
    self.txt = txt
    self.scale = scale
    self.direction = 0
    self.flags = ["clearline"]


  def __str__(self):
    s = '  Text[{}{} {}{} {} {} "{}" "{}"]\n'.format(
            self.x, self.units, self.y, self.units,
            self.direction, self.scale, self.txt,
            ",".join(self.flags))
    return s


