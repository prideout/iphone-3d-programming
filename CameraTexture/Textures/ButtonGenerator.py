#!/usr/bin/python

import cairo
import os
import math

imagesize = (128,32)
viewport = (4,1)

def setColor(h, s, v):
    global cr
    rgb = hsvToRgb(h, s, v)
    cr.set_source_rgba(rgb[0], rgb[1], rgb[2], 1)

def hsvToRgb(h, s, v):
    if s == 0: return (v, v, v)
    i = int(h / 60.0) % 6
    f = (h / 60.0) - int(h / 60.0)
    p = v * (1.0 - s)
    q = v * (1.0 - (s * f))
    t = v * (1.0 - (s * (1 - f)))
    if i == 0: return (v, t, p)
    if i == 1: return (q, v, p)
    if i == 2: return (p, v, t)
    if i == 3: return (p, q, v)
    if i == 4: return (t, p, v)
    return (v, p, q)

def roundedrec(x,y,w,h,r = 10):
    global cr
    x -= w
    y -= h/2
    w *= 2
    cr.move_to(x+r,y)                      # Move to A
    cr.line_to(x+w-r,y)                    # Straight line to B
    cr.curve_to(x+w,y,x+w,y,x+w,y+r)       # Curve to C, Control points are both at Q
    cr.line_to(x+w,y+h-r)                  # Move to D
    cr.curve_to(x+w,y+h,x+w,y+h,x+w-r,y+h) # Curve to E
    cr.line_to(x+r,y+h)                    # Line to F
    cr.curve_to(x,y+h,x,y+h,x,y+h-r)       # Curve to G
    cr.line_to(x,y+r)                      # Line to H
    cr.curve_to(x,y,x,y,x+r,y)             # Curve to A

def text(str, x, y):
    global cr
    x_bearing, y_bearing, width, height = cr.text_extents(str)[:4]
    cr.move_to(x - width / 2 - x_bearing, y - height / 2 - y_bearing)
    cr.show_text(str)
    
def createSurface():
    global cr
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, *imagesize)
    scale = [x/y for x, y in zip(imagesize, viewport)]
    cr = cairo.Context(surface)
    cr.scale(*scale)
    cr.set_line_width(0.07)
    cr.set_line_cap(cairo.LINE_CAP_ROUND)
    cr.select_font_face("Helvetica", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
    cr.set_font_size(0.5)
    return surface

surface = createSurface()
roundedrec(2, 0.5, 1.9, 0.8, 0.125)
setColor(180, 0.25, 0.9)
cr.fill()
roundedrec(2, 0.5, 1.9, 0.8, 0.125)
setColor(180, 0.25, 0.5)
cr.stroke()
text("Take Picture", 2, 0.5)
surface.write_to_png("TakePicture.png")
os.system("open TakePicture.png")

surface = createSurface()
roundedrec(2, 0.5, 1.9, 0.8, 0.125)
setColor(90, 0.25, 0.9)
cr.fill()
setColor(90, 0.25, 0.5)
text("Please Wait", 2, 0.5)
surface.write_to_png("PleaseWait.png")
os.system("open PleaseWait.png")



