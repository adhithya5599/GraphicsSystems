"""
	Generates One Shot's textures as 24-bit BMPs (the format TextureBuilder reads).

	Usage, from the solution's directory (with a normal Python that has numpy and Pillow installed):
		python OneShotGame_/Content/Blender/make_textures.py OneShotGame_/Content/Textures

	Every texture is generated from code so that it can be tweaked and regenerated.
	(The scoreboard's layout is in scoreboard_layout.py, next to this file, because build_archery_range.py needs it too.)
	The repeating textures (grass and wood) are built only from waves that repeat a whole number of times
	across the image, so their left/right and top/bottom edges match and no seams show when they tile.
	The colors are authored in sRGB like a painted texture would be; the engine converts them to linear for lighting.
"""
import os
import sys
import numpy as np
from PIL import Image

outputDirectory = sys.argv[1]
os.makedirs( outputDirectory, exist_ok = True )
rng = np.random.default_rng( 6320 )
size = 512
y, x = np.mgrid[0:size, 0:size] / size

def TileableNoise( i_frequencies, i_waveCount = 24, i_stretchX = 1.0 ):
	""" Sums random waves with whole-number frequencies, so the result repeats exactly across the image """
	result = np.zeros( ( size, size ) )
	for _ in range( i_waveCount ):
		fx = rng.integers( i_frequencies[0], i_frequencies[1] + 1 )
		# Waves in every direction (a random sign on one axis), so the pattern has no diagonal bias
		fy = rng.integers( i_frequencies[0], i_frequencies[1] + 1 ) * rng.choice( [ -1, 1 ] )
		fx = max( 1, int( round( fx / i_stretchX ) ) )
		phase = rng.uniform( 0, 2 * np.pi )
		result += np.cos( 2 * np.pi * ( fx * x + fy * y ) + phase ) / np.sqrt( fx * fx + fy * fy )
	result -= result.min()
	return result / result.max()

def Save( i_name, i_rgb ):
	image = Image.fromarray( np.clip( i_rgb, 0, 255 ).astype( np.uint8 ), "RGB" )
	image.save( os.path.join( outputDirectory, i_name ) )
	print( "wrote", i_name )

# Grass
#------
# Large soft patches of lighter and darker green plus fine streaks (blades) that run mostly one way
if True:
	patches = TileableNoise( ( 1, 4 ) )
	detail = TileableNoise( ( 12, 40 ), 40 )
	blades = TileableNoise( ( 30, 90 ), 60, i_stretchX = 4.0 )
	darkGreen = np.array( [ 70, 112, 46 ] )
	lightGreen = np.array( [ 118, 158, 70 ] )
	amount = ( 0.35 * patches + 0.35 * detail + 0.3 * blades )[..., None]
	grass = darkGreen * ( 1 - amount ) + lightGreen * amount
	# A few dry, yellowish flecks
	flecks = ( TileableNoise( ( 40, 110 ), 50 ) > 0.86 )[..., None]
	grass = np.where( flecks, grass * 0.8 + np.array( [ 150, 150, 80 ] ) * 0.2, grass )
	Save( "grass.bmp", grass )

# Wood planks
#------------
# Four horizontal planks per tile with grain running along them and darker gaps between them.
# (Wood surfaces repeat this texture, so unlike the target texture it can't have a white corner for vertex-colored parts:
# any repeating surface would show the white square. Meshes that need both are split into a wood mesh and a vertex-colored mesh.)
if True:
	grain = TileableNoise( ( 2, 30 ), 40, i_stretchX = 12.0 )
	knots = TileableNoise( ( 3, 8 ), 10 )
	plankIndex = np.floor( y * 4 )
	plankShade = 0.9 + 0.2 * ( ( plankIndex * 0.37 ) % 1.0 )
	light = np.array( [ 196, 150, 98 ] )
	dark = np.array( [ 128, 86, 50 ] )
	amount = ( 0.7 * grain + 0.3 * knots )[..., None]
	wood = ( dark * ( 1 - amount ) + light * amount ) * plankShade[..., None]
	# The gaps between planks
	gap = ( ( y * 4 ) % 1.0 )
	wood = np.where( ( ( gap < 0.025 ) | ( gap > 0.985 ) )[..., None], wood * 0.45, wood )
	Save( "wood.bmp", wood )

# Target face
#------------
# An Olympic (World Archery) 122 cm face: ten rings, two each of white, black, blue, red, and gold,
# drawn on the straw of the target boss (the boss is 130 cm square, so the face fills 122 / 130 of it).
# The top-left corner is pure white (for the target stand's vertex-colored parts), outside of the area the boss uses.
if True:
	straw = TileableNoise( ( 8, 60 ), 50, i_stretchX = 3.0 )
	strawColor = np.array( [ 176, 150, 92 ] ) * ( 0.85 + 0.3 * straw[..., None] )
	target = strawColor.copy()
	# The boss front uses UVs from 1/16 to 1 (480 pixels), so the face is centered in that area
	bossStart = size / 16
	bossSize = size - bossStart
	centerX = centerY = bossStart + bossSize / 2
	py, px = np.mgrid[0:size, 0:size] + 0.5
	radius = np.sqrt( ( px - centerX ) ** 2 + ( py - centerY ) ** 2 ) / ( bossSize / 2 ) * ( 130.0 / 122.0 )
	ringColors = [ [ 255, 214, 0 ], [ 255, 214, 0 ], [ 228, 32, 36 ], [ 228, 32, 36 ], [ 0, 160, 226 ], [ 0, 160, 226 ],
		[ 30, 30, 30 ], [ 30, 30, 30 ], [ 245, 245, 245 ], [ 245, 245, 245 ] ]
	for ringIndex in reversed( range( 10 ) ):
		inside = radius <= ( ringIndex + 1 ) / 10.0
		target[inside] = ringColors[ringIndex]
	# Thin dividing lines between the rings (a white line between the two black rings, black lines elsewhere)
	ringPosition = radius * 10.0
	nearLine = ( np.abs( ringPosition - np.round( ringPosition ) ) < 0.035 ) & ( radius <= 1.0 ) & ( radius > 0.05 )
	lineIsWhite = ( ringPosition > 6.5 ) & ( ringPosition < 7.5 )
	target[nearLine & lineIsWhite] = [ 235, 235, 235 ]
	target[nearLine & ~lineIsWhite] = [ 40, 40, 40 ]
	# The inner "X" ring of the 10
	target[np.abs( radius - 0.05 ) < 0.004] = [ 40, 40, 40 ]
	target[: size // 16, : size // 16] = 255
	Save( "target.bmp", target )

# White
#------
# For meshes that are colored only by their vertex colors (texture * vertex color = vertex color)
Save( "white.bmp", np.full( ( 16, 16, 3 ), 255 ) )

# Scoreboard
#-----------
# The big screen's characters and labels, drawn as glowing LED dots on black (see scoreboard_layout.py for the layout).
# The dots are white so that each quad's vertex color decides its color (amber numbers, white labels, red for a miss),
# and the background is pure black because black times any vertex color is still black,
# which means every quad blends into the screen's black background without a visible edge.
if True:
	import scoreboard_layout as layout
	atlas = np.zeros( ( layout.ATLAS_HEIGHT, layout.ATLAS_WIDTH ) )
	py, px = np.mgrid[0:layout.ATLAS_HEIGHT, 0:layout.ATLAS_WIDTH] + 0.5
	dotRadius = 0.4 * layout.DOT_PIXELS
	def DrawDot( i_centerX, i_centerY ):
		# Only the pixels near the dot are touched (drawing every dot over the whole image would be slow)
		reach = layout.DOT_PIXELS // 2
		y0, y1 = int( i_centerY ) - reach, int( i_centerY ) + reach + 1
		x0, x1 = int( i_centerX ) - reach, int( i_centerX ) + reach + 1
		distance = np.sqrt( ( px[y0:y1, x0:x1] - i_centerX ) ** 2 + ( py[y0:y1, x0:x1] - i_centerY ) ** 2 )
		# A bright core with a soft edge and a faint glow around it, like a real LED
		core = np.clip( dotRadius + 0.5 - distance, 0.0, 1.0 )
		glow = 0.18 * np.clip( 1.0 - distance / ( layout.DOT_PIXELS / 2.0 ), 0.0, 1.0 )
		atlas[y0:y1, x0:x1] = np.maximum( atlas[y0:y1, x0:x1], np.maximum( core, glow ) )
	def DrawText( i_name, i_text ):
		x, y, width, height = layout.ATLAS_LAYOUT[i_name]
		for characterIndex, character in enumerate( i_text ):
			rows = layout.FONT[character]
			for row, pattern in enumerate( rows ):
				for column, mark in enumerate( pattern ):
					if mark == "#":
						# (The quad's edge is half a dot past the outside dots, so the first dot's center is one dot in from the edge)
						DrawDot( x + ( characterIndex * layout.CHARACTER_ADVANCE + column + 1 ) * layout.DOT_PIXELS,
							y + ( row + 1 ) * layout.DOT_PIXELS )
	for glyph in layout.NUMBER_GLYPHS:
		DrawText( "glyph_" + glyph, glyph )
	for label in layout.LABELS + [ layout.TIMING_LIGHT_LABEL ]:
		DrawText( "label_" + label, label )
	x, y, width, height = layout.ATLAS_LAYOUT["solid_white"]
	atlas[y:y + height, x:x + width] = 1.0
	Save( "scoreboard.bmp", np.repeat( ( atlas * 255.0 )[..., None], 3, axis = 2 ) )
