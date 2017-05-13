/*!
 * \file src/hid_draw.h
 *
 * \brief Human Interface Device - Drawing.
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Contact addresses for paper mail and Email:
 * harry eaton, 6697 Buttonhole Ct, Columbia, MD 21044 USA
 * haceaton@aplcomm.jhuapl.edu
 */


/*!
 * \brief Mask modes.
 */
enum mask_mode
{
  HID_MASK_OFF    = 0, /*!< Flush the buffer and return to non-mask operation. */
  HID_MASK_BEFORE = 1, /*!< Polygons being drawn before clears.                */
  HID_MASK_CLEAR  = 2, /*!< Clearances being drawn.                            */
  HID_MASK_AFTER  = 3, /*!< Polygons being drawn after clears.                 */
};


/*!
 * \brief Low level drawing API Drawing Functions.
 *
 * Coordinates and distances are ALWAYS in PCB's default coordinates
 * (1 nm at the time this comment was written).
 *
 * Angles are always in degrees, with 0 being "right" (positive X) and
 * 90 being "up" (positive Y).
 */
struct hid_draw_st
{
  hidGC (*make_gc) (void); /*!< Make an empty graphics context.  */
  void (*destroy_gc) (hidGC gc);
  void (*use_mask) (enum mask_mode mode);

  void (*set_color) (hidGC gc, const char *name);
    /*!< Set a color.  Names can be like "red" or "#rrggbb" or special
     * names like "erase".
     * *Always* use the "erase" color for removing ink (like polygon
     * reliefs or thermals), as you cannot rely on knowing the background
     * color or special needs of the HID.
     * Always use the "drill" color to draw holes.
     * You may assume this is cheap enough to call inside the redraw
     * callback, but not cheap enough to call for each item drawn.
     */

  void (*set_line_cap) (hidGC gc, EndCapStyle style);
    /*!< Set the line style.  While calling this is cheap, calling it with
     * different values each time may be expensive, so grouping items by
     * line style is helpful.
     */
  void (*set_line_width) (hidGC gc, Coord width);
  void (*set_draw_xor) (hidGC gc, int xor_);

  void (*set_draw_faded) (hidGC gc, int faded);
    /*!< Blends 20% or so color with 80% background.
     * Only used for assembly drawings so far.
     */

  /* The usual drawing functions.  "draw" means to use segments of the
     given width, whereas "fill" means to fill to a zero-width
     outline.  */
  void (*draw_line)    (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);
  void (*draw_arc)     (hidGC gc, Coord cx, Coord cy, Coord xradius, Coord yradius, Angle start_angle, Angle delta_angle);
  void (*draw_rect)    (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);
  void (*fill_circle)  (hidGC gc, Coord cx, Coord cy, Coord radius);
  void (*fill_polygon) (hidGC gc, int n_coords, Coord *x, Coord *y);
  void (*fill_rect)    (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2);

  /* The following APIs render using PCB data-structures, not immediate parameters */

  void (*draw_pcb_line) (hidGC gc, LineType *line);
  void (*draw_pcb_arc) (hidGC gc, ArcType *arc);
  void (*draw_pcb_text) (hidGC gc, TextType *, Coord);
  void (*draw_pcb_polygon) (hidGC gc, PolygonType *poly, const BoxType *clip_box);

  void (*fill_pcb_polygon) (hidGC gc, PolygonType *poly, const BoxType *clip_box);
  void (*thindraw_pcb_polygon) (hidGC gc, PolygonType *poly, const BoxType *clip_box);
  void (*fill_pcb_pad) (hidGC gc, PadType *pad, bool clip, bool mask);
  void (*thindraw_pcb_pad) (hidGC gc, PadType *pad, bool clip, bool mask);
  void (*fill_pcb_pv) (hidGC fg_gc, hidGC bg_gc, PinType *pv, bool drawHole, bool mask);
  void (*thindraw_pcb_pv) (hidGC fg_gc, hidGC bg_gc, PinType *pv, bool drawHole, bool mask);

};
