#!/usr/bin/python

import cairo
import os
import math

def text(str, x, y):
    global cr
    x_bearing, y_bearing, width, height = cr.text_extents(str)[:4]
    cr.move_to(x, y - height / 2 - y_bearing)
    cr.show_text(str)

def trim(infile, outfile):
    sourceSurface = cairo.ImageSurface.create_from_png(infile)
    destSurface = cairo.ImageSurface(cairo.FORMAT_ARGB32, 320, 480)
    original = cairo.SurfacePattern(sourceSurface)
    original.set_matrix
    cr = cairo.Context(destSurface)
    matrix = cairo.Matrix()
    matrix.translate(47,139)
    original.set_matrix(matrix)
    cr.set_source(original)
    cr.rectangle(0, 0, 320, 480)
    cr.fill()
    sourceSurface = destSurface
    sourceSurface.write_to_png(outfile)

trim("SrcAlpha.png", "Trimmed.SrcAlpha.png")
trim("One.png", "Trimmed.One.png")

SrcAlpha = cairo.SurfacePattern(cairo.ImageSurface.create_from_png("Trimmed.SrcAlpha.png"));

One = cairo.SurfacePattern(cairo.ImageSurface.create_from_png("Trimmed.One.png"));
matrix = cairo.Matrix()
matrix.translate(0, -320)
One.set_matrix(matrix)

imagesize = (640,855)
viewport = (640,855)
surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, *imagesize)
scale = [x/y for x, y in zip(imagesize, viewport)]
cr = cairo.Context(surface)
cr.scale(*scale)

# Look in /Library/Fonts
cr.select_font_face("Trebuchet MS", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_BOLD)
cr.set_font_size(24)
cr.set_source_rgb(0, 0, 0)

text("Straight", 35, 25)
text("Premultipied", 170, 25)
cr.translate(2, 52)

cr.set_source_rgba(0, 0, 0, 1)
cr.rectangle(-2, -2, 324, 804)
cr.fill()

cr.set_source(One)
cr.rectangle(0, 0, 320, 800)
cr.fill()
cr.set_source(SrcAlpha)
cr.rectangle(0, 0, 320, 480)
cr.fill()



def renderLabel(y, a, b, c):
    text(a, 330, y - 20)
    text(b, 330, y)
    text(c, 330, y + 20)

cr.set_source_rgb(0, 0, 0)
renderLabel(80 + 160 * 0, "", "Blending Disabled", "")
renderLabel(80 + 160 * 1, "",  "Gray Background",  "sfactor = GL_SRC_ALPHA")
renderLabel(80 + 160 * 2, "",  "White Background",  "sfactor = GL_SRC_ALPHA")
renderLabel(80 + 160 * 3, "",  "Gray Background",  "sfactor = GL_ONE")
renderLabel(80 + 160 * 4, "",  "White Background",  "sfactor = GL_ONE")

surface.write_to_png("TextureAlpha.png")
os.system("open TextureAlpha.png")
