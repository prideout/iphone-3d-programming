#!/usr/bin/python

import cairo
import os
import math

imagesize = (128,128)
viewport = (1,1)
surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, *imagesize)
scale = [x/y for x, y in zip(imagesize, viewport)]
cr = cairo.Context(surface)
cr.scale(*scale)

cr.set_line_width(0.05)
cr.set_line_cap(cairo.LINE_CAP_ROUND)

cr.move_to(0, 1)
cr.line_to(1, 1)
cr.line_to(0.5, 1 - math.sqrt(3.0) / 2.0)
cr.line_to(0, 1)
cr.stroke()

cr.move_to(0, 1)
cr.line_to(1, 1)
cr.line_to(1, 2)
cr.line_to(0, 2)
cr.fill()

cr.move_to(0, 1)
cr.line_to(0, -500)
cr.line_to(0.5, 1 - math.sqrt(3.0) / 2.0)
cr.fill()

cr.move_to(1, 1)
cr.line_to(1, -500)
cr.line_to(0.5, 1 - math.sqrt(3.0) / 2.0)
cr.fill()

surface.write_to_png("Triangle.png")
os.system("open Triangle.png")

pattern = cairo.SurfacePattern(surface)

margin = 20
imagesize = (128 + margin * 2, 128 + margin * 2)
viewport = (128 + margin * 2, 128 + margin * 2)

surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, *imagesize)
scale = [x/y for x, y in zip(imagesize, viewport)]
cr = cairo.Context(surface)
cr.scale(*scale)

cr.set_source_rgb(1,1,1)
cr.rectangle(0,0,1000,margin)
cr.rectangle(0,0,margin,1000)
cr.rectangle(0,128+margin,1000,margin)
cr.rectangle(128+margin,0,margin,1000)
cr.fill()

cr.translate(margin, margin)


cr.set_source(pattern)
cr.rectangle(0, 0, 128, 128)
cr.fill()

cr.set_line_width(10)
cr.set_line_cap(cairo.LINE_CAP_ROUND)


cr.scale(128, 128)

def dot(x, y):
    global cr
    cr.set_line_width(10.0/128.0)
    #cr.set_source_rgb(1, 0, 0)
    cr.set_source_rgb(0.5, 0.5, 0.5)
    cr.move_to(x,y), cr.line_to(x,y), cr.stroke()
    cr.set_source_rgb(0, 0, 0)
    cr.set_line_width(5.0/128.0)
    cr.move_to(x,y), cr.line_to(x,y), cr.stroke()

dot(0,1)
dot(1,1)
dot(0.5, 1 - math.sqrt(3.0) / 2.0)


surface.write_to_png("TriangleFigure.png")
os.system("open TriangleFigure.png")
