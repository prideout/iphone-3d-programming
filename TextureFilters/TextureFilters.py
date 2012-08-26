#!/usr/bin/python

import cairo
import os
import math

black = [0] * 3
white = [1] * 3

m = cairo.Matrix()
m.translate(47, 139)

trilinear = cairo.SurfacePattern(cairo.ImageSurface.create_from_png("Trilinear.png"))
trilinear.set_matrix(m)

bilinear = cairo.SurfacePattern(cairo.ImageSurface.create_from_png("Bilinear.png"))
m.translate(-320,0)
bilinear.set_matrix(m)

nearest = cairo.SurfacePattern(cairo.ImageSurface.create_from_png("Nearest.png"))
m.translate(-320,0)
nearest.set_matrix(m)

imagesize = (320, 480)
viewport = (320,480)
surface = cairo.ImageSurface(cairo.FORMAT_RGB24, *imagesize)
scale = [x/y for x, y in zip(imagesize, viewport)]
cr = cairo.Context(surface)
cr.scale(*scale)
cr.set_source(trilinear)
cr.rectangle(0, 0, 320, 480)
cr.fill()
surface.write_to_png("Default.png")

imagesize = (960, 480)
viewport = (960, 480)
surface = cairo.ImageSurface(cairo.FORMAT_RGB24, *imagesize)
scale = [x/y for x, y in zip(imagesize, viewport)]
cr = cairo.Context(surface)
cr.scale(*scale)
cr.set_source(trilinear),cr.rectangle(0, 0, 320, 480),cr.fill()
cr.set_source(bilinear),cr.rectangle(320, 0, 320, 480),cr.fill()
cr.set_source(nearest),cr.rectangle(640, 0, 320, 480),cr.fill()
surface.write_to_png("Triptych.png")
os.system("open Triptych.png")

imagesize = (480, 480)
viewport = (480, 480)
surface = cairo.ImageSurface(cairo.FORMAT_RGB24, *imagesize)
scale = [x/y for x, y in zip(imagesize, viewport)]
cr = cairo.Context(surface)
cr.scale(*scale)
cr.set_source(trilinear),cr.rectangle(0, 0, 320, 480),cr.fill()
cr.set_line_width(3), cr.set_source_rgba(1,1,1,1), cr.rectangle(55,190,45,45),cr.stroke()

m = cairo.Matrix()
m.translate(47, 139) # Get to the upper-right of the canvas
m.translate(55, 190) # Get to the zoom box
m.scale(45.0 / 160.0, 45.0 / 160.0) # Blow it up
m.translate(-320, 0) # Shift to the final position
nearest.set_matrix(m)
nearest.set_filter(cairo.FILTER_NEAREST)
cr.set_source(nearest),cr.rectangle(320, 0, 160, 160),cr.fill()

m = cairo.Matrix()
m.translate(47, 139) # Get to the upper-right of the canvas
m.translate(55, 190) # Get to the zoom box
m.scale(45.0 / 160.0, 45.0 / 160.0) # Blow it up
m.translate(-320, -160) # Shift to the final position
bilinear.set_matrix(m)
bilinear.set_filter(cairo.FILTER_NEAREST)
cr.set_source(bilinear),cr.rectangle(320, 160, 160, 160),cr.fill()

m = cairo.Matrix()
m.translate(47, 139) # Get to the upper-right of the canvas
m.translate(55, 190) # Get to the zoom box
m.scale(45.0 / 160.0, 45.0 / 160.0) # Blow it up
m.translate(-320, -320) # Shift to the final position
trilinear.set_matrix(m)
trilinear.set_filter(cairo.FILTER_NEAREST)
cr.set_source(trilinear),cr.rectangle(320, 320, 160, 160),cr.fill()

surface.write_to_png("TextureFilters.png")
os.system("open TextureFilters.png")
