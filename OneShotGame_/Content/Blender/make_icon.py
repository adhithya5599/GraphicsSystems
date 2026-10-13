"""
	Generates One Shot's application icon: the same World Archery target face as the game's target texture,
	with an arrow stuck in the gold.

	Usage, from the solution's directory (with a normal Python that has numpy and Pillow installed):
		python OneShotGame_/Content/Blender/make_icon.py "OneShotGame_/OneShotGame/Resource Files/target.ico"

	A Windows icon file holds several pictures of different sizes, and Windows picks the one closest to what it needs
	(16 x 16 for the title bar, 32 x 32 or 48 x 48 for the taskbar and desktop, up to 256 x 256 for large Explorer views).
	Each size is drawn separately rather than shrinking one big picture,
	because ten rings and an arrow can't survive being shrunk to 16 pixels:
		* 48 pixels and up: all ten rings with the thin lines between them, and the arrow
		* 32 pixels: the five colors (each color is two rings) and a simpler arrow
		* 16 and 24 pixels: just the five colors
	Every size has a thin dark outline so that the white outer ring doesn't disappear against a white background.
"""
import io
import math
import struct
import sys

import numpy as np
from PIL import Image, ImageDraw

# The ring colors, from the outside in (the same as the target texture in make_textures.py)
RING_COLORS = [ ( 245, 245, 245 ), ( 30, 30, 30 ), ( 0, 160, 226 ), ( 228, 32, 36 ), ( 255, 214, 0 ) ]
LINE_DARK = ( 40, 40, 40 )
LINE_LIGHT = ( 235, 235, 235 )
OUTLINE = ( 70, 70, 75 )
# The arrow's colors (the same as the arrow mesh in build_archery_range.py)
SHAFT = ( 40, 40, 45 )
COCK_VANE = ( 255, 120, 20 )
VANE = ( 245, 245, 245 )

SUPERSAMPLING = 8

def DrawFace( i_size, i_showAllRings, i_arrow ):
	""" Draws the icon at i_size x i_size pixels (drawn bigger and then shrunk, which smooths the edges) """
	size = i_size * SUPERSAMPLING
	image = Image.new( "RGBA", ( size, size ), ( 0, 0, 0, 0 ) )
	draw = ImageDraw.Draw( image )
	center = size / 2
	# The face fills the icon, minus the outline
	outlineWidth = max( 1.0, i_size / 32.0 ) * SUPERSAMPLING
	radius = size / 2 - outlineWidth
	draw.ellipse( [ center - radius - outlineWidth, center - radius - outlineWidth, center + radius + outlineWidth, center + radius + outlineWidth ],
		fill = OUTLINE + ( 255, ) )
	ringCount = 10
	for ring in range( ringCount ):
		# (From the outside in: rings 0 and 1 are white, 2 and 3 black, and so on)
		r = radius * ( ringCount - ring ) / ringCount
		draw.ellipse( [ center - r, center - r, center + r, center + r ], fill = RING_COLORS[ring // 2] + ( 255, ) )
	if i_showAllRings:
		# Thin lines between the rings (white between the two black rings, dark everywhere else), and the inner "X" ring
		lineWidth = max( 1, int( round( radius * 0.012 ) ) )
		for ring in range( 1, ringCount ):
			r = radius * ( ringCount - ring ) / ringCount
			color = LINE_LIGHT if ring == 3 else LINE_DARK
			draw.ellipse( [ center - r, center - r, center + r, center + r ], outline = color + ( 255, ), width = lineWidth )
		r = radius * 0.05
		draw.ellipse( [ center - r, center - r, center + r, center + r ], outline = LINE_DARK + ( 255, ), width = lineWidth )
	if i_arrow:
		DrawArrow( draw, center, radius, i_arrow == "detailed" )
	return image.resize( ( i_size, i_size ), Image.LANCZOS )

def DrawArrow( io_draw, i_center, i_radius, i_isDetailed ):
	"""
		An arrow that hit just above and to the left of the middle, seen from the front and a little to the side,
		so its shaft comes out of the target toward the lower right and ends in its fletching
	"""
	tip = ( i_center - i_radius * 0.08, i_center - i_radius * 0.1 )
	direction = ( math.cos( math.radians( 38 ) ), math.sin( math.radians( 38 ) ) )
	length = i_radius * 0.95
	nock = ( tip[0] + direction[0] * length, tip[1] + direction[1] * length )
	shaftWidth = i_radius * ( 0.055 if i_isDetailed else 0.08 )
	# A darker shadow on the face under the shaft makes it read as sticking out
	shadowOffset = i_radius * 0.04
	io_draw.line( [ ( tip[0] + shadowOffset, tip[1] + shadowOffset * 1.5 ), ( nock[0] + shadowOffset, nock[1] + shadowOffset * 1.5 ) ],
		fill = ( 0, 0, 0, 90 ), width = int( shaftWidth ) )
	io_draw.line( [ tip, nock ], fill = SHAFT + ( 255, ), width = int( shaftWidth ) )
	# The fletching: three vanes near the nock, one of them (the "cock vane") orange like the game's arrow
	perpendicular = ( -direction[1], direction[0] )
	vaneLength = i_radius * 0.28
	vaneWidth = i_radius * ( 0.12 if i_isDetailed else 0.15 )
	vaneStart = ( nock[0] - direction[0] * ( vaneLength + i_radius * 0.04 ), nock[1] - direction[1] * ( vaneLength + i_radius * 0.04 ) )
	vaneEnd = ( nock[0] - direction[0] * i_radius * 0.04, nock[1] - direction[1] * i_radius * 0.04 )
	for side, color in ( ( 1, COCK_VANE ), ( -1, VANE ) ):
		outer = ( vaneEnd[0] + perpendicular[0] * vaneWidth * side, vaneEnd[1] + perpendicular[1] * vaneWidth * side )
		io_draw.polygon( [ vaneStart, vaneEnd, outer ], fill = color + ( 255, ), outline = SHAFT + ( 255, ) )
	# The nock
	r = shaftWidth * 0.7
	io_draw.ellipse( [ nock[0] - r, nock[1] - r, nock[0] + r, nock[1] + r ], fill = COCK_VANE + ( 255, ) )

def ToIconBitmap( i_image ):
	"""
		The classic format for one picture in an icon: a 32-bit bitmap (blue, green, red, alpha, bottom row first)
		followed by a 1-bit transparency mask (which 32-bit icons don't need, but which must still be there)
	"""
	width, height = i_image.size
	pixels = np.asarray( i_image, dtype = np.uint8 )
	bgra = pixels[::-1, :, [ 2, 1, 0, 3 ]].tobytes()
	maskRowBytes = ( ( width + 31 ) // 32 ) * 4
	mask = bytes( maskRowBytes * height )
	# (The header's height is doubled because it counts the mask's rows too)
	header = struct.pack( "<IiiHHIIiiII", 40, width, height * 2, 1, 32, 0, len( bgra ) + len( mask ), 0, 0, 0, 0 )
	return header + bgra + mask

def ToPng( i_image ):
	stream = io.BytesIO()
	i_image.save( stream, format = "PNG" )
	return stream.getvalue()

def WriteIcon( i_path, i_images ):
	""" Writes an .ico file: a header, a directory entry for every picture, and then the pictures """
	# (The largest picture is stored as PNG to keep the file small; Windows has read PNG icons since Vista)
	datas = [ ToPng( image ) if image.size[0] >= 256 else ToIconBitmap( image ) for image in i_images ]
	header = struct.pack( "<HHH", 0, 1, len( i_images ) )
	offset = len( header ) + 16 * len( i_images )
	directory = b""
	for image, data in zip( i_images, datas ):
		width, height = image.size
		# (A size of 256 is written as 0 because it doesn't fit in a byte)
		directory += struct.pack( "<BBBBHHII", width % 256, height % 256, 0, 0, 1, 32, len( data ), offset )
		offset += len( data )
	with open( i_path, "wb" ) as iconFile:
		iconFile.write( header + directory + b"".join( datas ) )

if __name__ == "__main__":
	outputPath = sys.argv[1]
	images = [
		DrawFace( 16, False, None ),
		DrawFace( 24, False, None ),
		DrawFace( 32, False, "simple" ),
		DrawFace( 48, True, "detailed" ),
		DrawFace( 64, True, "detailed" ),
		DrawFace( 256, True, "detailed" ),
	]
	WriteIcon( outputPath, images )
	print( "wrote", outputPath, "with sizes", [ image.size[0] for image in images ] )
