# element_flags, description, pcb-name, value, mark_x, mark_y,
# text_x, text_y, text_direction, text_scale, text_flags
Element[0x00000000 "Surface Mount Coin Cell Holder" "J?" "KEYSTONE_1062" 0 0 -3150 -3150 0 100 ""]
(
# Pad[x1, y1, x2, y2, thickness, clearance, mask, name , pad number, flags]
  Pad[0 -39400 0 -46500 12500 1000 800 "NEG" "1" "square"]
  Pad[0  39400 0  46500 12500 1000 800 "POS" "2" "square"]

# ElementArc[x, y, xradius, yradius, start angle, delta angle, width]
  ElementArc[0 -16550 45800 45800 156  65 1000]
  ElementArc[0 -16550 45800 45800  24 -65 1000]

# ElementLine[x1, y1, x2, y2, thickness]
  # left/right lines that slope down and to the center
  ElementLine[ 41794 2182  22850 44450 1000]
  ElementLine[-41794 2182 -22850 44450 1000]

  # lines around bottom pad
  ElementLine[  22850 44450  8250 44450 1000]
  ElementLine[ -22850 44450 -8250 44450 1000]

  ElementLine[  8250 44450  8250 54450 1000]
  ElementLine[ -8250 44450 -8250 54450 1000]
  ElementLine[ -8250 54450  8250 54450 1000]

  # lines around top pad
  ElementLine[  34456 -46650  8250 -46650 1000]
  ElementLine[ -34456 -46650 -8250 -46650 1000]

  ElementLine[  8250 -46650  8250 -54450 1000]
  ElementLine[ -8250 -46650 -8250 -54450 1000]
  ElementLine[ -8250 -54450  8250 -54450 1000]

# POS terminal
  ElementLine[ -13250 46500 -13250 50500 1000]
  ElementLine[ -15250 48500 -11250 48500 1000]

# NEG terminal
  ElementLine[ -15250 -48500 -11250 -48500 1000]

)
