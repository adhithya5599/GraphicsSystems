"""
	This module defines the Blender mesh exporter

	It is the Blender counterpart of Tools/MayaMeshExporter/cMayaMeshExporter.cpp:
	it writes exactly the same human-readable Lua mesh file,
	which Tools/MeshBuilder then builds into the binary mesh that Engine/Graphics/cMesh loads.
	Nothing downstream of this file knows (or needs to know) which DCC tool authored a mesh.
"""

# Imports
#========

import bpy
import numpy as np

# Static Data
#============

s_vertexCountPerTriangle = 3

# The binary mesh format (see Tools/MeshBuilder/cMeshBuilder.cpp and Engine/Graphics/cMesh.cpp)
# stores the vertex count, the index count, and every index as a uint16_t
s_maxVertexCount = 0xFFFF
s_maxIndexCount = 0xFFFF
s_maxTriangleCount = s_maxIndexCount // s_vertexCountPerTriangle

# The Maya exporter writes floats with std::fixed (i.e. 6 decimal places).
# Vertices are de-duplicated after rounding to that same precision
# so that two vertices are only merged if they would be written identically.
s_decimalPlaces = 6

# Triangles that don't have a material are grouped together under this name
# (the Maya exporter uses the same name for a shading group with no surface shader)
s_unassignedMaterialName = "UNASSIGNED"

# Column layout of the per-corner data that is gathered before de-duplication.
# The material group comes first so that sorting the unique vertices
# puts all of the vertices that use a single material in one contiguous block
# (the same thing that the Maya exporter does with shading groups).
s_column_materialGroup = 0
s_columns_position = slice( 1, 4 )
s_columns_normal = slice( 4, 7 )
s_columns_texcoord = slice( 7, 9 )
s_columns_color = slice( 9, 13 )
s_columnCount = 13

# Exceptions
#===========

class ExportError( Exception ):
	"""An error that should be shown to the user in Blender (the equivalent of MGlobal::displayError())"""
	pass

# Class Definition
#=================

class cBlenderMeshExporter:
	"""
		Gathers the geometry of a Blender scene and writes it to an EAE6320 mesh file

		Maya distinguishes "Export All" from "Export Selection" with two file access modes;
		in Blender the same choice is made with i_selectedOnly
	"""

	# Interface
	#==========

	def __init__( self, i_selectedOnly = False, i_scale = 1.0 ):
		self.m_selectedOnly = i_selectedOnly
		self.m_scale = i_scale

	def Export( self, i_context, i_path ):
		"""
			Writes the mesh file and returns ( vertexCount, triangleCount ).
			Raises ExportError if the scene can't be exported.
		"""
		# Calculate the vertex and index data
		cornerData = self._GatherCorners( i_context )
		vertexArray, indexArray = self._FillVertexAndIndexArrays( cornerData )
		# Make sure the mesh fits in the engine's format
		self._ValidateLimits( vertexArray, indexArray )
		# Write the mesh data to the requested file
		self._WriteMeshToFile( i_path, vertexArray, indexArray )
		return len( vertexArray ), len( indexArray ) // s_vertexCountPerTriangle

	# Implementation
	#===============

	def _GatherCorners( self, i_context ):
		"""
			Returns an array with one row per triangle corner (three consecutive rows per triangle).
			Each row is [ materialGroup, x, y, z, nx, ny, nz, u, v, r, g, b, a ] in the engine's coordinate system.
		"""
		# Edits made in Edit Mode aren't stored in the mesh until Blender syncs them
		for objectInEditMode in ( i_context.objects_in_mode or () ):
			if objectInEditMode.type == 'MESH':
				objectInEditMode.update_from_editmode()

		# The evaluated dependency graph contains the meshes as they are displayed
		# (i.e. with modifiers applied, the same way that the Maya exporter exports the final mesh shape)
		depsgraph = i_context.evaluated_depsgraph_get()

		map_materialNamesToGroups = {}
		cornerArrays = []
		# Object instances include regular objects as well as instanced ones
		# (collection instances, geometry nodes instances, etc.),
		# which is the Blender equivalent of Maya's DAG instancing
		for instance in depsgraph.object_instances:
			evaluatedObject = instance.object
			if evaluatedObject.type != 'MESH':
				continue
			if self.m_selectedOnly:
				# An instance is considered selected if the object that instances it is selected
				selectableObject = instance.parent if instance.is_instance else evaluatedObject
				if not selectableObject.original.select_get():
					continue
			mesh = evaluatedObject.to_mesh()
			try:
				corners = self._ProcessSingleMesh( mesh, evaluatedObject, instance.matrix_world, map_materialNamesToGroups )
			finally:
				evaluatedObject.to_mesh_clear()
			if corners is not None:
				cornerArrays.append( corners )

		if not cornerArrays:
			if self.m_selectedOnly:
				raise ExportError( "Nothing to export: no selected mesh objects have any faces" )
			else:
				raise ExportError( "Nothing to export: the scene has no visible mesh objects with faces" )
		return np.concatenate( cornerArrays )

	def _ProcessSingleMesh( self, i_mesh, i_object, i_matrix_world, io_map_materialNamesToGroups ):
		# Triangulate
		# (loop triangles are Blender's triangulation of each polygon,
		# the equivalent of MItMeshPolygon::getTriangle())
		i_mesh.calc_loop_triangles()
		triangleCount = len( i_mesh.loop_triangles )
		if triangleCount == 0:
			return None
		cornerCount = triangleCount * s_vertexCountPerTriangle

		cornerVertexIndices = np.empty( cornerCount, dtype = np.int32 )
		i_mesh.loop_triangles.foreach_get( "vertices", cornerVertexIndices )
		cornerLoopIndices = np.empty( cornerCount, dtype = np.int32 )
		i_mesh.loop_triangles.foreach_get( "loops", cornerLoopIndices )
		triangleMaterialIndices = np.empty( triangleCount, dtype = np.int32 )
		i_mesh.loop_triangles.foreach_get( "material_index", triangleMaterialIndices )

		# An object with a negative scale (e.g. mirrored with a scale of -1 on one axis)
		# turns every triangle inside out when it is transformed into world space,
		# so the winding order must be flipped to keep the front faces in front
		matrix_world = np.array( i_matrix_world, dtype = np.float64 )
		if np.linalg.det( matrix_world[:3, :3] ) < 0.0:
			for cornerIndices in ( cornerVertexIndices, cornerLoopIndices ):
				cornerIndices_byTriangle = cornerIndices.reshape( -1, s_vertexCountPerTriangle )
				cornerIndices_byTriangle[:, [1, 2]] = cornerIndices_byTriangle[:, [2, 1]]

		corners = np.empty( ( cornerCount, s_columnCount ), dtype = np.float64 )

		# Positions
		corners[:, s_columns_position] = self._CalculateEnginePositions( i_mesh, matrix_world )[cornerVertexIndices]
		# Normals
		corners[:, s_columns_normal] = self._CalculateEngineCornerNormals( i_mesh, matrix_world )[cornerLoopIndices]
		# Texture coordinates
		corners[:, s_columns_texcoord] = self._GetCornerTexcoords( i_mesh )[cornerLoopIndices]
		# Colors
		corners[:, s_columns_color] = self._GetCornerColors( i_mesh, cornerVertexIndices, cornerLoopIndices )
		# Materials
		triangleGroups = self._GetTriangleMaterialGroups( i_object, triangleMaterialIndices, io_map_materialNamesToGroups )
		corners[:, s_column_materialGroup] = np.repeat( triangleGroups, s_vertexCountPerTriangle )

		return corners

	@staticmethod
	def _GetTriangleMaterialGroups( i_object, i_triangleMaterialIndices, io_map_materialNamesToGroups ):
		# A Maya "shading group" is similar to a Blender material.
		# Materials are identified by name so that every object that uses the same material
		# shares a single group (the same way the Maya exporter maps shading group names to indices)
		def GetGroup( i_materialName ):
			return io_map_materialNamesToGroups.setdefault( i_materialName, len( io_map_materialNamesToGroups ) )
		slotGroups = [ GetGroup( slot.material.name if slot.material else s_unassignedMaterialName )
			for slot in i_object.material_slots ]
		if not slotGroups:
			slotGroups.append( GetGroup( s_unassignedMaterialName ) )
		# A polygon's material index can be larger than the number of slots
		# (Blender then displays it with the last slot)
		slotGroups = np.array( slotGroups, dtype = np.float64 )
		return slotGroups[np.clip( i_triangleMaterialIndices, 0, len( slotGroups ) - 1 )]

	def _CalculateEnginePositions( self, i_mesh, i_matrix_world ):
		positions_local = np.empty( len( i_mesh.vertices ) * 3, dtype = np.float32 )
		i_mesh.vertices.foreach_get( "co", positions_local )
		positions_local = positions_local.reshape( -1, 3 ).astype( np.float64 )
		# Transform into world space
		# (the Maya exporter uses MSpace::kWorld, which means that object transforms are baked into the mesh)
		positions_world = ( positions_local @ i_matrix_world[:3, :3].T ) + i_matrix_world[:3, 3]
		positions_world *= self.m_scale
		# Blender is right-handed with +Z up and -Y as "front".
		# Maya is right-handed with +Y up and +Z as "front",
		# which means that a Blender position is ( x, -z, y ) in Maya.
		# The Maya exporter then writes ( x, y, -z ),
		# so the combined conversion from Blender to the engine is ( x, z, y ).
		# Like the Maya conversion this mirrors the geometry,
		# and so the triangle winding order must also be reversed (see _WriteMeshToFile()).
		return positions_world[:, [0, 2, 1]]

	@staticmethod
	def _CalculateEngineCornerNormals( i_mesh, i_matrix_world ):
		# Blender stores a normal for every face corner ("loop").
		# Corner normals already account for smooth vs. flat shading, sharp edges, and custom normals,
		# so they are exactly the normals that Blender's viewport uses to shade the mesh.
		normals_local = np.empty( len( i_mesh.loops ) * 3, dtype = np.float32 )
		i_mesh.corner_normals.foreach_get( "vector", normals_local )
		normals_local = normals_local.reshape( -1, 3 ).astype( np.float64 )
		# Normals can't be transformed with the same matrix as positions:
		# a non-uniform scale would tilt them so that they no longer point away from the surface.
		# The correct matrix is the inverse transpose of the object's rotation/scale matrix.
		# (With row vectors, multiplying by the inverse is the same as multiplying column vectors by the inverse transpose.)
		# Translation doesn't affect directions, so only the 3x3 part is used,
		# and the overall scale of the object doesn't matter because the result is normalized.
		normals_world = normals_local @ np.linalg.inv( i_matrix_world[:3, :3] )
		lengths = np.linalg.norm( normals_world, axis = 1, keepdims = True )
		normals_world = np.divide( normals_world, lengths, out = np.tile( [0.0, 0.0, 1.0], ( len( normals_world ), 1 ) ), where = lengths > 1.0e-12 )
		# The same axis conversion as positions (see _CalculateEnginePositions())
		return normals_world[:, [0, 2, 1]]

	@staticmethod
	def _GetCornerTexcoords( i_mesh ):
		# Like Maya's "default" UV set, the active UV map is used
		# (the one that is highlighted in Blender's UV Maps list)
		uvLayer = i_mesh.uv_layers.active
		if uvLayer is None:
			# The Maya exporter uses ( 0, 0 ) when a mesh has no UVs
			return np.zeros( ( len( i_mesh.loops ), 2 ), dtype = np.float64 )
		texcoords = np.empty( len( i_mesh.loops ) * 2, dtype = np.float32 )
		uvLayer.uv.foreach_get( "vector", texcoords )
		texcoords = texcoords.reshape( -1, 2 ).astype( np.float64 )
		# Blender (like Maya) puts ( 0, 0 ) at the bottom-left of a texture,
		# but the engine's textures have ( 0, 0 ) at the top-left (the Direct3D convention),
		# so v is flipped exactly the way the Maya exporter does it ( v -> 1 - v )
		texcoords[:, 1] = 1.0 - texcoords[:, 1]
		return texcoords

	def _GetCornerColors( self, i_mesh, i_cornerVertexIndices, i_cornerLoopIndices ):
		colorAttribute = self._FindColorAttribute( i_mesh )
		if colorAttribute is None:
			# The Maya exporter uses white when a mesh has no color set
			return np.ones( ( len( i_cornerVertexIndices ), 4 ), dtype = np.float64 )
		colors = np.empty( len( colorAttribute.data ) * 4, dtype = np.float32 )
		# The sRGB values are the ones that are displayed in Blender,
		# and the engine writes vertex colors straight to a (non-sRGB) UNORM back buffer
		colorAttribute.data.foreach_get( "color_srgb", colors )
		colors = colors.reshape( -1, 4 ).astype( np.float64 )
		if colorAttribute.domain == 'POINT':
			colors = colors[i_cornerVertexIndices]
		elif colorAttribute.domain == 'CORNER':
			colors = colors[i_cornerLoopIndices]
		else:
			raise ExportError( f"The color attribute \"{colorAttribute.name}\" uses the unsupported domain {colorAttribute.domain}" )
		# MeshBuilder multiplies by 255 and casts to uint8_t,
		# so values outside of [0,1] (possible with float color attributes) must be clamped
		return np.clip( colors, 0.0, 1.0 )

	@staticmethod
	def _FindColorAttribute( i_mesh ):
		# Like Maya's "default" color set, prefer the active color attribute
		# and then the one that is used for rendering
		colorAttributes = i_mesh.color_attributes
		if len( colorAttributes ) == 0:
			return None
		for name in ( colorAttributes.active_color_name, colorAttributes.default_color_name ):
			if name and ( name in colorAttributes ):
				return colorAttributes[name]
		return colorAttributes[0]

	@staticmethod
	def _FillVertexAndIndexArrays( i_corners ):
		# Round to the precision that will be written,
		# and add 0 to turn any -0.0 into 0.0 so that it isn't written as "-0.000000"
		corners = i_corners.copy()
		corners[:, 1:] = np.round( corners[:, 1:], s_decimalPlaces ) + 0.0

		# De-duplicate the vertices.
		# The Maya exporter creates unique keys that include tangent and UV indices
		# even though they aren't written,
		# which results in identical vertices being written more than once.
		# This only considers the data that is actually written (position, normal, texture coordinates, and color),
		# so a vertex on a smooth surface is shared by its triangles
		# but a vertex on a hard edge or a UV seam is split into one vertex per normal/UV.
		# np.unique() also sorts the vertices, and because the material group is the first column
		# all of the vertices that use a single material end up contiguous.
		vertexArray, cornerToVertexIndex = np.unique( corners, axis = 0, return_inverse = True )
		cornerToVertexIndex = cornerToVertexIndex.reshape( -1 )

		triangles = cornerToVertexIndex.reshape( -1, s_vertexCountPerTriangle )
		triangleGroups = corners[0::s_vertexCountPerTriangle, s_column_materialGroup]
		# Remove triangles that collapsed into lines or points when identical vertices were merged
		# (they have no area and would never be drawn)
		isDegenerate = ( triangles[:, 0] == triangles[:, 1] ) | ( triangles[:, 1] == triangles[:, 2] ) | ( triangles[:, 0] == triangles[:, 2] )
		triangles = triangles[~isDegenerate]
		triangleGroups = triangleGroups[~isDegenerate]
		# Sort the triangles by material group
		# (so that a single draw call could work with a single contiguous block of index data)
		# and then by their vertices so that exported files are deterministic
		triangleOrder = np.lexsort( ( triangles[:, 2], triangles[:, 1], triangles[:, 0], triangleGroups ) )
		indexArray = triangles[triangleOrder].reshape( -1 )

		return vertexArray, indexArray

	@staticmethod
	def _ValidateLimits( i_vertexArray, i_indexArray ):
		vertexCount = len( i_vertexArray )
		indexCount = len( i_indexArray )
		triangleCount = indexCount // s_vertexCountPerTriangle
		if ( vertexCount > s_maxVertexCount ) or ( indexCount > s_maxIndexCount ):
			raise ExportError(
				f"The mesh is too big for the engine's 16-bit mesh format: it has {vertexCount:,} vertices and {triangleCount:,} triangles"
				f" but the limits are {s_maxVertexCount:,} vertices and {s_maxTriangleCount:,} triangles."
				" Reduce the geometry (e.g. lower Subdivision levels or add a Decimate modifier) or split it into multiple meshes" )

	@staticmethod
	def _WriteMeshToFile( i_path, i_vertexArray, i_indexArray ):
		# The engine expects the same data that the Maya exporter writes:
		#	* POSITION	-> converted from Blender's coordinate system in _CalculateEnginePositions()
		#	* NORMAL	-> converted the same way in _CalculateEngineCornerNormals()
		#	* TEXCOORD	-> u, 1 - v (converted in _GetCornerTexcoords())
		#	* COLOR		-> r, g, b, a in [0,1]
		#	* triangle index order	-> index_0, index_2, index_1
		# Note that "triangleCount" is actually the number of indices;
		# that is the key the Maya exporter writes and that MeshBuilder reads.
		lines = [
			"return",
			"{",
			f"\tvertexCount = {len( i_vertexArray )},",
			f"\ttriangleCount = {len( i_indexArray )},",
			"\tvertex =",
			"\t{",
		]
		for i, vertex in enumerate( i_vertexArray, start = 1 ):
			x, y, z = vertex[s_columns_position]
			nx, ny, nz = vertex[s_columns_normal]
			u, v = vertex[s_columns_texcoord]
			r, g, b, a = vertex[s_columns_color]
			lines.append( f"\t\t{{ name = \"vertex Position {i} \", x = {x:.6f}, y = {y:.6f}, z = {z:.6f},"
				f" nx = {nx:.6f}, ny = {ny:.6f}, nz = {nz:.6f},"
				f" colorAt = \"vertex {i} Color \", r = {r:.6f}, g = {g:.6f}, b = {b:.6f}, a = {a:.6f},"
				f" u = {u:.6f}, v = {v:.6f} }}, " )
		lines += [
			"\t},",
			"\tindex =",
			"\t{",
		]
		for triangle in i_indexArray.reshape( -1, s_vertexCountPerTriangle ):
			lines.append( f"\t\t{triangle[0]}, {triangle[2]}, {triangle[1]}, " )
		lines += [
			"\t}",
			"}",
			"",
		]
		try:
			# Text mode writes Windows line endings on Windows, the same as the Maya exporter's std::ofstream
			with open( i_path, "w", encoding = "ascii" ) as fout:
				fout.write( "\n".join( lines ) )
		except OSError as e:
			raise ExportError( f"Couldn't open {i_path} for writing: {e.strerror}" )
