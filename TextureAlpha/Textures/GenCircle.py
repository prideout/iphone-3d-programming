#!/usr/bin/python

import cairo
import os
import math

def renderCircle(s):
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, s, s)
    cr = cairo.Context(surface)
    cr.scale(s,s)

    cr.set_source_rgb(1,1,1)

    cr.translate(0.5, 0.5)
    cr.arc(0, 0, 0.4, 0, 2 * math.pi)
    cr.fill()

    surface.write_to_png("Circle.png")

renderCircle(16)
os.system("open Circle.png")
