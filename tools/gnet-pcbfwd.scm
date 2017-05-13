;;; gEDA - GPL Electronic Design Automation
;;; gnetlist - gEDA Netlist
;;; Copyright (C) 1998-2008 Ales Hvezda
;;; Copyright (C) 1998-2008 gEDA Contributors (see ChangeLog for details)
;;;
;;; This program is free software; you can redistribute it and/or modify
;;; it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation; either version 2 of the License, or
;;; (at your option) any later version.
;;;
;;; This program is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.
;;;
;;; You should have received a copy of the GNU General Public License along
;;; with this program; if not, write to the Free Software Foundation, Inc.,
;;; 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

;; PCB forward annotation script

(use-modules (ice-9 regex))
(use-modules (ice-9 format))

;; This is a list of attributes which are propogated to the pcb
;; elements.  Note that refdes, value, and footprint need not be
;; listed here.
(define pcbfwd:element-attrs
  '("device"
    "manufacturer"
    "manufacturer_part_number"
    "vendor"
    "vendor_part_number"
    ))

(define (pcbfwd:quote-string s)
  (string-append "\""
		 (regexp-substitute/global #f "\"" s 'pre "\\\"" 'post)
		 "\"")
  )

(define (pcbfwd:pinfmt pin)
  (format #f "~a-~a" (car pin) (car (cdr pin)))
  )

(define (pcbfwd:each-pin net pins port)
  (if (not (null? pins))
      (let ((pin (car pins)))
	(format port "Netlist(Add,~a,~a)~%" net (pcbfwd:pinfmt pin))
	(pcbfwd:each-pin net (cdr pins) port))))

(define (pcbfwd:each-net netnames port)
  (if (not (null? netnames))
      (let ((netname (car netnames)))
	(pcbfwd:each-pin netname (gnetlist:get-all-connections netname) port)
	(pcbfwd:each-net (cdr netnames) port))))

(define (pcbfwd:each-attr refdes attrs port)
  (if (not (null? attrs))
      (let ((attr (car attrs)))
	(format port "ElementSetAttr(~a,~a,~a)~%"
		(pcbfwd:quote-string refdes)
		(pcbfwd:quote-string attr)
		(pcbfwd:quote-string (gnetlist:get-package-attribute refdes attr)))
	(pcbfwd:each-attr refdes (cdr attrs) port))))

;; write out the pins for a particular component
(define pcbfwd:component_pins
  (lambda (port package pins)
    (if (and (not (null? package)) (not (null? pins)))
	(begin
	  (let (
		(pin (car pins))
		(label #f)
		(pinnum #f)
		)
	    (display "ChangePinName(" port)
	    (display (pcbfwd:quote-string package) port)
	    (display ", " port)

	    (set! pinnum (gnetlist:get-attribute-by-pinnumber package pin "pinnumber"))

	    (display pinnum port)
	    (display ", " port)

	    (set! label (gnetlist:get-attribute-by-pinnumber package pin "pinlabel"))
	    (if (string=? label "unknown") 
		(set! label pinnum)
		)
	    (display (pcbfwd:quote-string label) port)
	    (display ")\n" port)
	    )
	  (pcbfwd:component_pins port package (cdr pins))
	  )
	)
    )
  )

(define (pcbfwd:each-element elements port)
  (if (not (null? elements))
      (let* ((refdes (car elements))
	     (value (gnetlist:get-package-attribute refdes "value"))
	     (footprint (gnetlist:get-package-attribute refdes "footprint"))
	     )

	(format port "ElementList(Need,~a,~a,~a)~%"
		(pcbfwd:quote-string refdes)
		(pcbfwd:quote-string footprint)
		(pcbfwd:quote-string value))
	(pcbfwd:each-attr refdes pcbfwd:element-attrs port)
	(pcbfwd:component_pins port refdes (gnetlist:get-pins refdes))

	(pcbfwd:each-element (cdr elements) port))))

(define (pcbfwd output-filename)
  (let ((port (open-output-file output-filename)))
    (format port "Netlist(Freeze)\n")
    (format port "Netlist(Clear)\n")
    (pcbfwd:each-net (gnetlist:get-all-unique-nets "dummy") port)
    (format port "Netlist(Sort)\n")
    (format port "Netlist(Thaw)\n")
    (format port "ElementList(Start)\n")
    (pcbfwd:each-element packages port)
    (format port "ElementList(Done)\n")
    (close-output-port port)))
