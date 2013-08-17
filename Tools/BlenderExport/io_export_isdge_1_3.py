#################################
#Addon header
#################################
bl_info = {
	'name': 'iSDGE model format (.sgm)',
	'author': 'Nils Daumann',
	'blender': (2, 6, 5),
	'version': '1.3',
	'description': 'Exports an object in iSDGEs .sgm file format.',
	'category': 'Import-Export',
	'location': 'File -> Export -> iSDGE model (.sgm)'}

############################################################
#Structure of exported mesh files (.sgm)
############################################################
#magic number - uint32 - 352658064
#version - uint8 - 2
#number of materials - uint8
#material id - uint8
#	number of textures - uint8
#		filename length - uint16
#		filename - char*filename length
#
#number of meshs - uint8
#mesh id - uint8
#	used materials id - uint8
#	number of vertices - uint32
#	texcoord count - uint8
#	texdata count - uint8
#	has tangents - uint8 0 if not, 1 otherwise
#	has bones - uint8 0 if not, 1 otherwise
#	interleaved vertex data - float32
#		- position, normal, uvN, color, tangents, weights, bone indices
#
#	number of indices - uint32
#	index size - uint8, usually 2 or 4 bytes
#	indices - index size
#
#has animation - uint8 0 if not, 1 otherwise
#	animfilename length - uint16
#	animfilename - char*animfilename length

############################################################
#Structure of exported animation files (.sga)
############################################################
#magic number - uint32 - 383405658
#version - uint8 - 1
#skeleton name length - uint16
#skeleton name - char*skeleton name length
#
#number of bones - uint16
#	bone name length - uint16
#	bone name - char*bone name length
#	bone position xyz - float32*3
#	bone is a root (does not have a parent) - uint8 0 if not, 1 otherwise
#	bone number of children - uint16
#		child index within this list of bones - uint16
#
#number of animations - uint16
#	anim name length - uint16
#	anim name - char*anim name length
#	number of effected bones - uint16
#		skeleton bone id - uint16
#		number of frames for that bone - uint32
#			time - float
#			position xyz - 3*float
#			scale xyz - 3*float
#			rotation xyzw (quaternion) - 4*float

#################################
#Changelog
#################################
##V1 2011/10/22
#Features:
#-initial file format version 1
#-support for two uv sets
#-one texture per uv set, per vertex
#-different faces can have different textures, the object is automatically split into one mesh per texture
#-texture file ending can be chosen (textures have to be provided in the correct format for loading in iSDGE)
#-support for 4 vertex color channels (rgb and r from a second vertex color)
#-tangent generation and export
#-bone weights and per mesh indices with mapping to armature bones are exported
#-animation filename is exported
#-vertex data is exported interleaved with indices
#Known Problems:
#-every face has 3 vertices, which causes more vertex data then needed
##
#################################
##V1.1 2013/01/15
#-works with blender 2.6.5
#-fixed problem with vertices being exported for every face
#-number of indices is now 32bit, which solves problems with most larger meshs
#Known Problems:
#-crashes on exporting vertex colors
##
#################################
##V1.2 2013/03/05
#-added magic number
#-removed bone mapping
#-bone indices and weights are correctly exported
#-rest bones and animations can be exported
#-support for 32 bit indices
#Known Problems:
#-crashes on exporting vertex colors
#-scaled armatures and different origin of model and armature are problematic
##
#################################
##V1.3 2013/08/17
#-32bit indices are now fully supported
#Known Problems:
#-crashes on exporting vertex colors
#-scaled armatures and different origin of model and armature are problematic
#-polygons other than tris aren´t working
##


#################################
#ToDO
#################################
#-export of more than one texture per mesh for things like normalmaps
#-generation and export of shadow volume data, which otherwize is done on model loading in iSDGE
#-automatic converting of texture files to the desired format
#-export of more complex material setups
#(-support for morph animations)



#################################
#Includes
#################################
import os
import bpy
import struct
import math
import mathutils
from mathutils import Matrix

#Container classes for a better overview
class c_vertex(object):
	__slots__ = 'blendindex', 'position', 'uvs', 'color', 'normal', 'tangent', 'weights', 'bones'
	def __init__(self, blendindex, position = (0, 0, 0), uvs = [], color = None, normal = (0, 0, 0), tangent = (0, 0, 0, 0), weights = (0, 0, 0, 0), bones = (0, 0, 0, 0)):
		self.blendindex = blendindex	#index within the blender mesh
		self.position = position
		self.uvs = uvs
		self.color = color
		self.normal = normal
		self.tangent = tangent
		self.weights = weights
		self.bones = bones

class c_triangle(object):
	__slots__ = 'vertices', 'images', 'newindices'
	def __init__(self, vertices = [], images = [], newindices = None):
		self.vertices = vertices
		self.images = images
		self.newindices = newindices		#indices within the c_mesh
		
class c_mesh(object):
	__slots__ = 'triangles', 'images', 'vertices', 'indices'
	def __init__(self, images = [], tri = None):
		self.vertices = []
		self.triangles = []
		if tri != None:
			self.triangles.append(tri)
		self.images = images
		self.indices = []
	
	#doublicates face vertices with different uv coords and sets the new index
	def uvsplit(self):
		for tri in self.triangles:
			ind = []
			for trivert in tri.vertices:
				check = 0
				for i, vert in enumerate(self.vertices):
					if vert.position == trivert.position and vert.uvs == trivert.uvs:#vert == trivert:
						ind.append(i)
						check = 1
						break
				if check == 0:
					ind.append(len(self.vertices))
					self.vertices.append(trivert)
			
			self.indices.append(ind[0])
			self.indices.append(ind[1])
			self.indices.append(ind[2])
	
	#generates tangent for the given face
	def genfacetangent(self, vertex, neighbour_a, neighbour_b, bitangent0, bitangent1, bitangent2):
		posdir1 = (neighbour_a.position[0]-vertex.position[0], neighbour_a.position[1]-vertex.position[1], neighbour_a.position[2]-vertex.position[2])
		posdir2 = (neighbour_b.position[0]-vertex.position[0], neighbour_b.position[1]-vertex.position[1], neighbour_b.position[2]-vertex.position[2])
		uvdir1 = (neighbour_a.uvs[0][0]-vertex.uvs[0][0], neighbour_a.uvs[0][1]-vertex.uvs[0][1])
		uvdir2 = (neighbour_b.uvs[0][0]-vertex.uvs[0][0], neighbour_b.uvs[0][1]-vertex.uvs[0][1])
		
		r = (uvdir1[0]*uvdir2[1]-uvdir2[0]*uvdir1[1])
		if r != 0:
			r = 1.0/r
		else:
			r = 1.0
		tangent = ((uvdir2[1]*posdir1[0]-uvdir1[1]*posdir2[0])*r, (uvdir2[1]*posdir1[1]-uvdir1[1]*posdir2[1])*r, (uvdir2[1]*posdir1[2]-uvdir1[1]*posdir2[2])*r, 0.0)
		bitangent = ((uvdir1[0]*posdir2[0]-uvdir2[0]*posdir1[0])*r, (uvdir1[0]*posdir2[1]-uvdir2[0]*posdir1[1])*r, (uvdir1[0]*posdir2[2]-uvdir2[0]*posdir1[2])*r)
		
		vertex.tangent = (vertex.tangent[0]+tangent[0], vertex.tangent[1]+tangent[1], vertex.tangent[2]+tangent[2], 0.0)
		neighbour_a.tangent = (neighbour_a.tangent[0]+tangent[0], neighbour_a.tangent[1]+tangent[1], neighbour_a.tangent[2]+tangent[2], 0.0)
		neighbour_b.tangent = (neighbour_b.tangent[0]+tangent[0], neighbour_b.tangent[1]+tangent[1], neighbour_b.tangent[2]+tangent[2], 0.0)
		
		bitangent0 = (bitangent0[0]+bitangent[0], bitangent0[1]+bitangent[1], bitangent0[2]+bitangent[2])
		bitangent1 = (bitangent1[0]+bitangent[0], bitangent1[1]+bitangent[1], bitangent1[2]+bitangent[2])
		bitangent2 = (bitangent2[0]+bitangent[0], bitangent2[1]+bitangent[1], bitangent2[2]+bitangent[2])
		return bitangent0, bitangent1, bitangent2
	
	#generates tangents
	def gentangents(self):
		i = 0
		bitangents = []
		while i < len(self.vertices):
			bitangents.append((0, 0, 0))
			i += 1
		
		#Calculate the tangent of each vertex
		i = 0
		while i < len(self.indices):
			bitangents[self.indices[i]], bitangents[self.indices[i+2]], bitangents[self.indices[i+1]] = self.genfacetangent(self.vertices[self.indices[i]], self.vertices[self.indices[i+2]], self.vertices[self.indices[i+1]], bitangents[self.indices[i]], bitangents[self.indices[i+2]], bitangents[self.indices[i+1]])
			i += 3
		
		#Normalize all tangents
		i = 0
		while i < len(self.vertices):
			temptangent = (self.vertices[i].tangent[0]-self.vertices[i].normal[0]*self.vertices[i].normal[0]*self.vertices[i].tangent[0], self.vertices[i].tangent[1]-self.vertices[i].normal[1]*self.vertices[i].normal[1]*self.vertices[i].tangent[1], self.vertices[i].tangent[2]-self.vertices[i].normal[2]*self.vertices[i].normal[2]*self.vertices[i].tangent[2], 0.0)
			l = temptangent[0]*temptangent[0]+temptangent[1]*temptangent[1]+temptangent[2]*temptangent[2]
			l = math.sqrt(l)
			if l == 0: l = 1
			temptangent = (temptangent[0]/l, temptangent[1]/l, temptangent[2]/l, 0.0)
			self.vertices[i].tangent = temptangent
			i += 1
			
		#Calculate bitangent direction
		i = 0
		while i < len(self.vertices):
			temptangent = self.vertices[i].tangent
			bicross = (self.vertices[i].normal[1]*temptangent[2]-self.vertices[i].normal[2]*temptangent[1], self.vertices[i].normal[2]*temptangent[0]-self.vertices[i].normal[0]*temptangent[2], self.vertices[i].normal[0]*temptangent[1]-self.vertices[i].normal[1]*temptangent[0])
			bidot = bicross[0]*bitangents[i][0]+bicross[1]*bitangents[i][1]+bicross[2]*bitangents[i][2]
			if bidot < 0.0:
				bidot = -1.0
			else:
				bidot = 1.0
			self.vertices[i].tangent = (temptangent[0], temptangent[1], temptangent[2], bidot)
			i += 1


class c_object(object):
	__slots__ = 'meshs', 'hasbones', 'animname'
	#splits the blender object into triangle meshs with the same textures
	def __init__(self, objparent, obj, exptangents, expshadow, texextension):
		#check for bones
		self.hasbones = False
		ArmatureList = [Modifier for Modifier in objparent.modifiers if Modifier.type == "ARMATURE"]
		if ArmatureList:
			self.hasbones = True
#			self.animname = ArmatureList[0].object.data.name+".sga"
		if len(ArmatureList) > 1:
			print("only one armature per object supported: possible messed up bone assignements")

		obj.update(calc_tessface=True)
		triangles = []
		#generate vertices and triangles
		for i, face in enumerate(obj.polygons):
			images = []
			for tex in obj.uv_textures:
				if tex.data[i].image:
					imgpath = tex.data[i].image.filepath
					img = imgpath.split('/')
					img = img[len(img)-1]
					img = img.split('\\')
					img = img[len(img)-1]
					if texextension != 'keep':
						img = img[:-3]
						img += texextension
					images.append((tex.data[i].image.name, img))
				else:
					images.append(("default", "default."+texextension))
			
			verts = []
			for n, vertind in enumerate(face.vertices):
				uvs = []
				for tex in obj.tessface_uv_textures:
					uvs.append((round(tex.data[i].uv[n][0], 6), 1.0-round(tex.data[i].uv[n][1], 6)))
				color = None
				if len(obj.vertex_colors) > 0:
					alpha = 1.0
					if len(obj.vertex_colors) > 1:
						alpha = obj.vertex_colors[1].data[i].color.r
					color = (obj.vertex_colors[0].data[i].color.r, obj.vertex_colors[0].data[i].color.g, obj.vertex_colors[0].data[i].color.b, alpha)
				
				position = (obj.vertices[vertind].co.x, obj.vertices[vertind].co.y, obj.vertices[vertind].co.z)
				normal = (obj.vertices[vertind].normal.x, obj.vertices[vertind].normal.y, obj.vertices[vertind].normal.z)

				#get vertex weights and bone indices
				weights = [0, 0, 0, 0]
				bones = [0, 0, 0, 0]
				if self.hasbones == True:
					groups = sorted(obj.vertices[vertind].groups, key=lambda item: item.weight, reverse=True)

					sumweights = 0
					for g, group in enumerate(groups):
						if g > 3:
							break
						sumweights += group.weight

					for g, group in enumerate(groups):
						if g > 3:
							print("more then four groups assigned to vertex: loss of data")
							break
						weights[g] = group.weight/sumweights
						bones[g] = ArmatureList[0].object.data.bones.find(objparent.vertex_groups[group.group].name)
				
				verts.append(c_vertex(vertind, position, uvs, color, normal))
				verts[-1].weights = weights	#hacky as the above line should already do this, but for some reason does not...
				verts[-1].bones = bones
			
			if len(face.vertices) == 3:
				triangles.append(c_triangle(verts, images))
			else:
				tri1 = [verts[0], verts[1], verts[2]]
				tri2 = [verts[0], verts[2], verts[3]]
				triangles.append(c_triangle(tri1, images))
				triangles.append(c_triangle(tri2, images))
		
		#generate meshs
		self.meshs = []
		m = c_mesh(triangles[0].images)
		self.meshs.append(m)
		for tri in triangles:
			check = 0
			for mesh in self.meshs:
				if mesh.images == tri.images:
					mesh.triangles.append(tri)
					check = 1
					break
			if check == 0:
				m = c_mesh(tri.images, tri)
				self.meshs.append(m)
		
		for mesh in self.meshs:
			mesh.uvsplit()
		
		if exptangents == True:
			for mesh in self.meshs:
				mesh.gentangents()
	
	
	def write(self, filename, exptextures, exptangents, expshadow, expanimations):
		print("open or create file")
		file = open(filename, 'wb')

		file.write(struct.pack('<L', 352658064))
		print("write file format version number: 2")
		file.write(struct.pack('<B', 2))
		
		print("write materials")
		file.write(struct.pack('<B', len(self.meshs)))  #number of materials
		for i, mesh in enumerate(self.meshs):
			file.write(struct.pack('<B', i))			#material id
			texcount = len(mesh.images)
			if exptextures != True:
				texcount = 0
			file.write(struct.pack('<B', texcount)) #number of textures
			if texcount > 0:
				for img in mesh.images:
					file.write(struct.pack('<H', len(img[1])+1))
					file.write(struct.pack('<%is'%(len(img[1])+1), img[1].encode('ascii', 'replace')))
		
		print("write meshs")
		file.write(struct.pack('<B', len(self.meshs)))
		for i, mesh in enumerate(self.meshs):
			datachannels = 0
			if mesh.vertices[0].color != None:
				datachannels = 4
				
			file.write(struct.pack('<B', i))	#mesh id
			file.write(struct.pack('<B', i))	#material id
			file.write(struct.pack('<I', len(mesh.vertices)))   #vertexnum
			file.write(struct.pack('<B', len(mesh.images))) #texcoord count
			file.write(struct.pack('<B', datachannels)) #texdata count
			
			if exptangents == True:
				file.write(struct.pack('<B', 1)) #has tangents
			else:
				file.write(struct.pack('<B', 0)) #does not have tangents
				
			if self.hasbones == True:
				file.write(struct.pack('<B', 1)) #has bones
			else:
				file.write(struct.pack('<B', 0)) #does not have bones (number of bones is 0)
			
			print("write interleaved vertex data")
			
			for vertex in mesh.vertices:
				bindata = struct.pack('<fff', -vertex.position[0], vertex.position[2], vertex.position[1])
				file.write(bindata)
				
				bindata = struct.pack('<fff', -vertex.normal[0], vertex.normal[2], vertex.normal[1])
				file.write(bindata)

				set = 0
				while set < len(mesh.images):
					bindata = struct.pack('<ff', vertex.uvs[set][0], vertex.uvs[set][1])
					file.write(bindata)
					set += 1
				
				if mesh.vertices[0].color != None:
					bindata = struct.pack('<ffff', vertex.color[0], vertex.color[1], vertex.color[2], vertex.color[3])
					file.write(bindata)
				
				if exptangents == True:
					bindata = struct.pack('<ffff', -vertex.tangent[0], vertex.tangent[2], vertex.tangent[1], vertex.tangent[3])
					file.write(bindata)
					
				if self.hasbones == True:
					bindata = struct.pack('<ffff', vertex.weights[0], vertex.weights[1], vertex.weights[2], vertex.weights[3])
					file.write(bindata)
					bindata = struct.pack('<ffff', vertex.bones[0], vertex.bones[1], vertex.bones[2], vertex.bones[3])
					file.write(bindata)
					
			print("finished writing interleaved vertex data")
			
			print("write indices")
			file.write(struct.pack('<I', len(mesh.indices)))
			maxval = max(mesh.indices)
			if maxval > 65535:
				file.write(struct.pack('<B', 4))
			else:
				file.write(struct.pack('<B', 2))

			for ind in mesh.indices:
				if maxval > 65535:
					bindata = struct.pack('<I', ind)
				else:
					bindata = struct.pack('<H', ind)
				file.write(bindata)
				
		print("write animation reference")
		if self.hasbones == True and expanimations == True:
			file.write(struct.pack('<B', 1)) #has animations
			file.write(struct.pack('<H', len(self.animname)+1))
			file.write(struct.pack('<%is'%(len(self.animname)+1), self.animname.encode('ascii', 'replace')))
		else:
			file.write(struct.pack('<B', 0)) #does not have animations
	
		file.close()


class c_boneframe(object):
	__slots__ = 'time', 'position', 'scale', 'rotation'
	def __init__(self, time, position, scale, rotation):
		self.time = time
		self.position = position
		self.scale = scale
		self.rotation = rotation

class c_animation(object):
	__slots__ = 'name', 'length', 'frames'
	def __init__(self, name):
		self.name = name
		self.frames = {}

class c_bone(object):
	__slots__ = 'name', 'children', 'position', 'isroot'
	def __init__(self, name, position, isroot):
		self.name = name
		self.children = []
		self.position = position
		self.isroot = isroot;


class c_armature(object):
	__slots__ = 'name', 'bones', 'animations'
	def __init__(self, objparent):
		self.name = 'empty'
		self.bones = []
		self.animations = []

		ArmatureList = [Modifier for Modifier in objparent.modifiers if Modifier.type == "ARMATURE"]
		if ArmatureList:
			self.name = ArmatureList[0].object.data.name
		else:
			return
		armature = ArmatureList[0].object
		#get skeleton data
		for bone in armature.data.bones:
			b = c_bone(bone.name, armature.matrix_world*bone.head_local, False if bone.parent else True)	#add bone
			for child in bone.children:
				b.children.append(armature.data.bones.find(child.name))
			self.bones.append(b)

		vertspacemat = mathutils.Matrix.Identity(4)
		vertspacemat[0][0] = -1
		vertspacemat[1][1] = 0
		vertspacemat[1][2] = 1
		vertspacemat[2][2] = 0
		vertspacemat[2][1] = 1

		#get animation frames
		frame_current = bpy.context.scene.frame_current
		for action in bpy.data.actions:
			armature.animation_data.action = action
			anim = c_animation(action.name)
			for frame in range(int(action.frame_range[0]), int(action.frame_range[1])):
				bpy.context.scene.frame_set(frame)
				for i, bone in enumerate(armature.pose.bones):
					index = armature.data.bones.find(bone.bone.name)
					
					trans = Matrix.Translation(vertspacemat*bone.bone.head_local)
					itrans = Matrix.Translation(vertspacemat*(-bone.bone.head_local))
					if bone.parent:
						mat_final = itrans*vertspacemat*bone.parent.bone.matrix_local*(vertspacemat*bone.parent.matrix).inverted()*vertspacemat*bone.matrix*(vertspacemat*bone.bone.matrix_local).inverted()*trans
					else:
						mat_final = itrans*vertspacemat*bone.matrix*(vertspacemat*bone.bone.matrix_local).inverted()*trans
					pos, rot, scal = mat_final.decompose()

					rot = (rot[1], rot[2], rot[3], rot[0])
					boneframe = c_boneframe(frame-action.frame_range[0], pos, scal, rot)
					if frame == action.frame_range[0]:
						anim.frames[index] = []
					(anim.frames[index]).append(boneframe)

			self.animations.append(anim)
		bpy.context.scene.frame_set(frame_current)


	def write(self, filename):
		"""
		file = open(filename, 'w')
		file.write(self.name+"\n")
		file.write("\nskeleton:")
		for bone in self.bones:
			file.write(bone.name+"\n")
			file.write("pos: %f %f %f\n" % (-bone.position[0], bone.position[2], bone.position[1]))
			file.write("child count: %i children:" % len(bone.children))
			for child in bone.children:
				file.write("%i " % child)
			file.write("\n")

		file.write("\nanimations:\n")
		file.write("count: %i\n" % len(self.animations))
		for anim in self.animations:
			file.write("name: "+anim.name+"\n")
			file.write("bone count: %i\n" % len(anim.frames))
			for i, boneframes in anim.frames.items():
				file.write("skeleton bone id: %i\n" % i)
				file.write("bone frame count: %i\n" % len(boneframes))
				for frame in boneframes:
					file.write("time: %f\n" % frame.time)
					file.write("pos: %f %f %f\n" % (-frame.position[0], frame.position[2], frame.position[1]))
					file.write("scale: %f %f %f\n" % (frame.scale[0], frame.scale[2], frame.scale[1]))
					file.write("rot: %f %f %f %f\n" % (frame.rotation[0], frame.rotation[1], frame.rotation[2], frame.rotation[3]))
					#print(frame.rotation)
		file.close()
		"""
		file = open(filename, 'wb')
		file.write(struct.pack('<L', 383405658))	#magic number
		file.write(struct.pack('<B', 1))	#file type version (1)
		file.write(struct.pack('<H', len(self.name)+1))
		file.write(struct.pack('<%is'%(len(self.name)+1), self.name.encode('ascii', 'replace')))
		file.write(struct.pack('<H', len(self.bones)))
		for bone in self.bones:
			file.write(struct.pack('<H', len(bone.name)+1))
			file.write(struct.pack('<%is'%(len(bone.name)+1), bone.name.encode('ascii', 'replace')))
			bindata = struct.pack('<fff', -bone.position[0], bone.position[2], bone.position[1])
			file.write(bindata)
			file.write(struct.pack('<B', 1 if bone.isroot else 0))
			file.write(struct.pack('<H', len(bone.children)))
			for child in bone.children:
				file.write(struct.pack('<H', child))

		file.write(struct.pack('<H', len(self.animations)))
		for anim in self.animations:
			file.write(struct.pack('<H', len(anim.name)+1))
			file.write(struct.pack('<%is'%(len(anim.name)+1), anim.name.encode('ascii', 'replace')))
			file.write(struct.pack('<H', len(anim.frames)))
			for i, boneframes in anim.frames.items():
				file.write(struct.pack('<H', i))
				file.write(struct.pack('<L', len(boneframes)))
				for frame in boneframes:
					file.write(struct.pack('<f', frame.time))
					bindata = struct.pack('<fff', frame.position[0], frame.position[1], frame.position[2])
					file.write(bindata)
					bindata = struct.pack('<fff', frame.scale[0], frame.scale[1], frame.scale[2])
					file.write(bindata)
					bindata = struct.pack('<ffff', frame.rotation[0], frame.rotation[1], frame.rotation[2], frame.rotation[3])
					file.write(bindata)
		file.close()


#################################
#Interface and stuff
#################################
from bpy.props import *
class ExportSGM(bpy.types.Operator):
	'''Export to iSDGE model file format (.sgm)'''
	bl_idname = "export.isdge_sgm"
	bl_label = 'Export iSDGE model'

	filepath = StringProperty(name="File Path", description="Filepath used for exporting the iSDGE model file", maxlen= 1024, default= "")
	#filepath = StringProperty(subtype='FILE_PATH')
	check_existing = BoolProperty(name="Check Existing", description="Check and warn on overwriting existing files", default=True, options={'HIDDEN'})
	
	#properties
	exptextures = BoolProperty(name="Export textures", description="Reference external image files to be used by the model.", default=True)
	texextension = EnumProperty(
			name="Texture extension",
			items=(('png', ".png", ""),
				   ('pvr', ".pvr", ""),
				   ('keep', "keep current", ""),
				   ),
			default='png',
			)
	exptangents = BoolProperty(name="Export tangents", description="Generate tangents for the model to use for example tangent space normal mapping.", default=False)
	expanimations = BoolProperty(name="Export animations", description="Export animation data in an additional file and reference it in the object.", default=True)
	expshadow = False #BoolProperty(name="Export shadow", description="Prepare the mesh for shadow volume rendering to speed up loading.", default=False)
	
	def execute(self, context):
		bpy.ops.object.mode_set(mode='OBJECT')
		print("start exporting .sgm file")
		obj = c_object(context.object, context.object.data, self.exptangents, self.expshadow, self.texextension)
		obj.animname = os.path.basename(self.properties.filepath[0:len(self.properties.filepath)-4]+".sga")
		obj.write(self.properties.filepath, self.exptextures, self.exptangents, self.expshadow, self.expanimations)
		print("finished exporting .sgm file")
		if obj.hasbones and self.expanimations:
			print("start exporting .sga file")
			arm = c_armature(context.object)
			arm.write(self.properties.filepath[0:len(self.properties.filepath)-4]+".sga")
			print("finished exporting .sga file")
		return {'FINISHED'}

	def invoke(self, context, event):
		wm = context.window_manager
		wm.fileselect_add(self)
		return {'RUNNING_MODAL'}


def menu_func(self, context):
	default_path = os.path.splitext(bpy.data.filepath)[0] + ".sgm"
	self.layout.operator(ExportSGM.bl_idname, text="iSDGE model (.sgm)").filepath = default_path

def register():
	bpy.utils.register_module(__name__)
	bpy.types.INFO_MT_file_export.append(menu_func)

def unregister():
	bpy.utils.unregister_module(__name__)
	bpy.types.INFO_MT_file_export.remove(menu_func)

if __name__ == "__main__":
	register()