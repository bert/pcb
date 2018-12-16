/*!
 * \file src/hid/shopper/shopper.c
 *
 * \brief Exporter to automagically parse the current pcb and submit a
 * Request For Quotation (RFQ) on https://pcbshopper.com/ .
 *
 * What this exporter does is:
 * - assemble all required data for the RFQ from the current pcb,
 * - create and open a popup dialog for missing data to be given by the
 *   user (typically a human, this is also a good moment to check the
 *   gathered and assembled data),
 * - prepare a HTTP GET message for the pcbshopper server,
 * - send the HTTP GET message to pcbshopper.com,
 * - wait for and receive an answer from the pcbshopper server,
 * - open a (default) web browser session,
 * - display the quotes from various board houses.
 *
 * The original Eagle CAD User Language Program (ULP) is
 * Copyright (C) Jeremy Ruhland 2016 and licensed under GPL 3.0.
 *
 * For the original ULP code see:
 *
 * https://github.com/JeremyRuhland/pcbshopper
 *
 * \author Jeremy Ruhland <jeremy@goopypanther.org>
 *
 * The idea of the ULP code is adapted for use as an exporter by this
 * inter-active pcb layout editor and this adaptation is re-licensed
 * under the GPL v2+ license.
 *
 * \author Copyright (C) 2018 for adaptation by Bert Timmerman
 * <bert.timmerman@xs4all.nl>
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996 Thomas Nau
 *
 * Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact addresses for paper mail and Email:
 * Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 * haceaton@aplcomm.jhuapl.edu
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <math.h>

#include "global.h"
#include "data.h"
#include "error.h"
#include "misc.h"
#include "pcb-printf.h"

#include "hid.h"
#include "hid/common/hidnogui.h"
#include "../hidint.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

/*!
 * \brief Units option names.
 *
 * \warning Please use the exact names from https://pcbshopper.com/ in
 * the exact order given in the combobox.
 */
static const char *units_option_names[] =
{
  "inches",
  "mm",
  "cm",
  "mil",
  NULL
};

/*!
 * \brief Solder mask colour option names.
 *
 * \warning Please use the exact names from https://pcbshopper.com/ in
 * the exact order given in the combobox.
 */
static const char *solder_mask_colour_option_names[] =
{
  "No mask",
  "Least expensive color",
  "Black",
  "Black Matte",
  "Blue",
  "Green",
  "Green Matte",
  "Purple",
  "Red",
  "Transparent",
  "White",
  "Yellow",
  NULL
};

/*!
 * \brief Silkscreen option names.
 *
 * \warning Please use the exact names from https://pcbshopper.com/ in
 * the exact order given in the combobox.
 */
static const char *silkscreen_option_names[] =
{
  "None",
  "Top",
  "Bottom",
  "Both",
  NULL
};

/*!
 * \brief Surface finish option names.
 *
 * \warning Please use the exact names from https://pcbshopper.com/ in
 * the exact order given in the combobox.
 */
static const char *surface_finish_option_names[] =
{
  "Cheapest",
  "Cheapest Lead-free",
  "HASL (with lead)",
  "Lead-free HASL",
  "ENIG",
  NULL
};

/*!
 * \brief Board thickness option names.
 *
 * \warning Please use the exact names from https://pcbshopper.com/ in
 * the exact order given in the combobox.
 */
static const char *board_thickness_option_names[] =
{
  "0.6mm/0.024\"",
  "0.8mm/0.031\"",
  "1.0mm/0.040\"",
  "1.2mm/0.047\"",
  "1.6mm/0.062\"",
  "2.0mm/0.079\"",
  NULL
};

/*!
 * \brief Copper weight option names.
 *
 * \warning Please use the exact names from https://pcbshopper.com/ in
 * the exact order given in the combobox.
 */
static const char *copper_weight_option_names[] =
{
  "1 oz",
  "2 oz",
  "3 oz",
  "4 oz",
  "5 oz",
  "6 oz",
  NULL
};

/*!
 * \brief Stencil option names.
 *
 * \warning Please use the exact names from https://pcbshopper.com/ in
 * the exact order given in the combobox.
 */
static const char *stencil_option_names[] =
{
  "No",
  "One side",
  "Both sides",
  NULL
};

/*!
 * \brief Quality certification option names.
 *
 * \warning Please use the exact names from https://pcbshopper.com/ in
 * the exact order given in the combobox.
 */
static const char *quality_option_names[] =
{
  "Not required",
  "ISO 9001",
  "IPC 600",
  "Either ISO 9001 or IPC 600",
  "Both ISO 9001 and IPC 600",
  NULL
};

/*!
 * \brief Array of strings for country names according to ISO 3166-2.
 *
 * \warning Please use the exact names from https://pcbshopper.com/ in
 * the exact order given in the combobox.
 */
char *country_names[] =
{
  "Afghanistan", "Åland Islands", "Albania", "Algeria", "American Samoa",
  "Andorra", "Angola", "Anguilla", "Antigua + Barbuda", "Argentina",
  "Armenia", "Aruba", "Australia", "Austria", "Azerbaijan",
  "Bahamas", "Bahrain", "Bangladesh", "Barbados", "Belarus",
  "Belgium", "Belize", "Benin", "Bermuda", "Bhutan",
  "Bolivia", "Bonaire", "Bosnia-Herzegovina", "Botswana", "Bouvet Island",
  "Brazil", "British Indian Ocean", "Brunei", "Bulgaria", "Burkina Faso",
  "Burundi", "Cambodia", "Cameroon", "Canada", "Cape Verde",
  "Cayman Islands", "Central African Rep.", "Chad", "Chile", "China",
  "Christmas Island", "Cocos (Keeling) Islands", "Colombia", "Comoros", "Congo",
  "Congo, the Democratic Republic of the", "Cook Islands", "Costa Rica", "Côte d'Ivoire", "Croatia",
  "Curaçao", "Cyprus", "Czech Republic", "Denmark", "Djibouti",
  "Dominica", "Dominican Republic", "Ecuador", "Egypt", "El Salvador",
  "Equatorial Guinea", "Eritrea", "Estonia", "Ethiopia", "Falkland Islands",
  "Faroe Islands", "Fiji", "Finland", "France", "French Guiana",
  "French Polynesia", "Gabon", "Gambia", "Georgia", "Germany",
  "Ghana", "Gibraltar", "Greece", "Greenland", "Grenada",
  "Guadeloupe", "Guam", "Guatemala", "Guernsey", "Guinea",
  "Guinea-Bissau", "Guyana", "Haiti", "Holy See (Vatican)", "Honduras",
  "Hong Kong", "Hungary", "Iceland", "India", "Indonesia",
  "Iran", "Iraq", "Ireland", "Isle of Man", "Israel",
  "Italy", "Jamaica", "Japan", "Jersey", "Jordan",
  "Kazakhstan", "Kenya", "Kiribati", "Korea, Republic of", "Kuwait",
  "Kyrgyzstan", "Laos", "Latvia", "Lebanon", "Lesotho",
  "Liberia", "Libya", "Liechtenstein", "Lithuania", "Luxembourg",
  "Macau", "Macedonia", "Madagascar", "Malawi", "Malaysia",
  "Maldives", "Mali", "Malta", "Marshall Islands", "Martinique",
  "Mauritania", "Mauritius", "Mayotte", "Mexico", "Micronesia",
  "Moldova", "Monaco", "Mongolia", "Montenegro", "Montserrat",
  "Morocco", "Mozambique", "Myanmar", "Namibia", "Nauru",
  "Nepal", "Netherlands", "New Caledonia", "New Zealand", "Nicaragua",
  "Niger", "Nigeria", "Niue", "Norfolk Island", "North Mariana Is.",
  "Norway", "Oman", "Pakistan", "Palau", "Palestinian Territory",
  "Panama", "Papua New Guinea", "Paraguay", "Peru", "Philippines",
  "Pitcairn", "Poland", "Portugal", "Puerto Rico", "Qatar",
  "Réunion", "Romania", "Russia", "Rwanda", "Saint Barthélemy",
  "Saint Helena", "Saint Kitts and Nevis", "Saint Lucia", "Saint Martin", "Saint Pierre",
  "Saint Vincent", "Samoa", "San Marino", "Sao Tome", "Saudi Arabia",
  "Senegal", "Serbia", "Seychelles", "Sierra Leone", "Singapore",
  "Sint Maarten", "Slovakia", "Slovenia", "Solomon Islands", "Somalia",
  "South Africa", "South Georgia", "South Sudan", "Spain", "Sri Lanka",
  "Sudan", "Suriname", "Svalbard", "Swaziland", "Sweden",
  "Switzerland", "Syria", "Taiwan", "Tajikistan", "Tanzania",
  "Thailand", "Timor-Leste", "Togo", "Tokelau", "Tonga",
  "Trinidad", "Tunisia", "Turkey", "Turkmenistan", "Turks and Caicos",
  "Tuvalu", "Uganda", "Ukraine", "United Arab Emirates", "United Kingdom",
  "United States", "Uruguay", "Uzbekistan", "Vanuatu", "Venezuela",
  "Viet Nam", "Virgin Islands, British", "Virgin Islands, U.S.", "Wallis and Futuna", "Western Sahara",
  "Yemen", "Zambia", "Zimbabwe", NULL
};

/*!
 * \brief Array of strings for country codes according to ISO 3166-2.
 *
 * \note These strings are not obtained from https://pcbshopper.com/ .
 *
 * \warning Please use the exact order as the country_names array from
 * https://pcbshopper.com/ .
 */
char *country_code[] =
{
  "AF", "AX", "AL", "DZ", "AS", "AD", "AO", "AI", "AG", "AR",
  "AM", "AW", "AU", "AT", "AZ", "BS", "BH", "BD", "BB", "BY",
  "BE", "BZ", "BJ", "BM", "BT", "BO", "BQ", "BA", "BW", "BV",
  "BR", "IO", "BN", "BG", "BF", "BI", "KH", "CM", "CA", "CV",
  "KY", "CF", "TD", "CL", "CN", "CX", "CC", "CO", "KM", "CG",
  "CD", "CK", "CR", "CI", "HR", "CW", "CY", "CZ", "DK", "DJ",
  "DM", "DO", "EC", "EG", "SV", "GQ", "ER", "EE", "ET", "FK",
  "FO", "FJ", "FI", "FR", "GF", "PF", "GA", "GM", "GE", "DE",
  "GH", "GI", "GR", "GL", "GD", "GP", "GU", "GT", "GG", "GN",
  "GW", "GY", "HT", "VA", "HN", "HK", "HU", "IS", "IN", "ID",
  "IR", "IQ", "IE", "IM", "IL", "IT", "JM", "JP", "JE", "JO",
  "KZ", "KE", "KI", "KR", "KW", "KG", "LA", "LV", "LB", "LS",
  "LR", "LY", "LI", "LT", "LU", "MO", "MK", "MG", "MW", "MY",
  "MV", "ML", "MT", "MH", "MQ", "MR", "MU", "YT", "MX", "FM",
  "MD", "MC", "MN", "ME", "MS", "MA", "MZ", "MM", "NA", "NR",
  "NP", "NL", "NC", "NZ", "NI", "NE", "NG", "NU", "NF", "MP",
  "NO", "OM", "PK", "PW", "PS", "PA", "PG", "PY", "PE", "PH",
  "PN", "PL", "PT", "PR", "QA", "RE", "RO", "RU", "RW", "BL",
  "SH", "KN", "LC", "MF", "PM", "VC", "WS", "SM", "ST", "SA",
  "SN", "RS", "SC", "SL", "SG", "SX", "SK", "SI", "SB", "SO",
  "ZA", "GS", "SS", "ES", "LK", "SD", "SR", "SJ", "SZ", "SE",
  "CH", "SY", "TW", "TJ", "TZ", "TH", "TL", "TG", "TK", "TO",
  "TT", "TN", "TR", "TM", "TC", "TV", "UG", "UA", "AE", "GB",
  "US", "UY", "UZ", "VU", "VE", "VN", "VG", "VI", "WF", "EH",
  "YE", "ZM", "ZW", NULL
};

/*!
 * \brief Array of strings for the lead time options.
 *
 * \warning Please use the exact names from https://pcbshopper.com/ in
 * the exact order given in the combobox.
 */
char *lead_time_options[] =
{
  "Default time",
  "2 business days",
  "3 business days",
  "4 business days",
  "5 business days",
  "6 business days",
  "7 business days",
  "8 business days",
  "9 business days",
  "10 business days",
  "11 business days",
  "12 business days",
  "13 business days",
  "14 business days",
  "15 business days",
  "16 business days",
  "17 business days",
  "18 business days",
  "19 business days",
  "20 business days",
  "21 business days",
  "22 business days",
  "23 business days",
  "24 business days",
  "25 business days",
  "26 business days",
  "27 business days",
  "28 business days",
  "29 business days",
  "30 business days",
  NULL
};

static HID_Attribute shopper_attribute_list[] = {
/* other HIDs expect this to be first after all the character array
 * definitions (used strings in the HID menu dialog) are done with. */

/* %start-doc options "99 pcbshopper.com RFQ Options"
@ftable @code
@item --unit <unit>
Unit of dimensions. Defaults to mil. Please use units consistant.
Parameter @code{<unit>} can be @samp{inches}, @samp{mm}, @samp{cm}, or
@samp{mil}.
@end ftable
%end-doc
*/
  {N_("unit"), N_("Unit of dimensions, please use units consistent"),
   HID_Unit, 0, 0, {-1, 0, 0}, NULL, 0},
#define HA_unit 0

/* %start-doc options "99 pcbshopper.com RFQ Options"
@ftable @code
@item --width <double>
Width of the pcb.
@end ftable
%end-doc
*/
  {N_("width"), N_("Width of the pcb"),
   HID_Real, 0, 0, {0, 0, 0}, 0, 0},
#define HA_width 1

/* %start-doc options "99 pcbshopper.com RFQ Options"
@ftable @code
@item --height <double>
Height of the pcb.
@end ftable
%end-doc
*/
  {N_("height"), N_("Height of the pcb"),
   HID_Real, 0, 0, {0, 0, 0}, 0, 0},
#define HA_height 2

/* %start-doc options "99 pcbshopper.com RFQ Options"
@ftable @code
@item --layers <int>
Where @code{<int>} is the number of conductive layers.
Please use one of the positive Integer numbers @math{ \{1, 2, 3, ...\} }.
@end ftable
%end-doc
*/
  {N_("layers"), N_("Number of conductive layers"),
   HID_Integer, 0, 0, {1, 0, 0}, 0, 0},
#define HA_layers 3

/* %start-doc options "99 pcbshopper.com RFQ Options"
@ftable @code
@cindex solder-mask-colour
@item --solder-mask-colour <string>
The solder mask colour.
Parameter @code{<string>} can be @samp{"No mask"},
@samp{"Least expensive color"}, @samp{"Black"}, @samp{"Black Matte"},
@samp{"Blue"}, @samp{"Green"}, @samp{"Green Matte"}, @samp{"Purple"},
@samp{"Red"}, @samp{"Transparent"}, @samp{"White"}, or @samp{"Yellow"}.
@end ftable
%end-doc
*/
  {N_("solder-mask-colour"), N_("The solder mask colour"),
   HID_Enum, 0, 0, {0, 0, 0}, solder_mask_colour_option_names, 0},
#define HA_solder_mask_colour 4

/* %start-doc options "99 pcbshopper.com RFQ Options"
@ftable @code
@cindex silkscreen
@item --silkscreen <string>
The silkscreen option.
Parameter @code{<string>} can be @samp{"None"}, @samp{"Top"},
@samp{"Bottom"}, or @samp{"Both"}.
@end ftable
%end-doc
*/
  {N_("silkscreen"), N_("The silkscreen option"),
   HID_Enum, 0, 0, {1, 0, 0}, silkscreen_option_names, 0},
#define HA_silkscreen 5

/* %start-doc options "99 pcbshopper.com RFQ Options"
@ftable @code
@cindex surface-finish
@item --surface-finish <string>
The surface finish option.
Parameter @code{<string>} can be @samp{"Cheapest"},
@samp{"Cheapest Lead-free"}, @samp{"HASL (with lead)"},
@samp{"Lead-free HASL"}, or @samp{"ENIG"}.
@end ftable
%end-doc
*/
  {N_("surface-finish"), N_("The surface finish option"),
   HID_Enum, 0, 0, {0, 0, 0}, surface_finish_option_names, 0},
#define HA_surface_finish 6

/* %start-doc options "99 pcbshopper.com RFQ Options"
@ftable @code
@cindex board-thickness
@item --board-thickness <string>
The board thickness option.
Parameter @code{<string>} can be @samp{"0.6mm/0.024"},
@samp{"0.8mm/0.031"}, @samp{"1.0mm/0.040"}, @samp{"1.2mm/0.047"},
@samp{"1.6mm/0.062"}, or @samp{"2.0mm/0.079"}.
@end ftable
%end-doc
*/
  {N_("board-thickness"), N_("The board thickness option"),
   HID_Enum, 0, 0, {4, 0, 0}, board_thickness_option_names, 0},
#define HA_board_thickness_option 7

/* %start-doc options "99 pcbshopper.com RFQ Options"
@ftable @code
@cindex copper-weight-option
@item --copper-weight-option <string>
The copper weight of outer layers.
Parameter @code{<string>} can be @samp{"1oz"}, @samp{"2oz"},
@samp{"3oz"}, @samp{"4oz"}, @samp{"5oz"}, or @samp{"6oz"}.
@end ftable
%end-doc
*/
  {N_("copper-weight-option"), N_("The copper weight of outer layers option"),
   HID_Enum, 0, 0, {0, 0, 0}, copper_weight_option_names, 0},
#define HA_copper_weight_option 8

/* %start-doc options "99 pcbshopper.com RFQ Options"
@ftable @code
@item --min-trace-width <real>
Minimum trace width in units as defined with @samp{--unit}.
@end ftable
%end-doc
*/
  {N_("min-trace-width"), N_("Minimum trace width"),
   HID_Real, 0, 10000, {6.0, 0, 0}, 0, 0},
#define HA_min_trace_width 9

/* %start-doc options "99 pcbshopper.com RFQ Options"
@ftable @code
@item --min-space-width <real>
Minimum space between traces in units as defined with @samp{--unit}.
@end ftable
%end-doc
*/
  {N_("min-space-width"), N_("Minimum space between traces"),
   HID_Real, 0, 10000, {6.0, 0, 0}, 0, 0},
#define HA_min_space_width 10

/* %start-doc options "99 pcbshopper.com RFQ Options"
@ftable @code
@item --min-drill-width <real>
Minimum drill width in units as defined with @samp{--unit}.
@end ftable
%end-doc
*/
  {N_("min-drill-width"), N_("Minimum drill width"),
   HID_Real, 0, 10000, {16.0, 0, 0}, 0, 0},
#define HA_min_drill_width 11

/* %start-doc options "99 pcbshopper.com RFQ Options"
@ftable @code
@item --gold-fingers <int>
Where @code{<int>} is the total number of gold fingers on the board.
Please use one of the positive Integer numbers @math{ \{1, 2, 3, ...\} }.
@end ftable
%end-doc
*/
  {N_("gold-fingers"), N_("The total number of gold fingers on the board"),
   HID_Integer, 0, 0, {0, 0, 0}, 0, 0},
#define HA_gold_fingers 12

/* %start-doc options "99 pcbshopper.com RFQ Options"
@ftable @code
@cindex stencil
@item --stencil <string>
The stencil option.
Parameter @code{<string>} can be @samp{"No"}, @samp{"One side"},
or @samp{"Both sides"}.
@end ftable
%end-doc
*/
  {N_("stencil"), N_("The stencil option"),
   HID_Enum, 0, 0, {0, 0, 0}, stencil_option_names, 0},
#define HA_stencil 13

/* %start-doc options "99 pcbshopper.com RFQ Options"
@ftable @code
@cindex quality
@item --quality <string>
The quality certification option.
Parameter @code{<string>} can be @samp{"Not required"},
@samp{"ISO 9001"}, @samp{"IPC 600"}, @samp{"Either ISO 9001 or IPC 600"},
or @samp{"Both ISO 9001 and IPC 600"}.
@end ftable
%end-doc
*/
  {N_("quality"), N_("The quality certification option"),
   HID_Enum, 0, 0, {0, 0, 0}, quality_option_names, 0},
#define HA_quality 14

/* %start-doc options "99 pcbshopper.com RFQ Options"
@ftable @code
@item --designs <int>
Where @code{<int>} is the number of independent designs on the board
separated by a silkscreen line or by nothing at all.
Please use one of the positive Integer numbers @math{ \{1, 2, 3, ...\} }.
@end ftable
%end-doc
*/
  {N_("designs"), N_("The number of independent designs on the board separated by a silkscreened line or by nothing at all"),
   HID_Integer, 0, 0, {1, 0, 0}, 0, 0},
#define HA_designs 15

/* %start-doc options "99 pcbshopper.com RFQ Options"
@ftable @code
@item --quantity <int>
Where @code{<int>} is the quantity of boards. Up to 1000.
Please use one of the positive Integer numbers @math{ \{1, 2, 3, ..., 1000\} }.
@end ftable
%end-doc
*/
  {N_("quantity"), N_("Quantities up to 1,000"),
   HID_Integer, 0, 1000, {3, 0, 0}, 0, 0},
#define HA_quantity 16

/* %start-doc options "99 pcbshopper.com RFQ Options"
@ftable @code
@cindex country
@item --country <string>
The country to ship the boards to. The parameter @code{<string>} has to
comply with the name as in ISO 3166-2.

Below are some examples to avoid common mistakes and typos:
@example
"@AA{}land Islands": try to get the accent right.
"Antigua + Barbuda": ISO uses a "+" sign.
"Congo, the Democratic Republic of the": ISO got it backwards.
"Cura@,{c}ao": more accents.
"Bosnia-Herzegovina": use of a dash "-".
"Central African Rep.": abbreviated.
"Cocos (Keeling) Islands": in between parentheses for explanation.
"C@^ote d'Ivoire": more accents.
"Holy See (Vatican)": more explanations.
"Korea, Republic of": ISO got another one backwards.
"R@'eunion": more accents.
"Saint Barth@'elemy": more accents.
"United Kingdom": please do not use "Great Britain".
"United States": please do not use "U.S.A." or "USA".
"Virgin Islands, U.S.": ISO got yet another one backwards.
@end example
@end ftable
%end-doc
*/
  {N_("country"), N_("The country to ship the boards to"),
   HID_Enum, 0, 0, {0, 0, 0}, 0, 0},
#define HA_country 17

/* %start-doc options "99 pcbshopper.com RFQ Options"
@ftable @code
@cindex lead-time
@item --lead-time <string>
Where @code{<string>} is the lead time of the manufacturer.

Note that shipping time is a different thing and @code{additional} to the lead
time of the manufacturer.

PCBShopper will quote shorter lead times and faster shipping to try to
meet your schedule. 'Default time' selects the manufacturer's default
lead time.

Examples:
@example
"Default time"
"2 business days"
"3 business days"
...
"30 business days"
@end example
@end ftable
%end-doc
*/
  {N_("lead-time"), N_("The manufacturer's lead time"),
   HID_Enum, 0, 0, {0, 0, 0}, 0, 0},
#define HA_lead_time 18

};

#define NUM_OPTIONS (sizeof(shopper_attribute_list)/sizeof(shopper_attribute_list[0]))

REGISTER_ATTRIBUTES (shopper_attribute_list)

static HID_Attr_Val shopper_values[NUM_OPTIONS];

/* shopper variables. */
#define shopper_version = "0.1"

char *shopper_warnings = NULL;
int shopper_units_index;
int shopper_tracewidth_units_index;
int shopper_drillwidth_units_index;
int shopper_soldermask_color_index;
int shopper_copper_weight_index;
char *shopper_quote_URL = NULL;
char *shopper_param_warnings = NULL;

/* Variables for the pcbShopper API. */
char *shopper_units = NULL; /*!< Dimension measurement units. */
double shopper_width; /*!< Board width. */
double shopper_height; /*!< Board height. */
double shopper_thickness; /*!< Board thickness. */
int shopper_layers; /*!< Number of copper layers. */
int shopper_soldermask_color; /*!< Solder mask color. */
int shopper_silkscreen; /*!< Silkscreen color number. */
int shopper_copper_finish_type; /*!< Copper finish type. */
int shopper_copper_weight; /*!< Copper weight. */
double shopper_min_trace_width; /*!< Minimum trace width. */
char *shopper_tracewidth_units = NULL; /*!< Trace width dimension measurement units. */
double shopper_min_drill_size; /*!< Minimum drill size. */
char *shopper_drillwidth_units = NULL; /*!< Drill width dimension measurement units. */
int shopper_plated_gold_fingers; /*!< Number of hard gold finish parts (ENIG). */
int shopper_stencil; /*!< Stencils  requirements. */
int shopper_quality; /*!< Quality Certifications required. */
int shopper_designs; /*!< Number of Designs. */
int shopper_quantity; /*!< Quantity upto 1,000. */
char *shopper_country = NULL; /*!< Country to ship to. */
int shopper_days; /*!< Lead time in days. */

/*!
 * \brief Print a usage message and quit.
 */
void shopper_usage ()
{
  Message ("PCB shopper Request For Quotation (RFQ) Helper.\n"
    "Automatically submits an RFQ to http://www.PCBshopper.com\n");
}

/*!
 * \brief Print a disclaimer message to the log window.
 */
void shopper_disclaimer ()
{
  Message ("PCB fabrication cost estimates vary widely.\n"
    "One of the largest contributors to unit price is the quantity of"
    "PCBs being built in the production run.\n"
    "Several PCB design and layout options may be provided on the quote"
    "with various pricing options that offer a lower price.\n"
    "However, the lower price assumes a relatively large number of PCBs"
    "are being fabricated.\n"
    "For smaller volumes, or for prototyping, the cost may be the same or"
    "even higher for some of these options.\n"
    "Obtain several quotes and be wary of quick quotes that seem out of"
    "line.\n"
    "When this occurs, the PCB fabricator providing an out of line quote"
    "may classify your PCB design as advanced technology by their"
    "standards causing a relatively high quote compared to other PCB"
    "fabricators that classify your PCB design as standard technology.\n"
    "Press them hard for ways to lower the cost.\n"
    "Also be careful of quotes that are much lower than others.\n"
    "This may occur when a PCB fabricator does not understand the"
    "complexity of your PCB design.");
}

/*!
 * \brief Turn an int value into a string.
 *
 * \param \c num Int in to a string.
 *
 * \return String containing an int value.
 */
char *
int2string (int num)
{
  char *ret = NULL;

  sprintf (ret, "%d", num);
  return (ret);
}

/*!
 * \brief Turn a double value into a string.
 *
 * \param \c num Double in to a string.
 *
 * \return String containing a double value.
 */
char *
double2string (double num)
{
  char *ret = NULL;

  sprintf (ret, "%f", num);
  return (ret);
}

/*!
 * \brief Wrap string in list item tags.
 *
 * \param \c item String to wrapped in html "list item" tags.
 *
 * \return Wrapped string.
 */
char *
list_wrap (char *item)
{
  return (Concat ("<li>", item, "</li>"));
}

/*!
 * \brief Wrap string in html bold tags.
 *
 * \param \c str String to wrapped in html "bold" tags.
 *
 * \return Wrapped string.
 */
char *
string2bold (char *str)
{
  return (Concat ("<b>", str, "</b>"));
}

/*!
 * \brief Wrap string in html underlined tags.
 *
 * \param \c str String to wrapped in html "underlined" tags.
 *
 * \return Wrapped string.
 */
char *
string2underlined (char *str)
{
  return (Concat ("<u>", str, "</u>"));
}

/*!
 * \brief Assemble URL string for pcbshopper.com quote.
 *
 * \return URL string.
 */
char *
shopper_assemble_quote_URL (void)
{
  char *shopper_quote_string = NULL;
  char *stringUnits;
  char *stringTWUnits;
  char *stringDrillUnits;
  char *stringSolderMaskColor;
  char *stringFinish;
  char *stringStencil;
  char *stringQuality;
  char *stringCountry;
  char *stringDays;

  /* Create a string for units. */
  switch (shopper_units_index)
  {
    case 0:
      stringUnits = strdup ("inches");
      break;
    case 1:
      stringUnits = strdup ("cm");
      break;
    case 2:
      stringUnits = strdup ("mm");
      break;
    case 3:
      stringUnits = strdup ("mil");
      break;
    default:
      stringUnits = strdup ("err");
      break;
  }

  /* Create a string for trace width units. */
  switch (shopper_tracewidth_units_index)
  {
    case 0:
      stringTWUnits = strdup ("inches");
      break;
    case 1:
      stringTWUnits = strdup ("cm");
      break;
    case 2:
      stringTWUnits = strdup ("mm");
      break;
    case 3:
      stringTWUnits = strdup ("mil");
      break;
    default:
      stringTWUnits = strdup ("err");
      break;
  }

  /* Create a string for drill width units. */
  switch (shopper_drillwidth_units_index)
  {
    case 0:
      stringDrillUnits = strdup ("inches");
      break;
    case 1:
      stringDrillUnits = strdup ("cm");
      break;
    case 2:
      stringDrillUnits = strdup ("mm");
      break;
    case 3:
      stringDrillUnits = strdup ("mil");
      break;
    default:
      stringDrillUnits = strdup ("err");
      break;
  }

  /* Create a string for soldermask color. */
  switch (shopper_soldermask_color_index)
  {
    case 0:
      stringSolderMaskColor = strdup ("nomask");
      break;
    case 1:
      stringSolderMaskColor = strdup ("cheapest");
      break;
    case 2:
      stringSolderMaskColor = strdup ("black");
      break;
    case 3:
      stringSolderMaskColor = strdup ("blue");
      break;
    case 4:
      stringSolderMaskColor = strdup ("green");
      break;
    case 5:
      stringSolderMaskColor = strdup ("purple");
      break;
    case 6:
      stringSolderMaskColor = strdup ("red");
      break;
    case 7:
      stringSolderMaskColor = strdup ("white");
      break;
    case 8:
      stringSolderMaskColor = strdup ("yellow");
      break;
    default:
      stringSolderMaskColor = strdup ("err");
      break;
  }

  /* Create a string for copper surface finish. */
  switch (shopper_copper_finish_type)
  {
    case 0:
      stringFinish = strdup ("cheapest");
      break;
    case 1:
      stringFinish = strdup ("cheapestLF");
      break;
    case 2:
      stringFinish = strdup ("HASL");
      break;
    case 3:
      stringFinish = strdup ("Lead-free HASL");
      break;
    case 4:
      stringFinish = strdup ("ENIG");
      break;
    default:
      stringFinish = strdup ("err");
      break;
  }

  /* Create a string for quality certificates. */
  switch (shopper_quality)
  {
    case 0:
      stringQuality = strdup ("Any");
      break;
    case 2:
      stringQuality = strdup ("iso9001");
      break;
    case 3:
      stringQuality = strdup ("ipc600");
      break;
    case 4:
      stringQuality = strdup ("either");
      break;
    case 5:
      stringQuality = strdup ("both");
      break;
    default:
      stringQuality = strdup ("err");
      break;
  }

  /* Create a string for days lead time. */
  if (shopper_days > 0)
    {
      /* Lead time options skip 1, so we count over it. */
      stringDays = int2string (shopper_days + 1);
    }
  else
    {
      stringDays = "default";
    }

  sprintf (shopper_quote_string, "http://pcbshopper.com/?Width=%f&Height=%f&Units=%s&"
    "Layers=%d&Color=%s&Silkscreen=%d&Finish=%s&Cu=%d&Trace=%f&"
    "TWUnits=%s&Drill=%f&DrillUnits=%s&Fingers=%d&Stencil=%d&"
    "Quality=%s&Designs=%d&Quantity=%d&Country=%s&days=%s",
    shopper_width, shopper_height, stringUnits, shopper_layers,
    stringSolderMaskColor, shopper_silkscreen, stringFinish,
    shopper_copper_weight_index, shopper_min_trace_width, stringTWUnits,
    shopper_min_drill_size, stringDrillUnits, shopper_plated_gold_fingers,
    shopper_stencil, stringQuality, shopper_designs,
    shopper_quantity, shopper_country, stringDays);

  shopper_quote_string = Concat ("<a href=\"", shopper_quote_string, "\"> Click Here To Open PCBShopper.com</a>");

  return (shopper_quote_string);
}

/*!
 * \brief Calculate PCB parameters required for quote.
 *
 * Currently calculates (in order):
 * <ol>
 *   <li>Unrouted nets.</li>
 *   <li>Find out what Units pcb currently is using.</li>
 *   <li>Copper layer count.</li>
 *   <li>Silkscreen layer usage.</li>
 *   <li>Outer board dimensions.</li>
 *   <li>Minimum trace width.</li>
 *   <li>Smallest drill size.</li>
 *   <li>Hard gold finish objects.</li>
 *   <li>Defauls for solder mask color, copper finish type, copper weight,
 *       stencil, quality options, design number, country and speed.</li>
 * </ol>
 * Everything is stored into pcbShopper global variables, mm where
 * applicable.
 */
void
shopper_calculate_params (void)
{
  int tSilk = false; /* Set Top silk flag to false. */
  int bSilk = false; /* Set Bottom silk flag to false. */
  int tFinish = false; /* Set Top finish flag to false. */
  int bFinish = false; /* Set Bottom finish flag to false. */
  double smallestTrace; /* Will lose to any other real trace width. */
  Coord smallestHole; /* Will lose to any other real hole diameter. */
  Coord xMin; /* Minimum X-coordinate. */
  Coord xMax; /* Maximum X-coordinate. */
  Coord yMin; /* Minimum Y-coordinate. */
  Coord yMax; /* Maximum Y-coordinate. */
  int i; /* An iter. */
  int outline_layer_found = false; /* Flag. */

  /* Find unrouted nets.
   * If found, we are not finished with the layout yet ! */
  if (PCB->Data->RatN != 0)
    {
      Message (_("shopper: found unrouted nets, you haven't finished yet !\n"));
    }

  /* Copper layer count.
   * Loop over each layer(group) and count copper layers. */
  shopper_layers = 0;
  for (i = 0; i < MAX_GROUP; i++)
  {
    GROUP_LOOP (PCB->Data, i)
    {
      if (layer[number].Type == LT_COPPER)
      {
        shopper_layers++;
      }
    }
    END_LOOP;
  }
  Message (Concat (N_("shopper: number of layer groups containing a copper layer = %f\n"), shopper_layers));

  /* Silkscreen layer usage.
   * Test for silkscreen on Top, Bottom or both sides. */
  if ((PCB->Data->Layer[bottom_silk_layer].ArcN >= 0)
    || (PCB->Data->Layer[bottom_silk_layer].Polygon >= 0)
    || (PCB->Data->Layer[bottom_silk_layer].LineN >= 0)
    || (PCB->Data->Layer[bottom_silk_layer].TextN >= 0))
  {
    bSilk = true;
    Message (_("shopper: this pcb seems to have entities on the the bottom silkscreen layer\n"));
  }
  if ((PCB->Data->Layer[top_silk_layer].ArcN >= 0)
    || (PCB->Data->Layer[top_silk_layer].Polygon >= 0)
    || (PCB->Data->Layer[top_silk_layer].LineN >= 0)
    || (PCB->Data->Layer[top_silk_layer].TextN >= 0))
  {
    tSilk = true;
    Message (_("shopper: this pcb seems to have entities on the the top silkscreen layer\n"));
  }

  /* Find the dimensions of the pcb.
   * Get the board dimensions from the PCB data (editable area). */
  shopper_width = COORD_TO_MM (PCB->MaxWidth);
  shopper_height = COORD_TO_MM (PCB->MaxHeight);
  shopper_units_index = 2;
  Message (Concat (N_("shopper: maximum width = %f\n"),
    shopper_width));
  Message (Concat (N_("shopper: maximum height = %f\n"),
    shopper_height));
  Message (Concat (N_("shopper: dimensions index %d\n"),
    shopper_units_index));

  /* Get the minimum/maximum board dimensions from the outline layer. */
  xMin = 0.0;
  xMax = 0.0;
  yMin = 0.0;
  yMax = 0.0;
  /* Loop through all copper layers. */
  LAYER_LOOP (PCB->Data, max_copper_layer)
  {
  /* Find the "outline" layer. */
    if (strcmp (layer[n].Name, "outline") == 0)
    {
      outline_layer_found = true;
    }
  }
  END_LOOP;
  if (outline_layer_found == false)
  {
    Message (_("shopper: this pcb seems to have no outline layer !\n"));
  }
  else
  {
    /*! \todo test if the outline layer contains elements. */
  }

  /* Find the minimum trace width.
   * Let us assume that the smallest trace can't be wider than the
   * maximum board dimension, either width or height. */
  smallestTrace = MAX (PCB->MaxWidth, PCB->MaxHeight);
  /* Loop over all copper lines and find the minimum width (in Coord). */
  ALLLINE_LOOP (PCB->Data);
    {
      smallestTrace = MIN (line->Thickness, smallestTrace);
    }
  ENDALL_LOOP; /* ALLLINE_LOOP */
  /* Loop over copper arcs and find minimum width (in Coord). */
  ALLARC_LOOP (PCB->Data)
      {
        smallestTrace = MIN (arc->Thickness, smallestTrace);
      }
  ENDALL_LOOP; /* ALLARC_LOOP */
  /* Translate the smallest trace width to mm. */
  shopper_min_trace_width = COORD_TO_MM (smallestTrace);
  shopper_tracewidth_units_index = 2;
  Message (Concat (N_("shopper: minimum trace width = %f\n"),
    shopper_min_trace_width));
  Message (Concat (N_("shopper: trace width dimension index %d\n"),
    shopper_tracewidth_units_index));

  /* Find the smallest drill size.
   * Let us assume that the smallest drill hole can't be wider than the
   * maximum board dimension, either width or height. */
  smallestHole = MAX (PCB->MaxWidth, PCB->MaxHeight);
  /* Loop over all pins and find the minimum drill hole diameter (in
   * Coord). */
  ALLPIN_LOOP (PCB->Data)
    {
      smallestHole = MIN (pin->DrillingHole, smallestHole);
    }
  ENDALL_LOOP; /* ALLPIN_LOOP */
  Message (Concat (N_("shopper: smallest drill hole diameter = %f\n"),
    smallestHole));

  /* Find any hard gold finish objects.
   * Basically we are looking on the outer layers for:
   * - fingers: probably an ElementPad near the board edge with the
   *   attribute "finish" set to "ENIG",
   * - fiducials: round pad with the attribute "finish" set to "ENIG".
   * One of those will set the tFinish or bFinish flags. */
  ALLPAD_LOOP (PCB->Data)
    {
      /*! \todo Add code for ENIG fingers and other plated objects. */
    }
  ENDALL_LOOP; /* ALLPAD_LOOP */
  Message (Concat (N_("shopper: found %d ENIG fingers or plated objects\n"),
    shopper_plated_gold_fingers));

  /* Defaults for solder mask color, copper finish type,
   * copper weight, stencil, quality options, design number, country and
   * lead time for delivery in days. */
  shopper_soldermask_color_index = 0; /* "nomask". */
  shopper_copper_finish_type = 0; /* "cheapest". */
  shopper_copper_weight_index = 0; /* "1oz". */
  shopper_stencil = 0; /* "No" stencil required. */
  shopper_quality = 0; /* "Any" check. */
  shopper_designs = 1; /* Always at least one design.*/
  shopper_quantity = 3; /* So OSH Park is always an option ;-) */
  shopper_country = "US"; /* Assumed "United States". */
  shopper_days = 0; /* We wants to have it __now__. */
}

/*!
 * \brief Get export options.
 */
static HID_Attribute *
shopper_get_export_options (int *n)
{
  static int last_unit_value = -1;

  if (shopper_attribute_list[HA_unit].default_val.int_value == last_unit_value)
    {
      if (Settings.grid_unit)
        shopper_attribute_list[HA_unit].default_val.int_value = Settings.grid_unit->index;
      else
        shopper_attribute_list[HA_unit].default_val.int_value = get_unit_struct ("mil")->index;
      last_unit_value = shopper_attribute_list[HA_unit].default_val.int_value;
    }

  if (n)
    {
      *n = NUM_OPTIONS;
    }

  return shopper_attribute_list;
}

void
shopper_hid_export_to_file (FILE * the_file, HID_Attr_Val * options)
{
}

/*!
 * \brief Main shopper function.
 *
 * Upon invoking the shopper exporter the following is done:\n
 * <ol>
 *   <li>Print a message in the log window informing the user what is
 *     currently done.</li>
 *   <li>Find or calculate parameters of the current pcb.\n
 *     Report findings.</li>
 *   <li>Compose a Request for Quotation URL from the collected data.</li>
 *   <li>Print any warnings in the log window.<li>
 *   <li>Print the Request for Quotation URL.</li>
 * </ol>
 */
static void
shopper_do_export (HID_Attr_Val * options)
{
  Message (_("shopper is identifying current PCB design constraints..."));

  /* Check PCB for all relevant design constraints. */
  shopper_calculate_params();

  shopper_quote_URL = shopper_assemble_quote_URL ();

  /* Check if any warnings were generated during parameter calculations. */
  if (shopper_param_warnings)
    {
      /* Generate a warning string for log window. */
      Message (Concat (N_("shopper: warnings generated during parameter calculations:\n"),
        shopper_param_warnings));
    }

  sprintf ("%s\n", shopper_quote_URL);
}

/*!
 * \brief Parse arguments.
 */
static void
shopper_parse_arguments (int *argc, char ***argv)
{
  hid_register_attributes (shopper_attribute_list,
    sizeof (shopper_attribute_list) / sizeof (shopper_attribute_list[0]));
  hid_parse_command_line (argc, argv);
}

HID shopper_hid;

void
hid_shopper_init ()
{
  memset (&shopper_hid, 0, sizeof (shopper_hid));

  common_nogui_init (&shopper_hid);

  shopper_hid.struct_size         = sizeof (shopper_hid);
  shopper_hid.name                = "shopper";
  shopper_hid.description         = "PCBshopper export";
  shopper_hid.exporter            = 1;

  shopper_hid.get_export_options  = shopper_get_export_options;
  shopper_hid.do_export           = shopper_do_export;
  shopper_hid.parse_arguments     = shopper_parse_arguments;

  hid_register_hid (&shopper_hid);
}
