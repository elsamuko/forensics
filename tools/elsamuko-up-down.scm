; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 3 of the License, or
; (at your option) any later version.
; 
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
; 
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
; http://www.gnu.org/licenses/gpl-3.0.html
;
; Copyright (C) 2010 elsamuko <elsamuko@web.de>
;

(define (elsamuko-up-down aimg adraw)

  (let* ((img (car (gimp-drawable-get-image adraw)))
         (owidth (car (gimp-image-width img)))
         (oheight (car (gimp-image-height img)))
         (working-layer (car (gimp-layer-copy adraw FALSE)))
         )
    
    ; init
    (define (set-pt a index x y)
      (begin
        (aset a (* index 2) x)
        (aset a (+ (* index 2) 1) y)
        )
      )
    
    ; up down curve
    (define (spline-brightness)
      (let* ((a (cons-array 32 'byte)))
        (set-pt a  0   0   0)
        (set-pt a  1  17 255)
        (set-pt a  2  34   0)
        (set-pt a  3  51 255)
        (set-pt a  4  68   0)
        (set-pt a  5  85 255)
        (set-pt a  6 102   0)
        (set-pt a  7 119 255)
        (set-pt a  8 136   0)
        (set-pt a  9 153 255)
        (set-pt a 10 170   0)
        (set-pt a 11 187 255)
        (set-pt a 12 204   0)
        (set-pt a 13 221 255)
        (set-pt a 14 238   0)
        (set-pt a 15 255 255)
        a
        )
      )
    
    (gimp-context-push)
    (gimp-image-undo-group-start img)
    (if (= (car (gimp-drawable-is-gray adraw )) TRUE)
        (gimp-image-convert-rgb img)
        )
    
    ; set curve
    (gimp-image-add-layer img working-layer -1)
    (gimp-drawable-set-name working-layer "Analysis")
    (gimp-curves-spline working-layer HISTOGRAM-VALUE 32 (spline-brightness))
    
    ; blur to remove noise
    (plug-in-gauss 1 img working-layer 10 10 0)
    
    ; tidy up
    (gimp-image-undo-group-end img)
    (gimp-displays-flush)
    (gimp-context-pop)
    )
  )

(script-fu-register "elsamuko-up-down"
                    "_Up Down Analysis"
                    "Apply an extreme curve for forensics"
                    "elsamuko <elsamuko@web.de>"
                    "elsamuko"
                    "2012-10-08"
                    "RGB*"
                    SF-IMAGE       "Input image"          0
                    SF-DRAWABLE    "Input drawable"       0
                    )

(script-fu-menu-register "elsamuko-up-down" _"<Image>/Image")
