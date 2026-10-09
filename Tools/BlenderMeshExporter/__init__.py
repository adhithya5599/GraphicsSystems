"""
	This is how Blender interacts with the add-on
	(the Blender counterpart of Tools/MayaMeshExporter/EntryPoint.cpp):
	register() and unregister() are the equivalents of initializePlugin() and uninitializePlugin(),
	and the export operator is the equivalent of the registered MPxFileTranslator
"""

# Blender reads this when the add-on is installed as a legacy add-on.
# When it is installed as an extension (Blender 4.2+) blender_manifest.toml is used instead.
bl_info = {
	"name": "AdhithyaNarayanan's EAE6320 Mesh Format",
	"author": "Adhithya Narayanan",
	"version": ( 1, 0, 0 ),
	"blender": ( 4, 2, 0 ),
	"location": "File > Export > AdhithyaNarayanan's EAE6320 Mesh (.blendermesh)",
	"description": "Exports meshes as EAE6320 Lua mesh files that MeshBuilder builds into binary meshes",
	"category": "Import-Export",
}

# Imports
#========

# Support reloading the add-on (F3 > Reload Scripts) while iterating on the exporter
if "bpy" in locals():
	import importlib
	importlib.reload( cBlenderMeshExporter )
else:
	from . import cBlenderMeshExporter

import bpy
from bpy.props import BoolProperty, FloatProperty, StringProperty
from bpy_extras.io_utils import ExportHelper

# Static Data
#============

# This will be displayed in Blender's File > Export menu
s_pluginName = "AdhithyaNarayanan's EAE6320 Mesh"
# Like the Maya exporter's "mayamesh", this is the default file extension of an exported mesh
s_defaultExtension = ".blendermesh"

# Operator
#=========

class EAE6320_OT_ExportMesh( bpy.types.Operator, ExportHelper ):
	"""Export meshes as an EAE6320 Lua mesh file"""

	bl_idname = "export_mesh.eae6320_blendermesh"
	bl_label = "Export EAE6320 Mesh"

	# ExportHelper uses this to add the extension and to filter the file browser
	filename_ext = s_defaultExtension
	filter_glob: StringProperty( default = "*" + s_defaultExtension, options = { 'HIDDEN' } )

	use_selection: BoolProperty(
		name = "Selection Only",
		description = "Export only the selected objects (like Maya's Export Selection)"
			" instead of every visible mesh in the scene (like Maya's Export All)",
		default = False,
	)
	global_scale: FloatProperty(
		name = "Scale",
		description = "Uniform scale applied to every exported position",
		default = 1.0, min = 0.0001, max = 10000.0,
	)

	def execute( self, i_context ):
		exporter = cBlenderMeshExporter.cBlenderMeshExporter( i_selectedOnly = self.use_selection, i_scale = self.global_scale )
		try:
			vertexCount, triangleCount = exporter.Export( i_context, self.filepath )
		except cBlenderMeshExporter.ExportError as e:
			self.report( { 'ERROR' }, str( e ) )
			return { 'CANCELLED' }
		self.report( { 'INFO' }, f"Exported {vertexCount} vertices and {triangleCount} triangles to {self.filepath}" )
		return { 'FINISHED' }

# Entry Point
#============

def MenuFunction_export( self, i_context ):
	self.layout.operator( EAE6320_OT_ExportMesh.bl_idname, text = f"{s_pluginName} ({s_defaultExtension})" )

def register():
	bpy.utils.register_class( EAE6320_OT_ExportMesh )
	bpy.types.TOPBAR_MT_file_export.append( MenuFunction_export )

def unregister():
	bpy.types.TOPBAR_MT_file_export.remove( MenuFunction_export )
	bpy.utils.unregister_class( EAE6320_OT_ExportMesh )

if __name__ == "__main__":
	register()
