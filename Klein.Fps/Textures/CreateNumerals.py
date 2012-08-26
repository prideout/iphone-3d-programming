#!/usr/bin/python

import cairo
import os
from PIL import Image

# Create a Cairo image surface:
imagesize = (256,32)
surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, *imagesize)
cr = cairo.Context(surface)
padding = 3

# Choose a font (look in /Library/Fonts) and set up the transforms.
cr.select_font_face("Apple Chancery", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_BOLD)
cr.set_font_size(32)
cr.set_source_rgb(1,1,1)

# Create a string for appending the glyph metrics to the texture file:
glyphs = '''
struct GlyphPosition {
    int X;
    int Y;
};\n
struct GlyphMetrics {
    int XBearing;
    int YBearing;
    int Width;
    int Height;
    int XAdvance;
    int YAdvance;
};\n
struct Glyph {
    GlyphPosition Position;
    GlyphMetrics Metrics;
};\n
static const Glyph NumeralGlyphs[] = {\n'''

# Render glyphs '0' through '9' and write out their extents:
x, y = 0, 0
for character in '0123456789':
    extents = cr.text_extents(character)
    x_bearing, y_bearing, width, height, x_advance, y_advance = extents
    glyphs += '    {{ %d, %d }, ' % (x, y)
    glyphs += '{ %d, %d, %d, %d, %d, %d }},\n' % extents
    cr.save()
    cr.translate(x,-y_bearing)
    cr.text_path(character)
    cr.fill()
    cr.restore()
    x += width + padding
glyphs += '};\n'

# Extract the alpha channel and open it up for a quick preview:
surface.write_to_png("NumeralsTexture.png")
image = Image.open("NumeralsTexture.png")
image.load()
image.split()[3].save("NumeralsTexture.png")
os.system("open NumeralsTexture.png")

# Serialize the image data to a C header file:
os.system('PVRTexTool -h -yflip1 -fOGL8 -iNumeralsTexture.png')

# Write to the header file:
headerFile = open('NumeralsTexture.h', 'a')
headerFile.write(glyphs)
headerFile.close()
