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
;;; You should have received a copy of the GNU General Public License
;;; along with this program; if not, write to the Free Software
;;; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

;; PCB forward annotation script

(use-modules (ice-9 format))

;; This is a list of attributes which are propogated to the pcb
;; elements.  Note that refdes, value, and footprint need not be
;; listed here.
(define pcblf:element-attrs
  '("device"
    "manufacturer"
    "manufacturer_part_number"
    "vendor"
    "vendor_part_number"
    ))

(define (pcblf:pinfmt pin)
  (format #f "~a-~a" (car pin) (car (cdr pin)))
  )

(define (pcblf:each-pin net pins port)
  (if (not (null? pins))
      (let ((pin (car pins)))
	(format port "Netlist(Add,~a,~a,1)~%" net (pcblf:pinfmt pin))
	(pcblf:each-pin net (cdr pins) port))))

(define (pcblf:each-net netnames port)
  (if (not (null? netnames))
      (let ((netname (car netnames)))
	(pcblf:each-pin netname (gnetlist:get-all-connections netname) port)
	(pcblf:each-net (cdr netnames) port))))

(define (pcblf:each-attr refdes attrs port)
  (if (not (null? attrs))
      (let ((attr (car attrs)))
	(format port "ElementSetAttr(~a,~a,~a)~%"
		refdes
		attr
		(gnetlist:get-package-attribute refdes attr))
	(pcblf:each-attr refdes (cdr attrs) port))))

;; write out the pins for a particular component
(define pcblf:component_pins
  (lambda (port package pins)
    (if (and (not (null? package)) (not (null? pins)))
	(begin
	  (let (
		(pin (car pins))
		(label #f)
		(pinnum #f)
		)
	    (display "ChangePinName(" port)
	    (display package port)
	    (display ", " port)

	    (set! pinnum (gnetlist:get-attribute-by-pinnumber package pin "pinnumber"))

	    (display pinnum port)
	    (display ", " port)

	    (set! label (gnetlist:get-attribute-by-pinnumber package pin "pinlabel"))
	    (if (string=? label "unknown") 
		(set! label pinnum)
		)
	    (display label port)
	    (display ")\n" port)
	    )
	  (pcblf:component_pins port package (cdr pins))
	  )
	)
    )
  )

(define (pcblf:each-element elements port)
  (if (not (null? elements))
      (let* ((refdes (car elements))
	     (value (gnetlist:get-package-attribute refdes "value"))
	     (footprint (gnetlist:get-package-attribute refdes "footprint"))
	     )

	(format port "ElementAddIf(~a,~a,~a)~%" refdes footprint value)
	(pcblf:each-attr refdes pcblf:element-attrs port)
	(pcblf:component_pins port refdes (gnetlist:get-pins refdes))

	(pcblf:each-element (cdr elements) port))))

(define (pcblf output-filename)
  (let ((port (open-output-file output-filename)))
    (format port "Netlist(Clear)\n")
    (pcblf:each-net (gnetlist:get-all-unique-nets "dummy") port)
    (format port "Netlist(Sort)\n")
    (format port "NetlistChanged()\n")
    (format port "ElementAddStart()\n")
    (pcblf:each-element packages port)
    (format port "ElementAddDone()\n")
    (close-output-port port)))
