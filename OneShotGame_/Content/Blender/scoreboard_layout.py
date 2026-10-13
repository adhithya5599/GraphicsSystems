"""
	The scoreboard's dot-matrix font and layout, shared by make_textures.py (which draws the texture)
	and build_archery_range.py (which builds the quads that show parts of that texture)

	The big screen is an LED wall: every character is a grid of 5 x 7 dots.
	All of the characters and labels are drawn into one texture (an "atlas"),
	and each thing on the screen is a quad whose texture coordinates pick its part of the atlas.
	The labels never change, so they are part of the screen's mesh;
	the numbers change, so each character has its own small quad mesh that the game draws wherever a number needs it.

	Units
		In the atlas, every dot is DOT_PIXELS pixels apart.
		On the screen, a character's height is the height of its 7 rows of dots (so one dot is height / 7 apart),
		and every character is 6 dots wide (5 dots plus 1 dot of space).
		A quad covers half a dot more than the dots on every side, so dots near its edge aren't cut off.
"""

# The Font
#=========

DOT_COLUMNS = 5
DOT_ROWS = 7
# Each character takes this many dots horizontally (its 5 dots plus a 1-dot gap before the next character)
CHARACTER_ADVANCE = 6

FONT = {
	"0": [ ".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###." ],
	"1": [ "..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###." ],
	"2": [ ".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####" ],
	"3": [ "#####", "...#.", "..#..", "...#.", "....#", "#...#", ".###." ],
	"4": [ "...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#." ],
	"5": [ "#####", "#....", "####.", "....#", "....#", "#...#", ".###." ],
	"6": [ "..##.", ".#...", "#....", "####.", "#...#", "#...#", ".###." ],
	"7": [ "#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..." ],
	"8": [ ".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###." ],
	"9": [ ".###.", "#...#", "#...#", ".####", "....#", "...#.", ".##.." ],
	"M": [ "#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#" ],
	"-": [ ".....", ".....", ".....", "#####", ".....", ".....", "....." ],
	"A": [ ".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#" ],
	"B": [ "####.", "#...#", "#...#", "####.", "#...#", "#...#", "####." ],
	"C": [ ".###.", "#...#", "#....", "#....", "#....", "#...#", ".###." ],
	"E": [ "#####", "#....", "#....", "####.", "#....", "#....", "#####" ],
	"F": [ "#####", "#....", "#....", "####.", "#....", "#....", "#...." ],
	"L": [ "#....", "#....", "#....", "#....", "#....", "#....", "#####" ],
	"O": [ ".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###." ],
	"P": [ "####.", "#...#", "#...#", "####.", "#....", "#....", "#...." ],
	"R": [ "####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#" ],
	"S": [ ".####", "#....", "#....", ".###.", "....#", "....#", "####." ],
	"T": [ "#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.." ],
	"U": [ "#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###." ],
	"V": [ "#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.." ],
	"W": [ "#...#", "#...#", "#...#", "#.#.#", "#.#.#", "#.#.#", ".#.#." ],
	" ": [ "....." ] * 7,
}

# The characters that the game can show in numbers, in the order of the game's glyph meshes
# (OneShotGame/cScoreboard.cpp uses the same order: the digits, then "M" for a miss, then "-" for no score yet)
NUMBER_GLYPHS = [ "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "M", "-" ]

# The fixed labels on the screen
LABELS = [ "70M RECURVE", "TOTAL", "ARROWS LEFT", "BEST" ]
# The word under the green light on the timing light at the shooting line (which uses the same texture)
TIMING_LIGHT_LABEL = "SPACE"

# The Atlas
#==========

ATLAS_WIDTH = 1024
ATLAS_HEIGHT = 512
DOT_PIXELS = 8
# Empty (black) space around every part of the atlas,
# so that the smaller mipmaps of one part never blur into its neighbors
MARGIN_PIXELS = 16
# Plain areas for the screen's black background and its white bars (any color can be made from white with a vertex color)
SOLID_SIZE_PIXELS = 32

def StripSizeInPixels( i_text ):
	""" The size of the quad for a line of text: 6 dots per character by 8 dots (7 rows plus half a dot above and below) """
	return ( len( i_text ) * CHARACTER_ADVANCE * DOT_PIXELS, ( DOT_ROWS + 1 ) * DOT_PIXELS )

def _BuildAtlasLayout():
	""" Packs every glyph, label, and solid area into rows (left to right, top to bottom); returns { name: ( x, y, width, height ) } in pixels from the top-left """
	items = [ ( "glyph_" + glyph, StripSizeInPixels( glyph ) ) for glyph in NUMBER_GLYPHS ]
	items += [ ( "label_" + label, StripSizeInPixels( label ) ) for label in LABELS ]
	items += [ ( "solid_black", ( SOLID_SIZE_PIXELS, SOLID_SIZE_PIXELS ) ), ( "solid_white", ( SOLID_SIZE_PIXELS, SOLID_SIZE_PIXELS ) ) ]
	# (Added after everything else so that adding it didn't move anything that was already in the atlas)
	items += [ ( "label_" + TIMING_LIGHT_LABEL, StripSizeInPixels( TIMING_LIGHT_LABEL ) ) ]
	layout = {}
	x, y, rowHeight = MARGIN_PIXELS, MARGIN_PIXELS, 0
	for name, ( width, height ) in items:
		if x + width + MARGIN_PIXELS > ATLAS_WIDTH:
			x = MARGIN_PIXELS
			y += rowHeight + MARGIN_PIXELS
			rowHeight = 0
		layout[name] = ( x, y, width, height )
		x += width + MARGIN_PIXELS
		rowHeight = max( rowHeight, height )
	assert y + rowHeight + MARGIN_PIXELS <= ATLAS_HEIGHT, "The scoreboard atlas is too small for everything in it"
	return layout

ATLAS_LAYOUT = _BuildAtlasLayout()

def AtlasUvs( i_name, i_inset = 0.0 ):
	"""
		The texture coordinates of an area of the atlas as ( left, right, bottom, top ),
		in Blender's convention (v = 1 is the top of the image).
		i_inset shrinks the area by that many pixels (for the solid areas, so filtering never reaches their edges).
	"""
	x, y, width, height = ATLAS_LAYOUT[i_name]
	left = ( x + i_inset ) / ATLAS_WIDTH
	right = ( x + width - i_inset ) / ATLAS_WIDTH
	top = 1.0 - ( y + i_inset ) / ATLAS_HEIGHT
	bottom = 1.0 - ( y + height - i_inset ) / ATLAS_HEIGHT
	return ( left, right, bottom, top )

# The Screen
#===========
# Positions on the screen are in meters from the middle of the screen (x to the right and y up).
# These numbers are also in OneShotGame/GameplaySettings.h (Settings::Scoreboard), which places the numbers;
# if a number changes in one place, change it in the other too.

DISPLAY_WIDTH = 16.0
DISPLAY_HEIGHT = 8.0
# The screen's middle, above the ground under the scoreboard
DISPLAY_CENTER_HEIGHT = 7.0
# How far in front of the cabinet each layer is (far enough apart that the depth buffer can always tell them apart from the archer 75 m away)
DISPLAY_BACKGROUND_Z = 0.44
DISPLAY_LABELS_Z = 0.48
DISPLAY_NUMBERS_Z = 0.52	# (Settings::Scoreboard::numbersZ)

# The six arrow slots: each shows one arrow's score above a bar
SLOT_COUNT = 6
SLOT_SPACING = 2.45	# (Settings::Scoreboard::slotSpacing)
SLOT_BOTTOM = -2.0	# (Settings::Scoreboard::slotBottom)
SLOT_HEIGHT = 1.2	# (Settings::Scoreboard::slotHeight)
SLOT_BAR_BOTTOM = -2.4
SLOT_BAR_HEIGHT = 0.12

def TextWidth( i_characterCount, i_height ):
	""" The width of a line of text's dots (from the left of its first dot to the right of its last dot) """
	return ( i_characterCount * CHARACTER_ADVANCE - 1 ) * ( i_height / DOT_ROWS )

def SlotCenterX( i_slotIndex ):
	return ( i_slotIndex - ( SLOT_COUNT - 1 ) / 2.0 ) * SLOT_SPACING

# The labels: ( text, left, bottom, height ), where left and bottom are the bottom-left corner of the text's dots
LABEL_PLACEMENTS = [
	( "70M RECURVE", -7.3, 2.9, 0.7 ),
	( "BEST", 3.3, 2.9, 0.7 ),
	( "TOTAL", -7.3, 0.9, 1.0 ),
	( "ARROWS LEFT", -7.3, -3.6, 0.7 ),
]

def TextQuadCorners( i_left, i_bottom, i_text, i_height ):
	""" The quad that shows a line of text: it extends half a dot past the outside dots on every side """
	dot = i_height / DOT_ROWS
	left = i_left - 0.5 * dot
	right = i_left + ( len( i_text ) * CHARACTER_ADVANCE - 0.5 ) * dot
	bottom = i_bottom - 0.5 * dot
	top = i_bottom + ( DOT_ROWS + 0.5 ) * dot
	return left, right, bottom, top
