"""
	Builds One Shot's archery range in Blender and exports every mesh with the Blender mesh exporter.

	Usage, from the solution's directory (Blender 4.2 or newer):
		blender --background --python OneShotGame_/Content/Blender/build_archery_range.py -- Tools OneShotGame_/Content/Blender/ArcheryRange.blend OneShotGame_/Content/Meshes OneShotGame_/Content/Textures
	(the arguments are: the directory containing BlenderMeshExporter, the .blend to save, where to export the meshes, and where the textures are).
	It takes about half a minute, most of it baking. Generate the textures first (make_textures.py) if they don't exist.

	Why a script instead of modeling by hand?
		* The level layout (where the target and obstacles are) has to match the game's colliders exactly.
		  The numbers below are the same numbers as OneShotGame/GameplaySettings.h,
		  and the game is the one that places every obstacle and target, so a mesh only has to be the right shape.
		* Anything can be tweaked and rebuilt in seconds, and the result is the same every time.
		* The saved .blend is still an ordinary Blender file that can be opened and edited by hand.

	Coordinates
		Everything is described in the engine's coordinates (meters):
		+x is right, +y is up, and the arrow flies down -z toward the target, which is 70 m away (an Olympic distance).
		The exporter converts Blender ( x, y, z ) to engine ( x, z, y ),
		so in Blender the range runs along -Y and Z is up.
		(That conversion also mirrors the scene left-to-right compared to the Blender viewport, like the Maya exporter does.)

	Look
		The engine has no shadows, no normal maps, and one texture per object, so the style is clean low-poly shapes
		colored mostly by vertex colors. The lighting that the engine can't do is baked into the vertex colors here:
		ambient occlusion (soft darkening where things meet) and the sun's shadows on everything that doesn't move.
"""

import math
import os
import random
import sys

import bpy
import bmesh
from mathutils import Matrix, Vector

# Arguments
#==========

arguments = sys.argv[sys.argv.index( "--" ) + 1:] if "--" in sys.argv else sys.argv[1:]
exporterDirectory, blendPath, meshDirectory, textureDirectory = arguments[:4]
sys.path.insert( 0, exporterDirectory )
import BlenderMeshExporter as exporterAddOn
from BlenderMeshExporter import cBlenderMeshExporter as exporterModule
# The scoreboard's font and layout are shared with make_textures.py, which draws the scoreboard's texture
sys.path.insert( 0, os.path.dirname( os.path.abspath( __file__ ) ) )
import scoreboard_layout

random.seed( 6320 )

# Layout
#=======
# (These must match OneShotGame/GameplaySettings.h)

ARROW_REST = ( 0.0, 1.5, 0.0 )
TARGET_POSITION = ( 0.0, 0.0, -70.0 )
TARGET_FACE_CENTER_HEIGHT = 1.3
TARGET_TILT_DEGREES = 12.0
NEIGHBOR_TARGET_XS = ( -15.0, -10.0, 10.0, 15.0 )
RAIL_SLIDER_Z = -20.0
BUNKER_Z = -33.0
PULLEY_Z = -48.0
SLED_Z = -60.0
FLAG_POSITIONS = ( ( -7.0, -12.0 ), ( 7.0, -40.0 ), ( -7.0, -66.0 ) )
# The big screen: the point on the ground under its middle, and how far it is turned (around the vertical axis) to face the shooting line
SCOREBOARD_POSITION = ( 21.0, 0.0, -73.0 )
SCOREBOARD_YAW_DEGREES = -16.0
# The ground quiver that holds the arrows the archer hasn't shot yet
QUIVER_POSITION = ( 2.2, 0.0, -1.2 )
# The timing light at the shooting line, turned to face the aiming camera
TIMING_LIGHT_POSITION = ( -2.2, 0.0, -2.0 )
TIMING_LIGHT_YAW_DEGREES = 28.0
# The sun: the direction its light travels (the same as "directional.direction" in Content/Lighting/scene.lighting)
SUN_DIRECTION = ( 0.45, -0.5, -0.74 )

# Colors (sRGB, 0-255)
#=====================

C_WHITE = ( 240, 240, 236 )
C_LINE_WHITE = ( 250, 250, 250 )
C_STEEL = ( 120, 126, 132 )
C_DARK_STEEL = ( 70, 74, 80 )
C_CONCRETE = ( 170, 168, 160 )
C_SEAT_BLUE = ( 30, 90, 170 )
C_ROOF = ( 225, 228, 232 )
C_NET_GREEN = ( 40, 80, 50 )
C_TRUNK = ( 105, 75, 50 )
C_LEAVES = ( ( 60, 110, 50 ), ( 75, 125, 55 ), ( 50, 95, 45 ) )
C_SANDBAG = ( 190, 170, 125 )
C_WARNING_RED = ( 210, 40, 35 )
C_ROPE = ( 200, 180, 140 )
C_FLAG = ( 245, 200, 30 )
C_SKIN = ( 220, 175, 140 )
C_SHIRT = ( 240, 240, 245 )
C_PANTS = ( 25, 40, 80 )
C_RISER = ( 30, 70, 140 )
C_LIMB = ( 25, 25, 28 )
C_STRING = ( 235, 235, 235 )
C_CABINET = ( 45, 48, 54 )
C_QUIVER = ( 30, 45, 90 )
# The scoreboard's LEDs (the scoreboard texture's dots are white, so these vertex colors give them their color)
C_LED_AMBER = ( 255, 176, 40 )
C_LED_RED = ( 255, 70, 50 )
C_LED_WHITE = ( 235, 235, 235 )
C_LED_BAR = ( 90, 90, 90 )

# Helpers
#========

def E( i_x, i_y, i_z ):
	""" Converts an engine position to a Blender position """
	return Vector( ( i_x, i_z, i_y ) )

def SrgbToLinear( i_value ):
	return ( i_value / 255.0 ) ** 2.2

def LinearToSrgb( i_value ):
	return max( 0.0, min( 1.0, i_value ) ) ** ( 1.0 / 2.2 )

class cMeshBuilder:
	"""
		Collects geometry into one bmesh.
		Every face gets a base color (stored per face corner, so neighboring faces can have different colors)
		and texture coordinates.
	"""
	def __init__( self, i_name ):
		self.name = i_name
		self.bm = bmesh.new()
		self.uvLayer = self.bm.loops.layers.uv.new( "UVMap" )
		self.colorLayer = self.bm.loops.layers.float_color.new( "Base" )

	def _Finish( self, i_faces, i_color, i_uvFunction ):
		r, g, b = ( SrgbToLinear( c ) for c in i_color )
		for face in i_faces:
			for loop in face.loops:
				loop[self.colorLayer] = ( r, g, b, 1.0 )
				loop[self.uvLayer].uv = i_uvFunction( loop.vert.co, face.normal )

	def AddFromBmeshOp( self, i_geometry, i_matrix, i_color, i_uvFunction ):
		verts = [ g for g in i_geometry if isinstance( g, bmesh.types.BMVert ) ]
		bmesh.ops.transform( self.bm, matrix = i_matrix, verts = verts )
		faces = list( { f for v in verts for f in v.link_faces } )
		bmesh.ops.recalc_face_normals( self.bm, faces = faces )
		self.bm.normal_update()
		self._Finish( faces, i_color, i_uvFunction )
		return faces

	def Box( self, i_center, i_size, i_color, i_uvFunction = None, i_rotationX = 0.0, i_rotationY = 0.0 ):
		""" A box in engine coordinates: center, size, and optional rotations (in degrees) around the engine x and y axes """
		geometry = bmesh.ops.create_cube( self.bm, size = 1.0 )["verts"]
		scale = Matrix.Diagonal( ( i_size[0], i_size[2], i_size[1], 1.0 ) )
		# Engine x is Blender X and engine y (up) is Blender Z
		rotation = Matrix.Rotation( math.radians( i_rotationY ), 4, 'Z' ) @ Matrix.Rotation( math.radians( i_rotationX ), 4, 'X' )
		matrix = Matrix.Translation( E( *i_center ) ) @ rotation @ scale
		return self.AddFromBmeshOp( geometry, matrix, i_color, i_uvFunction or UvWhite )

	def Cylinder( self, i_base, i_radius, i_height, i_color, i_segments = 8, i_topRadius = None, i_uvFunction = None ):
		""" A vertical cylinder (or cone) standing on i_base """
		geometry = bmesh.ops.create_cone( self.bm, cap_ends = True, segments = i_segments,
			radius1 = i_radius, radius2 = i_radius if i_topRadius is None else i_topRadius, depth = i_height )["verts"]
		matrix = Matrix.Translation( E( i_base[0], i_base[1] + i_height / 2, i_base[2] ) )
		return self.AddFromBmeshOp( geometry, matrix, i_color, i_uvFunction or UvWhite )

	def Rod( self, i_start, i_end, i_radius, i_color, i_segments = 6 ):
		""" A cylinder from one engine position to another """
		start, end = E( *i_start ), E( *i_end )
		direction = end - start
		geometry = bmesh.ops.create_cone( self.bm, cap_ends = True, segments = i_segments,
			radius1 = i_radius, radius2 = i_radius, depth = direction.length )["verts"]
		rotation = Vector( ( 0, 0, 1 ) ).rotation_difference( direction.normalized() ).to_matrix().to_4x4()
		matrix = Matrix.Translation( ( start + end ) / 2 ) @ rotation
		return self.AddFromBmeshOp( geometry, matrix, i_color, UvWhite )

	def Sphere( self, i_center, i_radius, i_color, i_subdivisions = 1, i_jitter = 0.0, i_scale = ( 1, 1, 1 ) ):
		geometry = bmesh.ops.create_icosphere( self.bm, subdivisions = i_subdivisions, radius = i_radius )["verts"]
		for vertex in geometry:
			vertex.co += Vector( ( random.uniform( -1, 1 ), random.uniform( -1, 1 ), random.uniform( -1, 1 ) ) ) * i_jitter
		matrix = Matrix.Translation( E( *i_center ) ) @ Matrix.Diagonal( ( i_scale[0], i_scale[2], i_scale[1], 1.0 ) )
		return self.AddFromBmeshOp( geometry, matrix, i_color, UvWhite )

	def Quad( self, i_corners, i_color, i_uvs, i_frontDirection ):
		"""
			A single face from four engine positions with one UV per corner.
			The engine only draws the front of a face, so the face is turned to point along i_frontDirection (in engine coordinates).
		"""
		verts = [ self.bm.verts.new( E( *c ) ) for c in i_corners ]
		face = self.bm.faces.new( verts )
		self.bm.normal_update()
		r, g, b = ( SrgbToLinear( c ) for c in i_color )
		for loop, uv in zip( face.loops, i_uvs ):
			loop[self.colorLayer] = ( r, g, b, 1.0 )
			loop[self.uvLayer].uv = uv
		if face.normal.dot( E( *i_frontDirection ) ) < 0.0:
			# (Flipping reverses the corner order, and each corner keeps its own UV and color)
			face.normal_flip()
			self.bm.normal_update()
		return [ face ]

	def ToObject( self, i_collection, i_smooth = False ):
		mesh = bpy.data.meshes.new( self.name )
		self.bm.to_mesh( mesh )
		self.bm.free()
		for polygon in mesh.polygons:
			polygon.use_smooth = i_smooth
		obj = bpy.data.objects.new( self.name, mesh )
		i_collection.objects.link( obj )
		return obj

# Texture coordinate functions
# (each takes a Blender vertex position and face normal and returns a UV)

def UvWhite( i_position, i_normal ):
	# The top-left corner of the target texture is white; every other texture is either white or never sampled this way.
	# (UV v = 1 is the top of the image after the exporter writes v as 1 - v)
	return ( 0.02, 0.98 )

def MakeUvBoxProjection( i_metersPerTile, i_origin = ( 0, 0, 0 ) ):
	""" Projects a repeating texture onto a box from whichever axis the face points along """
	def Uv( i_position, i_normal ):
		p = i_position - Vector( i_origin )
		n = i_normal
		if abs( n.z ) >= max( abs( n.x ), abs( n.y ) ):
			u, v = p.x, p.y
		elif abs( n.x ) >= abs( n.y ):
			u, v = p.y, p.z
		else:
			u, v = p.x, p.z
		return ( u / i_metersPerTile, v / i_metersPerTile )
	return Uv

UvWood = MakeUvBoxProjection( 1.2 )

def UvGround( i_position, i_normal ):
	# The grass repeats every 3 m
	return ( i_position.x / 3.0, i_position.y / 3.0 )

# Scene setup
#============

bpy.ops.wm.read_factory_settings( use_empty = True )
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'

def NewCollection( i_name ):
	collection = bpy.data.collections.new( i_name )
	scene.collection.children.link( collection )
	return collection

collection_environment = NewCollection( "Environment (static, world space)" )
collection_props = NewCollection( "Props (placed by the game)" )
collection_moving = NewCollection( "Moving parts (placed by the game)" )

# Each exported file is a list of objects plus the engine position that becomes the file's origin
# (None for meshes that are exported in world space)
exports = []
def Export( i_fileName, i_objects, i_origin = None, i_texture = "white" ):
	exports.append( ( i_fileName, i_objects, i_origin, i_texture ) )

# Ground
#=======
# A finely divided field (so baked shadows and the mowing stripes have enough vertices to show detail)
# on top of a coarse outer ground that reaches the hills.

# The engine's meshes use 16-bit indices, so one mesh can have at most 21,845 triangles.
# The field has more than that, so it is built and baked as one object and then split into three strips for exporting.
# (Baking the strips as separate objects darkens the seams between them:
# the baking rays from one strip's edge graze the next strip's edge and count it as something blocking the light.)
FIELD_STRIPS = ( ( "left", -42.0, -14.0 ), ( "center", -14.0, 14.0 ), ( "right", 14.0, 42.0 ) )

def BuildGround():
	field = BuildFieldStrip( "Ground_Field", -42.0, 42.0 )
	BuildOuterGround()
	return field

def SplitIntoStrips( i_object ):
	""" Copies the (baked) field into one object per strip, each keeping only the faces whose centers are in its strip """
	strips = []
	for stripName, xStart, xEnd in FIELD_STRIPS:
		copy = i_object.copy()
		copy.data = i_object.data.copy()
		copy.name = f"Ground_Field_{stripName}"
		collection_environment.objects.link( copy )
		bm = bmesh.new()
		bm.from_mesh( copy.data )
		outside = [ f for f in bm.faces if not ( xStart <= f.calc_center_median().x <= xEnd ) ]
		bmesh.ops.delete( bm, geom = outside, context = 'FACES' )
		bm.to_mesh( copy.data )
		bm.free()
		strips.append( ( stripName, copy ) )
	return strips

def BuildFieldStrip( i_name, i_xStart, i_xEnd ):
	builder = cMeshBuilder( i_name )
	spacing = 0.75
	xs = [ i_xStart + i * spacing for i in range( int( round( ( i_xEnd - i_xStart ) / spacing ) ) + 1 ) ]
	zs = [ 24.0 - i * spacing for i in range( int( 112 / spacing ) + 1 ) ]
	grid = [ [ builder.bm.verts.new( E( x, 0.0, z ) ) for x in xs ] for z in zs ]
	faces = []
	for row in range( len( zs ) - 1 ):
		for column in range( len( xs ) - 1 ):
			corners = [ grid[row][column], grid[row][column + 1], grid[row + 1][column + 1], grid[row + 1][column] ]
			faces.append( builder.bm.faces.new( corners ) )
	builder.bm.normal_update()
	for face in faces:
		if face.normal.z < 0:
			face.normal_flip()
	builder.bm.normal_update()
	for face in faces:
		for loop in face.loops:
			x, z = loop.vert.co.x, loop.vert.co.y
			# Mowed stripes: alternating 4 m wide bands across the range, slightly lighter and darker
			stripe = 1.06 if ( math.floor( x / 4.0 ) % 2 == 0 ) else 0.94
			# The field's edges fade into rougher, darker grass
			edge = min( 1.0, max( 0.0, ( 30.0 - abs( x ) ) / 6.0 ) ) * min( 1.0, max( 0.0, min( z + 80.0, 20.0 - z ) / 6.0 ) )
			shade = ( stripe * edge + 0.9 * ( 1 - edge ) )
			loop[builder.colorLayer] = ( shade, shade, shade, 1.0 )
			loop[builder.uvLayer].uv = UvGround( loop.vert.co, face.normal )
	return builder.ToObject( collection_environment )

def BuildOuterGround():
	builder = cMeshBuilder( "Ground_Outer" )
	# The outer ground sits 3 cm lower so that it never shows through the field where they overlap
	size, step = 420.0, 20.0
	count = int( size / step )
	grid = [ [ builder.bm.verts.new( E( -size / 2 + i * step, -0.03, -35.0 - size / 2 + j * step ) ) for i in range( count + 1 ) ] for j in range( count + 1 ) ]
	faces = []
	for j in range( count ):
		for i in range( count ):
			faces.append( builder.bm.faces.new( [ grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i] ] ) )
	builder.bm.normal_update()
	for face in faces:
		if face.normal.z < 0:
			face.normal_flip()
	builder.bm.normal_update()
	for face in faces:
		for loop in face.loops:
			loop[builder.colorLayer] = ( 0.8, 0.8, 0.8, 1.0 )
			loop[builder.uvLayer].uv = UvGround( loop.vert.co, face.normal )
	outer = builder.ToObject( collection_environment )
	Export( "range_ground_outer.blendermesh", [ outer ], i_texture = "grass" )

# Painted lines
#==============
# The shooting line and the lane lines are thin white quads 3 cm above the grass
# (far enough above it that the depth buffer can always tell them apart at 70 m)

def BuildLines():
	builder = cMeshBuilder( "Lines" )
	height = 0.03
	def Strip( i_x0, i_x1, i_z0, i_z1 ):
		builder.Quad( [ ( i_x0, height, i_z1 ), ( i_x1, height, i_z1 ), ( i_x1, height, i_z0 ), ( i_x0, height, i_z0 ) ],
			C_LINE_WHITE, [ ( 0.02, 0.98 ) ] * 4, ( 0, 1, 0 ) )
	# The shooting line
	Strip( -28.0, 28.0, -0.04, 0.04 )
	# The waiting line behind it
	for i in range( 28 ):
		x = -28.0 + i * 2.0
		Strip( x, x + 1.0, 4.96, 5.04 )
	# Lane lines between the targets, from the shooting line to the targets
	for x in ( -17.5, -12.5, -7.5, -2.5, 2.5, 7.5, 12.5, 17.5 ):
		Strip( x - 0.03, x + 0.03, -70.0, 0.0 )
	return builder.ToObject( collection_environment )

# Grandstand
#===========

# How many groups the crowd is split into (OneShotGame/sGameAssets.h has the same number)
CROWD_GROUP_COUNT = 3

def BuildGrandstand():
	objects = []
	builder = cMeshBuilder( "Grandstand" )
	zStart, zEnd = -8.0, -62.0
	length = zStart - zEnd
	zCenter = ( zStart + zEnd ) / 2
	stepCount, stepDepth, stepRise = 10, 0.9, 0.45
	xFront = 36.0
	for step in range( stepCount ):
		x = xFront + step * stepDepth
		height = ( step + 1 ) * stepRise
		builder.Box( ( x + stepDepth / 2, height / 2, zCenter ), ( stepDepth, height, length ), C_CONCRETE )
		# A row of seats on each step
		builder.Box( ( x + stepDepth * 0.65, height + 0.2, zCenter ), ( 0.35, 0.4, length - 1.0 ), C_SEAT_BLUE )
	xBack = xFront + stepCount * stepDepth
	topHeight = stepCount * stepRise
	# The back wall and the roof on posts
	builder.Box( ( xBack + 0.15, ( topHeight + 3.0 ) / 2, zCenter ), ( 0.3, topHeight + 3.0, length ), C_CONCRETE )
	for z in [ zStart - i * 6.0 for i in range( int( length / 6.0 ) + 1 ) ]:
		builder.Box( ( xFront + 0.2, ( topHeight + 2.6 ) / 2, z ), ( 0.2, topHeight + 2.6, 0.2 ), C_STEEL )
	builder.Box( ( ( xFront + xBack ) / 2 - 0.5, topHeight + 3.0, zCenter ), ( xBack - xFront + 2.5, 0.2, length + 2.0 ), C_ROOF, i_rotationY = 0.0 )
	# The stairs at each end
	for z in ( zStart + 0.75, zEnd - 0.75 ):
		builder.Box( ( ( xFront + xBack ) / 2, topHeight / 4, z ), ( xBack - xFront, topHeight / 2, 1.5 ), C_CONCRETE )
	objects.append( builder.ToObject( collection_environment ) )

	# The crowd: simple blocks in shirt colors, a morning crowd that fills the stands about halfway.
	# The people are shared out between a few meshes so that the game can make each group jump up and down
	# a little out of step with the others when the crowd cheers.
	builders = [ cMeshBuilder( f"Crowd_{group}" ) for group in range( CROWD_GROUP_COUNT ) ]
	shirtColors = [ ( 200, 40, 40 ), ( 40, 80, 170 ), ( 240, 240, 240 ), ( 240, 190, 40 ), ( 40, 140, 70 ), ( 30, 30, 35 ), ( 230, 120, 40 ) ]
	skinColors = [ ( 230, 190, 160 ), ( 200, 150, 115 ), ( 150, 105, 75 ), ( 100, 70, 50 ) ]
	seatIndex = 0
	for step in range( stepCount ):
		x = xFront + step * stepDepth + stepDepth * 0.65
		seatHeight = ( step + 1 ) * stepRise + 0.4
		z = zStart - 0.8
		while z > zEnd + 0.8:
			if random.random() < 0.5:
				# (The group comes from a fixed pattern rather than a random number,
				# so that the random numbers used for everything else, like where the trees go, stay the same)
				builder = builders[( seatIndex * 7 + step ) % CROWD_GROUP_COUNT]
				builder.Box( ( x, seatHeight + 0.3, z ), ( 0.32, 0.6, 0.4 ), random.choice( shirtColors ) )
				builder.Box( ( x, seatHeight + 0.72, z ), ( 0.2, 0.22, 0.2 ), random.choice( skinColors ) )
			seatIndex += 1
			z -= 0.65
	crowd = [ builder.ToObject( collection_environment ) for builder in builders ]
	return objects, crowd

# Surroundings
#=============

def BuildTree( io_builder, i_position, i_height ):
	x, z = i_position
	trunkHeight = i_height * 0.35
	io_builder.Cylinder( ( x, 0.0, z ), 0.18 * i_height / 8, trunkHeight, C_TRUNK, i_segments = 6 )
	# A rounded canopy of two lumpy spheres
	leaves = random.choice( C_LEAVES )
	radius = i_height * 0.3
	io_builder.Sphere( ( x, trunkHeight + radius * 0.8, z ), radius, leaves, i_subdivisions = 1, i_jitter = radius * 0.12 )
	io_builder.Sphere( ( x + radius * 0.3, trunkHeight + radius * 1.5, z - radius * 0.2 ), radius * 0.7, leaves, i_subdivisions = 1, i_jitter = radius * 0.1 )

def BuildSurroundings():
	builder = cMeshBuilder( "Surroundings" )
	# Fences along both sides of the field
	for side in ( -1, 1 ):
		x = side * 30.0
		for z in [ 14.0 - i * 3.0 for i in range( 32 ) ]:
			builder.Box( ( x, 0.55, z ), ( 0.08, 1.1, 0.08 ), C_WHITE )
		for height in ( 0.45, 0.95 ):
			builder.Box( ( x, height, -32.0 ), ( 0.05, 0.08, 93.0 ), C_WHITE )
	# The backstop net behind the targets
	builder.Box( ( 0.0, 2.0, -78.0 ), ( 60.0, 4.0, 0.1 ), C_NET_GREEN )
	for x in [ -30.0 + i * 6.0 for i in range( 11 ) ]:
		builder.Box( ( x, 2.1, -77.9 ), ( 0.12, 4.2, 0.12 ), C_STEEL )
	# (The old scoreboard used to be built here and used four random numbers.
	# Using them up keeps every tree below exactly where it was before the big screen replaced it.)
	for _ in range( 4 ):
		random.uniform( 2.0, 5.5 )
	# Pop-up canopy tents behind the shooting line (for the archers and the officials)
	for x, color in ( ( -12.0, ( 200, 40, 40 ) ), ( -5.0, C_WHITE ), ( 5.0, ( 30, 90, 170 ) ), ( 12.0, C_WHITE ) ):
		z = 12.0
		for dx in ( -1.4, 1.4 ):
			for dz in ( -1.4, 1.4 ):
				builder.Box( ( x + dx, 1.1, z + dz ), ( 0.06, 2.2, 0.06 ), C_STEEL )
		builder.Box( ( x, 2.25, z ), ( 3.0, 0.1, 3.0 ), color )
		builder.Cylinder( ( x, 2.3, z ), 2.1, 0.8, color, i_segments = 4, i_topRadius = 0.05 )
		# A table under each tent
		builder.Box( ( x, 0.72, z - 0.6 ), ( 1.8, 0.06, 0.7 ), C_WHITE )
		for dx in ( -0.8, 0.8 ):
			builder.Box( ( x + dx, 0.36, z - 0.6 ), ( 0.05, 0.72, 0.6 ), C_STEEL )
	# Trees beyond the fences and behind the targets, kept out of the stands
	positions = []
	for _ in range( 70 ):
		for _ in range( 50 ):
			x = random.uniform( -75.0, 75.0 )
			z = random.uniform( -120.0, 45.0 )
			outsideField = ( abs( x ) > 34.0 ) or ( z < -84.0 ) or ( z > 20.0 )
			clearOfStands = not ( 33.0 < x < 52.0 and -66.0 < z < -4.0 )
			farEnough = all( ( x - px ) ** 2 + ( z - pz ) ** 2 > 36.0 for px, pz in positions )
			if outsideField and clearOfStands and farEnough:
				positions.append( ( x, z ) )
				break
	for position in positions:
		BuildTree( builder, position, random.uniform( 7.0, 12.0 ) )
	objects = [ builder.ToObject( collection_environment ) ]

	# Rolling hills all the way around, hazier (bluer and lighter) than the nearby grass because they are far away
	builder = cMeshBuilder( "Hills" )
	segments = 72
	ring = []
	for i in range( segments ):
		angle = 2 * math.pi * i / segments
		radius = 150.0 + 15.0 * math.sin( angle * 3 + 1.0 )
		height = 10.0 + 7.0 * math.sin( angle * 5 ) + 4.0 * math.sin( angle * 11 + 2.0 )
		center = ( radius * math.cos( angle ), -35.0 + radius * math.sin( angle ) )
		inner = builder.bm.verts.new( E( center[0] * 0.8, -0.05, -35.0 + ( center[1] + 35.0 ) * 0.8 ) )
		top = builder.bm.verts.new( E( center[0], height, center[1] ) )
		outer = builder.bm.verts.new( E( center[0] * 1.15, -0.05, -35.0 + ( center[1] + 35.0 ) * 1.15 ) )
		ring.append( ( inner, top, outer ) )
	faces = []
	for i in range( segments ):
		a, b = ring[i], ring[( i + 1 ) % segments]
		faces.append( builder.bm.faces.new( [ a[0], b[0], b[1], a[1] ] ) )
		faces.append( builder.bm.faces.new( [ a[1], b[1], b[2], a[2] ] ) )
	builder.bm.normal_update()
	for face in faces:
		if face.normal.z < 0:
			face.normal_flip()
	builder.bm.normal_update()
	for face in faces:
		for loop in face.loops:
			height = loop.vert.co.z
			haze = 0.35 + 0.35 * min( 1.0, height / 20.0 )
			color = [ SrgbToLinear( c ) * ( 1 - haze ) + SrgbToLinear( h ) * haze for c, h in zip( ( 80, 120, 70 ), ( 175, 200, 220 ) ) ]
			loop[builder.colorLayer] = ( *color, 1.0 )
			loop[builder.uvLayer].uv = ( 0.02, 0.98 )
	objects.append( builder.ToObject( collection_environment ) )
	return objects

# The archer and the bow
#=======================
# A simple mannequin at full draw: standing side-on with the left shoulder toward the target,
# the bow arm reaching toward the target and the drawing hand at the chin.

def BuildShooter():
	builder = cMeshBuilder( "Archer_And_Bow" )
	nockX, nockY, nockZ = ARROW_REST
	bodyX = nockX + 0.17
	# Legs (feet straddling the shooting line), hips, torso, and head
	for footZ in ( -0.22, 0.22 ):
		builder.Rod( ( bodyX, 0.05, footZ ), ( bodyX, 0.92, footZ * 0.5 ), 0.075, C_PANTS )
		builder.Box( ( bodyX + 0.04, 0.04, footZ ), ( 0.26, 0.08, 0.1 ), ( 30, 30, 30 ) )
	builder.Box( ( bodyX, 0.98, 0.0 ), ( 0.24, 0.2, 0.36 ), C_PANTS )
	builder.Box( ( bodyX, 1.22, 0.0 ), ( 0.22, 0.5, 0.4 ), C_SHIRT )
	builder.Rod( ( bodyX, 1.45, 0.0 ), ( bodyX, 1.52, 0.0 ), 0.05, C_SKIN )
	builder.Sphere( ( bodyX - 0.04, 1.62, 0.03 ), 0.11, C_SKIN, i_subdivisions = 2, i_scale = ( 0.9, 1.1, 1.0 ) )
	builder.Sphere( ( bodyX - 0.01, 1.68, 0.06 ), 0.1, ( 60, 40, 25 ), i_subdivisions = 2, i_scale = ( 0.95, 0.8, 1.0 ) )
	# The bow arm: shoulder to the hand holding the bow's grip
	gripZ = nockZ - 0.62
	leftShoulder = ( bodyX, 1.43, -0.19 )
	builder.Rod( leftShoulder, ( nockX + 0.03, nockY - 0.02, gripZ + 0.04 ), 0.045, C_SHIRT )
	# The drawing arm: shoulder, elbow pulled back at shoulder height, hand at the nock under the chin
	rightShoulder = ( bodyX, 1.43, 0.19 )
	rightElbow = ( bodyX + 0.05, 1.52, 0.48 )
	builder.Rod( rightShoulder, rightElbow, 0.045, C_SHIRT )
	builder.Rod( rightElbow, ( nockX + 0.03, nockY - 0.01, nockZ + 0.03 ), 0.04, C_SKIN )
	builder.Sphere( ( nockX + 0.035, nockY - 0.02, nockZ + 0.03 ), 0.045, C_SKIN )
	builder.Sphere( ( nockX + 0.03, nockY - 0.03, gripZ + 0.02 ), 0.045, C_SKIN )

	# The recurve bow, drawn: the limbs bend back toward the archer and the string is pulled to the nock
	riserTop, riserBottom = nockY + 0.3, nockY - 0.32
	builder.Box( ( nockX, nockY - 0.01, gripZ ), ( 0.035, riserTop - riserBottom, 0.05 ), C_RISER )
	for sign in ( 1, -1 ):
		limbRoot = ( nockX, nockY + sign * 0.3, gripZ )
		limbMiddle = ( nockX, nockY + sign * 0.6, gripZ + 0.1 )
		limbTip = ( nockX, nockY + sign * 0.82, gripZ + 0.27 )
		recurveTip = ( nockX, nockY + sign * 0.86, gripZ + 0.22 )
		builder.Rod( limbRoot, limbMiddle, 0.016, C_LIMB )
		builder.Rod( limbMiddle, limbTip, 0.012, C_LIMB )
		builder.Rod( limbTip, recurveTip, 0.01, C_LIMB )
		builder.Rod( recurveTip, ( nockX, nockY, nockZ ), 0.003, C_STRING, i_segments = 4 )
	# The long stabilizer rod pointing at the target, a V-bar with short side rods, and the sight
	builder.Rod( ( nockX, nockY - 0.12, gripZ ), ( nockX, nockY - 0.16, gripZ - 0.75 ), 0.01, C_LIMB )
	builder.Sphere( ( nockX, nockY - 0.16, gripZ - 0.75 ), 0.022, C_DARK_STEEL )
	for sign in ( 1, -1 ):
		builder.Rod( ( nockX, nockY - 0.12, gripZ ), ( nockX + sign * 0.22, nockY - 0.2, gripZ + 0.25 ), 0.008, C_LIMB )
	builder.Rod( ( nockX, nockY + 0.12, gripZ ), ( nockX - 0.02, nockY + 0.12, gripZ - 0.3 ), 0.006, C_DARK_STEEL )
	builder.Box( ( nockX - 0.02, nockY + 0.12, gripZ - 0.3 ), ( 0.02, 0.03, 0.01 ), C_WARNING_RED )
	return builder.ToObject( collection_environment )

# The arrow
#==========
# Modeled with its nock at the origin, pointing down -z (its tip is 0.75 m in front)

def BuildArrow():
	builder = cMeshBuilder( "Arrow" )
	length = 0.75
	builder.Rod( ( 0, 0, -0.02 ), ( 0, 0, -length + 0.04 ), 0.0045, ( 40, 40, 45 ) )
	# The point and the nock
	builder.Rod( ( 0, 0, -length + 0.045 ), ( 0, 0, -length ), 0.0055, ( 200, 200, 205 ), i_segments = 6 )
	builder.Rod( ( 0, 0, 0.005 ), ( 0, 0, -0.025 ), 0.006, ( 255, 120, 20 ) )
	# Three vanes (fletching), one a different color like a real "cock vane"
	for index, angle in enumerate( ( 90.0, 210.0, 330.0 ) ):
		radians = math.radians( angle )
		offset = ( 0.012 * math.cos( radians ), 0.012 * math.sin( radians ) )
		color = ( 255, 120, 20 ) if index == 0 else ( 245, 245, 245 )
		builder.Box( ( offset[0], offset[1], -0.075 ), ( 0.002 + 0.016 * abs( math.cos( radians ) ), 0.002 + 0.016 * abs( math.sin( radians ) ), 0.07 ), color )
	return builder.ToObject( collection_moving )

# The target
#===========
# A 130 cm square straw boss tilted back 12 degrees on a wooden stand, with the 122 cm face on its front.
# Modeled around the point on the ground under the face's center (that is the target's position in the game).

def BuildTarget():
	builder = cMeshBuilder( "Target" )
	faceCenter = ( 0.0, TARGET_FACE_CENTER_HEIGHT, 0.0 )
	half = 0.65
	depth = 0.35
	# The boss front (the target face) is its own quad so that its UVs can cover exactly the face's area of the texture
	# (1/16 to 1 on both axes; the top-left 1/16 is the white corner for the vertex-colored stand).
	# In Blender UVs, v = 1 is the top of the image.
	start = 1.0 / 16.0
	y = faceCenter[1]
	face = builder.Quad( [ ( -half, y - half, 0.0 ), ( half, y - half, 0.0 ), ( half, y + half, 0.0 ), ( -half, y + half, 0.0 ) ],
		( 255, 255, 255 ), [ ( start, 0.0 ), ( 1.0, 0.0 ), ( 1.0, 1.0 - start ), ( start, 1.0 - start ) ], ( 0, 0, 1 ) )
	# The rest of the boss uses a straw-colored spot on the texture (outside the face's circle)
	boss = builder.Box( ( 0.0, y, -depth / 2 - 0.002 ), ( 2 * half, 2 * half, depth ), ( 255, 255, 255 ), lambda p, n: ( 0.1, 0.03 ) )
	# The whole boss leans back: its top tilts away from the archer around the face's center
	bmesh.ops.rotate( builder.bm, cent = E( *faceCenter ), matrix = Matrix.Rotation( math.radians( TARGET_TILT_DEGREES ), 3, 'X' ),
		verts = list( { v for f in face + boss for v in f.verts } ) )
	builder.bm.normal_update()
	# The stand: two A-frame legs on each side and a cross bar
	legColor = ( 150, 110, 70 )
	for x in ( -0.55, 0.55 ):
		builder.Rod( ( x, 0.0, 0.3 ), ( x, y + 0.5, -0.25 ), 0.04, legColor )
		builder.Rod( ( x, 0.0, -0.9 ), ( x, y + 0.5, -0.3 ), 0.04, legColor )
	builder.Rod( ( -0.6, 0.55, -0.3 ), ( 0.6, 0.55, -0.3 ), 0.035, legColor )
	return builder.ToObject( collection_props )

# The obstacles
#==============
# Each obstacle is a fixed frame (placed by the game) plus a moving wooden part (moved by the game).
# Frames are painted steel scaffolding; the moving parts are wood.

def BuildRailSlider():
	frame = cMeshBuilder( "RailSlider_Frame" )
	for x in ( -7.0, 7.0 ):
		frame.Box( ( x, 1.7, 0.0 ), ( 0.2, 3.4, 0.2 ), C_STEEL )
		frame.Box( ( x, 0.05, 0.0 ), ( 0.6, 0.1, 0.6 ), C_DARK_STEEL )
		# Diagonal braces
		frame.Rod( ( x, 0.1, 0.8 ), ( x, 2.2, 0.0 ), 0.04, C_STEEL )
		frame.Rod( ( x, 0.1, -0.8 ), ( x, 2.2, 0.0 ), 0.04, C_STEEL )
	frame.Box( ( 0.0, 3.2, 0.0 ), ( 14.4, 0.15, 0.12 ), C_DARK_STEEL )
	plank = cMeshBuilder( "RailSlider_Plank" )
	plank.Box( ( 0.0, 2.15, 0.0 ), ( 2.0, 1.5, 0.06 ), ( 255, 255, 255 ), UvWood )
	for x in ( -0.7, 0.7 ):
		plank.Box( ( x, 3.0, 0.0 ), ( 0.05, 0.3, 0.05 ), ( 90, 90, 95 ), UvWood )
	plank.Box( ( 0.0, 3.28, 0.0 ), ( 1.6, 0.06, 0.2 ), ( 90, 90, 95 ), UvWood )
	return frame.ToObject( collection_props ), plank.ToObject( collection_moving )

def BuildBunker():
	bunker = cMeshBuilder( "Bunker" )
	# Three staggered courses of sandbags, 8 m wide and 0.9 m tall
	for course in range( 3 ):
		y = 0.15 + course * 0.3
		offset = 0.3 if course % 2 else 0.0
		x = -4.0 + offset
		while x < 4.0 - 0.3:
			shade = random.uniform( 0.9, 1.08 )
			color = tuple( min( 255, int( c * shade ) ) for c in C_SANDBAG )
			for z in ( -0.2, 0.2 ):
				bunker.Box( ( x + 0.3, y, z ), ( 0.58, 0.28, 0.38 ), color )
			x += 0.6
	popup = cMeshBuilder( "PopupBoard" )
	# Built with its bottom edge at the origin; a red band across its top makes it visible above the sandbags while it waits
	popup.Box( ( 0.0, 0.9, 0.0 ), ( 1.4, 1.8, 0.08 ), ( 255, 255, 255 ), UvWood )
	popup.Box( ( 0.0, 1.72, 0.045 ), ( 1.4, 0.16, 0.01 ), C_WARNING_RED, UvWood )
	popup.Box( ( 0.0, 1.72, -0.045 ), ( 1.4, 0.16, 0.01 ), C_WARNING_RED, UvWood )
	return bunker.ToObject( collection_props ), popup.ToObject( collection_moving )

def BuildPulley():
	frame = cMeshBuilder( "Pulley_Frame" )
	for x in ( -3.2, 3.2 ):
		frame.Box( ( x, 3.4, 0.0 ), ( 0.25, 6.8, 0.25 ), C_STEEL )
		frame.Box( ( x, 0.05, 0.0 ), ( 0.8, 0.1, 0.8 ), C_DARK_STEEL )
		frame.Rod( ( x, 0.1, 1.2 ), ( x, 3.0, 0.0 ), 0.05, C_STEEL )
		frame.Rod( ( x, 0.1, -1.2 ), ( x, 3.0, 0.0 ), 0.05, C_STEEL )
	frame.Box( ( 0.0, 6.65, 0.0 ), ( 7.0, 0.3, 0.3 ), C_DARK_STEEL )
	for x in ( -0.9, 0.9 ):
		frame.Cylinder( ( x, 6.25, 0.0 ), 0.12, 0.25, C_DARK_STEEL, i_segments = 8 )
	board = cMeshBuilder( "Pulley_Board" )
	# Built with the middle of its top edge at the origin (where the ropes are tied)
	board.Box( ( 0.0, -0.7, 0.0 ), ( 2.4, 1.4, 0.06 ), ( 255, 255, 255 ), UvWood )
	for x in ( -0.9, 0.9 ):
		board.Box( ( x, 0.03, 0.0 ), ( 0.08, 0.06, 0.08 ), ( 90, 90, 95 ), UvWood )
	rope = cMeshBuilder( "Rope" )
	# 1 m long, hanging down from the origin; the game stretches it to reach from the pulley to the board
	rope.Rod( ( 0.0, 0.0, 0.0 ), ( 0.0, -1.0, 0.0 ), 0.015, C_ROPE, i_segments = 6 )
	return frame.ToObject( collection_props ), board.ToObject( collection_moving ), rope.ToObject( collection_moving )

def BuildSled():
	track = cMeshBuilder( "Sled_Track" )
	for z in ( -0.35, 0.35 ):
		track.Box( ( 0.0, 0.04, z ), ( 12.4, 0.08, 0.1 ), C_DARK_STEEL )
	for x in [ -6.0 + i * 1.5 for i in range( 9 ) ]:
		track.Box( ( x, 0.02, 0.0 ), ( 0.2, 0.04, 1.0 ), C_STEEL )
	sled = cMeshBuilder( "Sled_Plank" )
	sled.Box( ( 0.0, 0.8, 0.0 ), ( 1.8, 1.3, 0.06 ), ( 255, 255, 255 ), UvWood )
	sled.Box( ( 0.0, 0.11, 0.0 ), ( 1.0, 0.1, 0.8 ), ( 90, 90, 95 ), UvWood )
	for x in ( -0.4, 0.4 ):
		sled.Box( ( x, 0.13, 0.0 ), ( 0.06, 0.06, 0.1 ), ( 90, 90, 95 ), UvWood )
		sled.Rod( ( x, 0.15, 0.0 ), ( x * 0.5, 0.6, 0.15 ), 0.03, ( 90, 90, 95 ) )
	return track.ToObject( collection_props ), sled.ToObject( collection_moving )

def BuildFlag():
	pole = cMeshBuilder( "FlagPole" )
	pole.Cylinder( ( 0.0, 0.0, 0.0 ), 0.05, 5.0, C_WHITE, i_segments = 8, i_topRadius = 0.035 )
	pole.Sphere( ( 0.0, 5.05, 0.0 ), 0.07, C_FLAG )
	flag = cMeshBuilder( "Flag" )
	# 1.2 m long along +x from the pole (the game turns it and stretches it with the wind), with a gentle wave in it.
	# Both sides are built because the engine doesn't draw the back of a face.
	columns = 6
	for side in ( 1, -1 ):
		for i in range( columns ):
			x0, x1 = 1.2 * i / columns, 1.2 * ( i + 1 ) / columns
			z0, z1 = 0.06 * math.sin( x0 * 5.0 ), 0.06 * math.sin( x1 * 5.0 )
			corners = [ ( x0, 4.2, z0 ), ( x1, 4.2, z1 ), ( x1, 4.9, z1 ), ( x0, 4.9, z0 ) ]
			color = C_FLAG if ( i % 2 == 0 ) else ( 230, 60, 40 )
			flag.Quad( corners, color, [ ( 0.02, 0.98 ) ] * 4, ( 0, 0, side ) )
	return pole.ToObject( collection_props ), flag.ToObject( collection_moving )

# The big screen
#===============
# A stadium video wall that shows the round's score (see scoreboard_layout.py for its layout).
# It is three kinds of mesh, all modeled with the ground under the middle of the screen at the origin and the screen facing +z:
#	* The cabinet and its posts, which are lit and baked like the other props
#	* The display: the screen's black background, its labels, and the bars under the arrow slots.
#		It is drawn unlit (it gives off its own light) with the scoreboard texture.
#	* One small quad for each character that a number can use ("0" to "9", "M" for a miss, and "-" for no score yet).
#		The game draws these wherever the numbers need them, scaled to the right size (see OneShotGame/cScoreboard.cpp).
# The layers are a few centimeters apart so that the depth buffer can always tell which is in front, even from the archer 75 m away.

def BuildScoreboard():
	layout = scoreboard_layout
	frame = cMeshBuilder( "Scoreboard_Frame" )
	centerHeight = layout.DISPLAY_CENTER_HEIGHT
	bezel = 0.3
	cabinetWidth, cabinetHeight, cabinetDepth = layout.DISPLAY_WIDTH + 2 * bezel, layout.DISPLAY_HEIGHT + 2 * bezel, 0.8
	frame.Box( ( 0.0, centerHeight, 0.0 ), ( cabinetWidth, cabinetHeight, cabinetDepth ), C_CABINET )
	cabinetBottom = centerHeight - cabinetHeight / 2
	# Two posts with braces, and a walkway for maintenance under the cabinet
	for x in ( -5.5, 5.5 ):
		frame.Box( ( x, cabinetBottom / 2, -0.1 ), ( 0.5, cabinetBottom, 0.5 ), C_STEEL )
		frame.Box( ( x, 0.05, -0.1 ), ( 1.4, 0.1, 1.4 ), C_DARK_STEEL )
		frame.Rod( ( x, 0.1, -1.6 ), ( x, cabinetBottom - 0.4, -0.2 ), 0.08, C_STEEL )
	frame.Box( ( 0.0, cabinetBottom - 0.1, -0.2 ), ( cabinetWidth - 1.0, 0.2, 1.2 ), C_DARK_STEEL )
	frame.Box( ( 0.0, cabinetBottom + cabinetHeight + 0.15, -0.1 ), ( cabinetWidth + 0.2, 0.3, cabinetDepth + 0.3 ), C_DARK_STEEL )

	display = cMeshBuilder( "Scoreboard_Display" )
	def AtlasQuad( i_left, i_right, i_bottom, i_top, i_z, i_atlasName, i_color, i_inset = 0.0 ):
		u0, u1, v0, v1 = layout.AtlasUvs( i_atlasName, i_inset )
		y0, y1 = centerHeight + i_bottom, centerHeight + i_top
		display.Quad( [ ( i_left, y0, i_z ), ( i_right, y0, i_z ), ( i_right, y1, i_z ), ( i_left, y1, i_z ) ],
			i_color, [ ( u0, v0 ), ( u1, v0 ), ( u1, v1 ), ( u0, v1 ) ], ( 0, 0, 1 ) )
	# The black background
	halfWidth, halfHeight = layout.DISPLAY_WIDTH / 2, layout.DISPLAY_HEIGHT / 2
	AtlasQuad( -halfWidth, halfWidth, -halfHeight, halfHeight, layout.DISPLAY_BACKGROUND_Z, "solid_black", ( 255, 255, 255 ), 4.0 )
	# The labels
	for text, left, bottom, height in layout.LABEL_PLACEMENTS:
		x0, x1, y0, y1 = layout.TextQuadCorners( left, bottom, text, height )
		AtlasQuad( x0, x1, y0, y1, layout.DISPLAY_LABELS_Z, "label_" + text, C_LED_WHITE )
	# A bar under each arrow's slot (so an empty slot shows that an arrow is still to come)
	slotWidth = layout.TextWidth( 2, layout.SLOT_HEIGHT )
	for slot in range( layout.SLOT_COUNT ):
		center = layout.SlotCenterX( slot )
		AtlasQuad( center - slotWidth / 2, center + slotWidth / 2, layout.SLOT_BAR_BOTTOM, layout.SLOT_BAR_BOTTOM + layout.SLOT_BAR_HEIGHT,
			layout.DISPLAY_LABELS_Z, "solid_white", C_LED_BAR, 4.0 )
	return frame.ToObject( collection_props ), display.ToObject( collection_moving )

def BuildScoreGlyphs():
	"""
		One quad per character, 1 m tall (the height of its dots), with the bottom-left of its dots at the origin.
		The game scales each one to the size it needs and moves it in front of the screen.
	"""
	glyphs = []
	for glyph in scoreboard_layout.NUMBER_GLYPHS:
		builder = cMeshBuilder( f"ScoreGlyph_{glyph}" )
		x0, x1, y0, y1 = scoreboard_layout.TextQuadCorners( 0.0, 0.0, glyph, 1.0 )
		u0, u1, v0, v1 = scoreboard_layout.AtlasUvs( "glyph_" + glyph )
		builder.Quad( [ ( x0, y0, 0.0 ), ( x1, y0, 0.0 ), ( x1, y1, 0.0 ), ( x0, y1, 0.0 ) ],
			C_LED_RED if glyph == "M" else C_LED_AMBER, [ ( u0, v0 ), ( u1, v0 ), ( u1, v1 ), ( u0, v1 ) ], ( 0, 0, 1 ) )
		glyphs.append( ( glyph, builder.ToObject( collection_moving ) ) )
	return glyphs

# The quiver
#===========
# A ground quiver: a tube on three legs that holds the archer's arrows.
# The game draws one arrow in it for every arrow that is left to shoot (so the quiver itself shows how many are left).

def BuildQuiver():
	builder = cMeshBuilder( "Quiver" )
	builder.Cylinder( ( 0.0, 0.25, 0.0 ), 0.065, 0.6, C_QUIVER, i_segments = 10 )
	builder.Cylinder( ( 0.0, 0.8, 0.0 ), 0.072, 0.06, C_DARK_STEEL, i_segments = 10 )
	for angle in ( 90.0, 210.0, 330.0 ):
		radians = math.radians( angle )
		builder.Rod( ( 0.05 * math.cos( radians ), 0.45, 0.05 * math.sin( radians ) ),
			( 0.28 * math.cos( radians ), 0.0, 0.28 * math.sin( radians ) ), 0.012, C_DARK_STEEL )
	return builder.ToObject( collection_props )

# The timing light
#=================
# Real archery competitions control shooting with lights at the shooting line:
# red means stop, green means shoot, and yellow means 30 seconds are left.
# This one is lit by the game: green (with "SPACE" under it) while the arrow is ready, and red otherwise.
# It is modeled with the ground under its post at the origin, facing +z, as four meshes:
#	* The housing and post with dark (unlit) lenses (lit and baked like the other props)
#	* A bright red lamp and a bright green lamp, which the game draws (unlit) in front of the dark lenses to turn them on
#	* The word "SPACE" from the scoreboard's texture, which the game draws on the black panel under the lamps

TIMING_LIGHT_LAMP_HEIGHTS = { "red": 2.05, "yellow": 1.75, "green": 1.45 }
TIMING_LIGHT_LAMP_RADIUS = 0.12
TIMING_LIGHT_FRONT_Z = 0.1
TIMING_LIGHT_PANEL_HEIGHT = 1.18
TIMING_LIGHT_LABEL_HEIGHT = 0.12

def AddDisc( io_builder, i_center, i_radius, i_color, i_segments = 20 ):
	""" A flat disc facing +z, made of triangles around its center """
	cx, cy, cz = i_center
	ring = [ ( cx + i_radius * math.cos( 2 * math.pi * i / i_segments ), cy + i_radius * math.sin( 2 * math.pi * i / i_segments ), cz )
		for i in range( i_segments ) ]
	faces = []
	for i in range( i_segments ):
		a, b = ring[i], ring[( i + 1 ) % i_segments]
		corners = [ i_center, a, b ]
		verts = [ io_builder.bm.verts.new( E( *c ) ) for c in corners ]
		face = io_builder.bm.faces.new( verts )
		faces.append( face )
	io_builder.bm.normal_update()
	for face in faces:
		if face.normal.dot( E( 0, 0, 1 ) ) < 0.0:
			face.normal_flip()
	io_builder.bm.normal_update()
	io_builder._Finish( faces, i_color, UvWhite )
	return faces

def BuildTimingLight():
	frame = cMeshBuilder( "TimingLight_Frame" )
	width = 0.6
	housingBottom, housingTop = 1.05, 2.25
	# The post and its base
	frame.Box( ( 0.0, housingBottom / 2, -0.05 ), ( 0.08, housingBottom, 0.08 ), C_STEEL )
	frame.Box( ( 0.0, 0.03, -0.05 ), ( 0.4, 0.06, 0.4 ), C_DARK_STEEL )
	# The housing (its front is at TIMING_LIGHT_FRONT_Z)
	housingDepth = 0.2
	frame.Box( ( 0.0, ( housingBottom + housingTop ) / 2, TIMING_LIGHT_FRONT_Z - housingDepth / 2 ),
		( width, housingTop - housingBottom, housingDepth ), C_CABINET )
	# The dark lenses, each with a visor above it to keep the sun off
	darkLenses = { "red": ( 70, 18, 16 ), "yellow": ( 70, 52, 12 ), "green": ( 14, 56, 28 ) }
	for name, height in TIMING_LIGHT_LAMP_HEIGHTS.items():
		AddDisc( frame, ( 0.0, height, TIMING_LIGHT_FRONT_Z + 0.002 ), TIMING_LIGHT_LAMP_RADIUS, darkLenses[name] )
		frame.Box( ( 0.0, height + TIMING_LIGHT_LAMP_RADIUS + 0.02, TIMING_LIGHT_FRONT_Z + 0.06 ), ( 0.3, 0.015, 0.12 ), C_CABINET )
	# The black panel that the word is shown on
	frame.Box( ( 0.0, TIMING_LIGHT_PANEL_HEIGHT, TIMING_LIGHT_FRONT_Z + 0.005 ), ( width - 0.06, 0.2, 0.01 ), ( 8, 8, 10 ) )

	lamps = {}
	for name, color in ( ( "red", ( 255, 45, 35 ) ), ( "green", ( 60, 255, 110 ) ) ):
		lamp = cMeshBuilder( f"TimingLight_{name.capitalize()}" )
		AddDisc( lamp, ( 0.0, TIMING_LIGHT_LAMP_HEIGHTS[name], TIMING_LIGHT_FRONT_Z + 0.012 ), TIMING_LIGHT_LAMP_RADIUS, color )
		lamps[name] = lamp.ToObject( collection_moving )

	label = cMeshBuilder( "TimingLight_Label" )
	text = scoreboard_layout.TIMING_LIGHT_LABEL
	height = TIMING_LIGHT_LABEL_HEIGHT
	left = -scoreboard_layout.TextWidth( len( text ), height ) / 2
	x0, x1, y0, y1 = scoreboard_layout.TextQuadCorners( left, TIMING_LIGHT_PANEL_HEIGHT - height / 2, text, height )
	u0, u1, v0, v1 = scoreboard_layout.AtlasUvs( "label_" + text )
	z = TIMING_LIGHT_FRONT_Z + 0.02
	label.Quad( [ ( x0, y0, z ), ( x1, y0, z ), ( x1, y1, z ), ( x0, y1, z ) ], ( 60, 255, 110 ),
		[ ( u0, v0 ), ( u1, v0 ), ( u1, v1 ), ( u0, v1 ) ], ( 0, 0, 1 ) )
	return frame.ToObject( collection_props ), lamps["red"], lamps["green"], label.ToObject( collection_moving )

# The sky
#========
# A dome that the game keeps centered on the camera, colored from a pale morning haze at the horizon to blue overhead.
# (It is unlit: the sky is the light source, so shading it would just make it look like a painted ceiling.)

def BuildSkyDome():
	builder = cMeshBuilder( "SkyDome" )
	geometry = bmesh.ops.create_uvsphere( builder.bm, u_segments = 32, v_segments = 16, radius = 280.0 )["verts"]
	faces = list( { f for v in geometry for f in v.link_faces } )
	# The camera is inside the dome, so its faces must point inward
	for face in faces:
		face.normal_flip()
	builder.bm.normal_update()
	horizon, zenith, below = ( 205, 222, 236 ), ( 80, 140, 210 ), ( 170, 190, 200 )
	for face in faces:
		for loop in face.loops:
			height = loop.vert.co.z / 280.0
			if height >= 0:
				amount = min( 1.0, height / 0.6 ) ** 0.7
				color = [ SrgbToLinear( h ) * ( 1 - amount ) + SrgbToLinear( z ) * amount for h, z in zip( horizon, zenith ) ]
			else:
				color = [ SrgbToLinear( c ) for c in below ]
			loop[builder.colorLayer] = ( *color, 1.0 )
			loop[builder.uvLayer].uv = ( 0.02, 0.98 )
	return builder.ToObject( collection_moving, i_smooth = True )

# Build Everything
#=================

field = BuildGround()
lines = BuildLines()
stands, crowd = BuildGrandstand()
surroundings = BuildSurroundings()
shooter = BuildShooter()
arrow = BuildArrow()
target = BuildTarget()
railFrame, railPlank = BuildRailSlider()
bunker, popupBoard = BuildBunker()
pulleyFrame, pulleyBoard, rope = BuildPulley()
sledTrack, sledPlank = BuildSled()
flagPole, flag = BuildFlag()
skyDome = BuildSkyDome()
scoreboardFrame, scoreboardDisplay = BuildScoreboard()
scoreGlyphs = BuildScoreGlyphs()
quiver = BuildQuiver()
timingLightFrame, timingLightRed, timingLightGreen, timingLightLabel = BuildTimingLight()

Export( "range_lines.blendermesh", [ lines ] )
Export( "range_stands.blendermesh", stands )
for group, crowdGroup in enumerate( crowd ):
	Export( f"crowd_{group}.blendermesh", [ crowdGroup ] )
Export( "range_surroundings.blendermesh", surroundings )
Export( "shooter.blendermesh", [ shooter ] )
Export( "arrow.blendermesh", [ arrow ], ( 0.0, 0.0, 0.0 ) )
Export( "target.blendermesh", [ target ], ( 0.0, 0.0, 0.0 ), i_texture = "target" )
Export( "railSlider_frame.blendermesh", [ railFrame ], ( 0.0, 0.0, 0.0 ) )
Export( "railSlider_plank.blendermesh", [ railPlank ], ( 0.0, 0.0, 0.0 ), i_texture = "wood" )
Export( "bunker.blendermesh", [ bunker ], ( 0.0, 0.0, 0.0 ) )
Export( "popupBoard.blendermesh", [ popupBoard ], ( 0.0, 0.0, 0.0 ), i_texture = "wood" )
Export( "pulley_frame.blendermesh", [ pulleyFrame ], ( 0.0, 0.0, 0.0 ) )
Export( "pulley_board.blendermesh", [ pulleyBoard ], ( 0.0, 0.0, 0.0 ), i_texture = "wood" )
Export( "rope.blendermesh", [ rope ], ( 0.0, 0.0, 0.0 ) )
Export( "sled_track.blendermesh", [ sledTrack ], ( 0.0, 0.0, 0.0 ) )
Export( "sled_plank.blendermesh", [ sledPlank ], ( 0.0, 0.0, 0.0 ), i_texture = "wood" )
Export( "flagPole.blendermesh", [ flagPole ], ( 0.0, 0.0, 0.0 ) )
Export( "flag.blendermesh", [ flag ], ( 0.0, 0.0, 0.0 ) )
Export( "skyDome.blendermesh", [ skyDome ], ( 0.0, 0.0, 0.0 ) )
Export( "scoreboard_frame.blendermesh", [ scoreboardFrame ], ( 0.0, 0.0, 0.0 ) )
Export( "scoreboard_display.blendermesh", [ scoreboardDisplay ], ( 0.0, 0.0, 0.0 ), i_texture = "scoreboard" )
for glyph, glyphObject in scoreGlyphs:
	# ("-" is spelled out so that the file name doesn't look like a command line option)
	Export( "scoreGlyph_{}.blendermesh".format( "dash" if glyph == "-" else glyph ), [ glyphObject ], ( 0.0, 0.0, 0.0 ), i_texture = "scoreboard" )
Export( "quiver.blendermesh", [ quiver ], ( 0.0, 0.0, 0.0 ) )
Export( "timingLight_frame.blendermesh", [ timingLightFrame ], ( 0.0, 0.0, 0.0 ) )
Export( "timingLight_red.blendermesh", [ timingLightRed ], ( 0.0, 0.0, 0.0 ) )
Export( "timingLight_green.blendermesh", [ timingLightGreen ], ( 0.0, 0.0, 0.0 ) )
Export( "timingLight_label.blendermesh", [ timingLightLabel ], ( 0.0, 0.0, 0.0 ), i_texture = "scoreboard" )

# Placing the props in the layout
#================================
# The props and moving parts were built at the origin.
# For baking (and for looking at the .blend) linked copies are placed where the game puts them;
# the originals stay at the origin so they can be exported in their own local space.

def PlaceCopy( i_object, i_position, i_name, i_collection, i_yawDegrees = 0.0 ):
	copy = i_object.copy()
	copy.name = i_name
	copy.location = E( *i_position )
	# A turn around the engine's vertical (y) axis is a turn the other way around Blender's Z axis,
	# because the exporter's conversion between the two mirrors the scene
	copy.rotation_euler = ( 0.0, 0.0, -math.radians( i_yawDegrees ) )
	i_collection.objects.link( copy )
	return copy

collection_layout = NewCollection( "Layout preview (copies placed like the game places them)" )
layoutCopies = []
# The fixed props are exported from these placed (and baked) copies,
# so that their baked shading includes the ground and everything else around them
bakedCopyOf = {}
def PlaceStaticProp( i_object, i_position, i_name, i_yawDegrees = 0.0 ):
	copy = PlaceCopy( i_object, i_position, i_name, collection_layout, i_yawDegrees )
	layoutCopies.append( copy )
	bakedCopyOf.setdefault( i_object, copy )
	return copy
PlaceStaticProp( target, TARGET_POSITION, "Target_Player" )
for x in NEIGHBOR_TARGET_XS:
	PlaceStaticProp( target, ( x, 0.0, TARGET_POSITION[2] ), f"Target_Lane_{x:+.0f}" )
PlaceStaticProp( railFrame, ( 0.0, 0.0, RAIL_SLIDER_Z ), "RailSlider_Frame_Placed" )
PlaceStaticProp( bunker, ( 0.0, 0.0, BUNKER_Z ), "Bunker_Placed" )
PlaceStaticProp( pulleyFrame, ( 0.0, 0.0, PULLEY_Z ), "Pulley_Frame_Placed" )
PlaceStaticProp( sledTrack, ( 0.0, 0.0, SLED_Z ), "Sled_Track_Placed" )
for x, z in FLAG_POSITIONS:
	PlaceStaticProp( flagPole, ( x, 0.0, z ), f"FlagPole_{z:.0f}" )
PlaceStaticProp( scoreboardFrame, SCOREBOARD_POSITION, "Scoreboard_Frame_Placed", SCOREBOARD_YAW_DEGREES )
PlaceStaticProp( quiver, QUIVER_POSITION, "Quiver_Placed" )
PlaceStaticProp( timingLightFrame, TIMING_LIGHT_POSITION, "TimingLight_Frame_Placed", TIMING_LIGHT_YAW_DEGREES )
# The moving parts in a typical spot (for the preview only; they aren't baked)
movingCopies = [
	PlaceCopy( arrow, ARROW_REST, "Arrow_OnTheBow", collection_layout ),
	PlaceCopy( railPlank, ( 1.5, 0.0, RAIL_SLIDER_Z ), "RailSlider_Plank_Placed", collection_layout ),
	PlaceCopy( popupBoard, ( -1.6, -0.8, BUNKER_Z - 0.6 ), "PopupBoard_Left_Placed", collection_layout ),
	PlaceCopy( popupBoard, ( 1.6, -0.8, BUNKER_Z - 0.6 ), "PopupBoard_Right_Placed", collection_layout ),
	PlaceCopy( pulleyBoard, ( 0.0, 3.6, PULLEY_Z ), "Pulley_Board_Placed", collection_layout ),
	PlaceCopy( sledPlank, ( -2.0, 0.0, SLED_Z ), "Sled_Plank_Placed", collection_layout ),
]
for x, z in FLAG_POSITIONS:
	movingCopies.append( PlaceCopy( flag, ( x, 0.0, z ), f"Flag_{z:.0f}", collection_layout ) )
movingCopies.append( PlaceCopy( scoreboardDisplay, SCOREBOARD_POSITION, "Scoreboard_Display_Placed", collection_layout, SCOREBOARD_YAW_DEGREES ) )
movingCopies.append( PlaceCopy( timingLightGreen, TIMING_LIGHT_POSITION, "TimingLight_Green_Placed", collection_layout, TIMING_LIGHT_YAW_DEGREES ) )
movingCopies.append( PlaceCopy( timingLightLabel, TIMING_LIGHT_POSITION, "TimingLight_Label_Placed", collection_layout, TIMING_LIGHT_YAW_DEGREES ) )

# Baking
#=======
# The engine can't calculate shadows or ambient occlusion, so they are calculated once here (with Cycles)
# and multiplied into the vertex colors.
# Only things that never move are baked against the whole scene;
# a moving part's shading would be wrong as soon as it moved, so moving parts only get the base colors.

def AddAttribute( i_object, i_name ):
	mesh = i_object.data
	if i_name in mesh.color_attributes:
		mesh.color_attributes.remove( mesh.color_attributes[i_name] )
	return mesh.color_attributes.new( i_name, 'FLOAT_COLOR', 'POINT' )

bakeMaterial = bpy.data.materials.new( "BakeOnly" )
bakeMaterial.use_nodes = True
# The painted lines aren't baked: they sit 3 cm above the grass, so they would "occlude" the grass right under them
# and leave a dark band along every line (and they are too thin for their own shading to matter)
bakedObjects = [ o for o in collection_environment.objects if o != lines ] + layoutCopies
# The baking works on each object's own mesh, so the copies of the props are given their own mesh data while baking
for copy in layoutCopies:
	copy.data = copy.data.copy()
for obj in bakedObjects:
	if not obj.data.materials:
		obj.data.materials.append( bakeMaterial )

sun = bpy.data.objects.new( "Sun", bpy.data.lights.new( "Sun", 'SUN' ) )
scene.collection.objects.link( sun )
sunDirection = E( *SUN_DIRECTION ).normalized()
# A sun light shines down its local -Z axis
sun.rotation_euler = Vector( ( 0, 0, -1 ) ).rotation_difference( sunDirection ).to_euler()
sun.data.angle = math.radians( 2.0 )
sun.data.energy = 3.0

scene.render.engine = 'CYCLES'
scene.cycles.device = 'CPU'
scene.cycles.samples = 48
scene.render.bake.target = 'VERTEX_COLORS'
scene.world = bpy.data.worlds.new( "World" )
scene.world.light_settings.distance = 6.0

def Bake( i_type, i_attributeName ):
	for obj in scene.objects:
		obj.select_set( False )
	for obj in bakedObjects:
		attribute = AddAttribute( obj, i_attributeName )
		obj.data.color_attributes.active_color = attribute
		obj.select_set( True )
	bpy.context.view_layer.objects.active = bakedObjects[0]
	print( "Baking", i_type, bpy.ops.object.bake( type = i_type ) )

# The originals (all built at the origin) and the moving copies must not cast shadows or occlusion while baking:
# the originals aren't really there, and the moving parts' shadows would stay behind when they move
hiddenWhileBaking = movingCopies + list( collection_props.objects ) + list( collection_moving.objects ) + [ lines ]
for obj in hiddenWhileBaking:
	obj.hide_render = True
Bake( 'AO', "AO" )
Bake( 'SHADOW', "Shadow" )
for obj in movingCopies + [ lines ]:
	obj.hide_render = False

# Final vertex colors
#====================
# Color = base color * ambient occlusion * sun shadow, all in linear space and then converted to sRGB
# (the exporter writes the sRGB values, which is what the engine expects for vertex colors)

AO_STRENGTH = 0.55
SHADOW_DARKNESS = 0.45

def FinalizeColors( i_object, i_isBaked ):
	mesh = i_object.data
	base = mesh.color_attributes["Base"]
	final = mesh.color_attributes.new( "Color", 'FLOAT_COLOR', 'CORNER' )
	ao = mesh.color_attributes["AO"] if i_isBaked else None
	shadow = mesh.color_attributes["Shadow"] if i_isBaked else None
	for loop in mesh.loops:
		r, g, b, a = base.data[loop.index].color
		shade = 1.0
		if i_isBaked:
			occlusion = ao.data[loop.vertex_index].color[0]
			lit = shadow.data[loop.vertex_index].color[0]
			shade = ( 1.0 - AO_STRENGTH + AO_STRENGTH * occlusion ) * ( 1.0 - SHADOW_DARKNESS + SHADOW_DARKNESS * lit )
		final.data[loop.index].color_srgb = ( LinearToSrgb( r * shade ), LinearToSrgb( g * shade ), LinearToSrgb( b * shade ), 1.0 )
	mesh.color_attributes.active_color = final
	# (The exporter uses the active color attribute; the render color is set too so Blender's viewport shows the same thing)
	mesh.color_attributes.render_color_index = mesh.color_attributes.find( "Color" )

for obj in collection_environment.objects:
	FinalizeColors( obj, obj != lines )
# Now that the field has its final colors it can be split for exporting (the whole field stays in the .blend for editing)
for stripName, strip in SplitIntoStrips( field ):
	Export( f"range_ground_{stripName}.blendermesh", [ strip ], i_texture = "grass" )
	strip.hide_set( True )
for copy in layoutCopies:
	FinalizeColors( copy, True )
for obj in list( collection_props.objects ) + list( collection_moving.objects ):
	FinalizeColors( obj, False )

# Materials for viewing in Blender
#=================================
# These aren't exported; they make Blender show roughly what the game will show (texture * vertex color)

def MakeViewMaterial( i_name, i_textureFile ):
	material = bpy.data.materials.new( i_name )
	material.use_nodes = True
	nodes = material.node_tree.nodes
	links = material.node_tree.links
	bsdf = nodes["Principled BSDF"]
	color = nodes.new( "ShaderNodeVertexColor" )
	color.layer_name = "Color"
	multiply = nodes.new( "ShaderNodeMix" )
	multiply.data_type = 'RGBA'
	multiply.blend_type = 'MULTIPLY'
	multiply.inputs["Factor"].default_value = 1.0
	links.new( color.outputs["Color"], multiply.inputs[6] )
	if i_textureFile:
		image = nodes.new( "ShaderNodeTexImage" )
		image.image = bpy.data.images.load( os.path.abspath( os.path.join( textureDirectory, i_textureFile ) ) )
		links.new( image.outputs["Color"], multiply.inputs[7] )
	else:
		multiply.inputs[7].default_value = ( 1, 1, 1, 1 )
	links.new( multiply.outputs[2], bsdf.inputs["Base Color"] )
	bsdf.inputs["Roughness"].default_value = 0.9
	return material

viewMaterials = { name: MakeViewMaterial( f"View_{name}", None if name == "white" else f"{name}.bmp" )
	for name in ( "white", "grass", "wood", "target", "scoreboard" ) }
textureOfObject = {}
for fileName, objects, origin, texture in exports:
	for obj in objects:
		textureOfObject[obj.name] = texture
for obj in scene.objects:
	if obj.type != 'MESH':
		continue
	sourceName = obj.data.name.split( "." )[0]
	texture = textureOfObject.get( obj.name, textureOfObject.get( sourceName, "white" ) )
	obj.data.materials.clear()
	obj.data.materials.append( viewMaterials[texture] )

# Hide the originals at the origin in the viewport (the layout copies show where they really go)
for obj in list( collection_props.objects ) + list( collection_moving.objects ):
	obj.hide_set( True )
	obj.hide_render = True

for screen in bpy.data.screens:
	for area in screen.areas:
		if area.type == 'VIEW_3D':
			for space in area.spaces:
				if space.type == 'VIEW_3D':
					space.shading.type = 'MATERIAL'

# The texture paths are saved relative to the .blend so that it opens correctly on any computer
bpy.ops.wm.save_as_mainfile( filepath = blendPath, compress = True )
bpy.ops.file.make_paths_relative()
bpy.ops.wm.save_as_mainfile( filepath = blendPath, compress = True )

# Exporting
#==========

exporterAddOn.register()
os.makedirs( meshDirectory, exist_ok = True )
for fileName, objects, origin, texture in exports:
	objects = [ bakedCopyOf.get( obj, obj ) for obj in objects ]
	for obj in scene.objects:
		obj.select_set( False )
	savedPlacements = [ ( obj.location.copy(), obj.rotation_euler.copy() ) for obj in objects ]
	for obj in objects:
		obj.hide_set( False )
		obj.select_set( True )
		# A prop is exported in its own local space (the game decides where it goes and which way it faces)
		if origin is not None:
			obj.location = ( 0.0, 0.0, 0.0 )
			obj.rotation_euler = ( 0.0, 0.0, 0.0 )
	bpy.context.view_layer.update()
	exporter = exporterModule.cBlenderMeshExporter( i_selectedOnly = True )
	vertexCount, triangleCount = exporter.Export( bpy.context, os.path.join( meshDirectory, fileName ) )
	print( f"Exported {fileName}: {vertexCount} vertices, {triangleCount} triangles" )
	for obj, ( location, rotation ) in zip( objects, savedPlacements ):
		obj.location = location
		obj.rotation_euler = rotation
exporterAddOn.unregister()
