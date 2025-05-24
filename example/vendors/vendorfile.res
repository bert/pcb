# Optional name of the vendor
vendor = "Vendor Name"

# units for dimensions in this file.
# Allowed values: mil/inch/mm
units = mil

# drill table
drillmap = {
   # When mapping drill sizes, select the nearest size
   # or always round up. Allowed values: up/nearest
   round = up

   # The list of vendor drill sizes.
   # Units are as specified above.
   20
   28
   35 
   38
   42
   52
   59.5
   86
   125
   152

   # optional section for skipping mapping of certain elements
   # based on reference designator, value, or description
   # this is useful for critical parts where you may not
   # want to change the drill size. Note that the strings
   # are regular expressions.
   skips = {
      {refdes "^J3$"} # Skip J3.
      {refdes "J3"} # Skip anything with J3 as part of the refdes.
      {refdes "^U[1-3]$" "^X.*"} # Skip U1, U2, U3, and anything starting with X.
      {value "^JOHNSTECH_.*"} # Skip all Johnstech footprints based on the value of a p
      {descr "^AMP_MICTOR_767054_1$"} # Skip based on the description.
    }
}

# If specified, this section will change the current DRC
# settings for the design. Units are as specified above.
drc = {
   copper_space = 7
   copper_width = 7
   silk_width = 10
   copper_overlap = 4
}
