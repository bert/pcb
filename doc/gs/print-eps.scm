;; This file may be used to print gschem schematics from the
;; command line.  Typical usage is:
;;
;;   gschem -p -o mysch.ps -s /path/to/this/file/print.scm mysch.sch
;;
;; The schematic in "mysch.sch" will be printed to the file "mysch.ps"

; light background
;(load "/envy/dj/geda/share/gEDA/gschem-lightbg")

(output-orientation "portrait")
(output-type "extents no margins")
(output-color "enabled")
(output-text "ps")

(paper-size 0.0 0.0)

; You need call this after you call any rc file function
(gschem-use-rc-values)

; filename is specified on the command line
(gschem-postscript "dummyfilename.eps")

(gschem-exit)
