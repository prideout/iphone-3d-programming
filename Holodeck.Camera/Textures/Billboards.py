#!/usr/bin/python

import cairo
import os
import math

def text(str, x, y):
    global cr
    x_bearing, y_bearing, width, height = cr.text_extents(str)[:4]
    cr.move_to(x - width / 2 - x_bearing, y - height / 2 - y_bearing)
 #   cr.show_text(str)

def Write(s):
    global cr
    imagesize = (128,64)
    viewport = (128,64)
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, *imagesize)
    scale = [x/y for x, y in zip(imagesize, viewport)]
    cr = cairo.Context(surface)
    cr.scale(*scale)
# Look in /Library/Fonts
    cr.select_font_face("Apple Chancery", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_BOLD)
    cr.set_font_size(40)
    cr.set_source_rgb(0,0,0)
    text(s, 64, 32)
    cr.set_line_width(4)
    cr.text_path(s)
    cr.set_source_rgb(0,0,0)
    cr.stroke_preserve()
    cr.set_source_rgb(1,1,1)
    cr.fill()
    surface.write_to_png(s + ".png")
    os.system("open " + s + ".png")

Write("North")
Write("South")
Write("East")
Write("West")
