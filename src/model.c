#ifdef _WIN32
#undef WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <float.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glext.h> // for mingw

#include "types.h"
#include "mtx.h"
#include "endianess.h"
#include "model.h"
#include "animation.h"
#include "io.h"
#include "heap.h"
#include "os.h"

#ifdef _DEBUG
#	ifdef WIN32
#		define BREAKPOINT()				{ __asm { int 3 }; }
#	else
#		define BREAKPOINT()
#	endif
#else
#	define BREAKPOINT()
#endif

void fatal(const char* message)
{
	BREAKPOINT()
	printf(message);
	exit(0);
}

#define print_once(message, ...)			{ static bool done_it = false; if(!done_it) { printf(message, __VA_ARGS__); done_it = true; } }

enum CULL_MODE {
	DOUBLE_SIDED = 0,
	BACK_SIDE = 1,
	FRONT_SIDE = 2
};

enum GXPolygonMode {
	GX_POLYGONMODE_MODULATE = 0,
	GX_POLYGONMODE_DECAL = 1,
	GX_POLYGONMODE_TOON = 2,
	GX_POLYGONMODE_SHADOW = 3
};

enum REPEAT_MODE {
	CLAMP = 0,
	REPEAT = 1,
	MIRROR = 2
};

enum RENDER_MODE {
	NORMAL = 0,
	DECAL = 1,
	TRANSLUCENT = 2,
	// viewer only, not stored in a file
	ALPHA_TEST = 3
};

typedef struct {
	u8		name[64];
	u8		light;
	u8		culling;
	u8		alpha;
	u8		wireframe;
	u16		palid;
	u16		texid;
	u8		x_repeat;
	u8		y_repeat;
	Color3		diffuse;
	Color3		ambient;
	Color3		specular;
	u8		field_53;
	u32		polygon_mode;
	u8		render_mode;
	u8		anim_flags;
	u16		field_5A;
	u32		texcoord_transform_mode;
	u16		texcoord_animation_id;
	u16		field_62;
	u32		matrix_id;
	u32		scale_s;
	u32		scale_t;
	u16		rot_z;
	u16		field_72;
	u32		translate_s;
	u32		translate_t;
	u16		material_animation_id;
	u16		field_7E;
	u8		packed_repeat_mode;
	u8		field_81;
	u16		field_82;
} Material;

typedef struct {
	u32		start_ofs;
	u32		size;
	s32		bounds[3][2];
} Dlist;

typedef struct {
	u8		name[64];
	u16		parent;
	u16		child;
	u16		next;
	u16		field_46;
	u32		enabled;
	u16		mesh_count;
	u16		mesh_id;
	VecFx32		scale;
	fx16		angle_x;
	fx16		angle_y;
	fx16		angle_z;
	u16		field_62;
	VecFx32		pos;
	u32		field_70;
	VecFx32		vec1;
	VecFx32		vec2;
	u8		type;
	u8		field_8D;
	u16		field_8E;
	MtxFx43		node_transform;
	u32		field_C0;
	u32		field_C4;
	u32		field_C8;
	u32		field_CC;
	u32		field_D0;
	u32		field_D4;
	u32		field_D8;
	u32		field_DC;
	u32		field_E0;
	u32		field_E4;
	u32		field_E8;
	u32		field_EC;
} Node;

typedef struct {
	u16		matid;
	u16		dlistid;
} Mesh;

typedef struct {
	u16		format;
	u16		width;
	u16		height;
	u16		pad;
	u32		image_ofs;
	u32		imagesize;
	u32		dunno1;
	u32		dunno2;
	u32		vram_offset;
	u32		opaque;
	u32		some_value;
	u8		packed_size;
	u8		native_texture_format;
	u16		texture_obj_ref;
} Texture;

typedef struct {
	u32		entries_ofs;
	u32		count;
	u32		dunno1;
	u32		some_reference;
} Palette;

typedef struct {
	u32		modelview_mtx_shamt;
	s32		scale;
	u32		unk3;
	u32		unk4;
	u32		materials;
	u32		dlists;
	u32		nodes;
	u16		node_anim_count;
	u8		flags;
	u8		field_1F;
	u32		some_node_id;
	u32		meshes;
	u16		num_textures;
	u16		field_2A;
	u32		textures;
	u16		palette_count;
	u16		field_32;
	u32		palettes;
	u32		some_anim_counts;
	u32		unk8;
	u32		node_initial_pos;
	u32		node_pos;
	u16		num_materials;
	u16		num_nodes;
	u32		texture_matrices;
	u32		node_animation;
	u32		texcoord_animations;
	u32		material_animations;
	u32		texture_animations;
	u16		num_meshes;
	u16		num_matrices;
} __attribute__((__packed__)) HEADER;

#ifdef _WIN32
PFNGLCREATESHADERPROC		glCreateShader;	
PFNGLSHADERSOURCEPROC		glShaderSource;
PFNGLCOMPILESHADERPROC		glCompileShader;
PFNGLGETSHADERIVPROC		glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC	glGetShaderInfoLog;
PFNGLDELETESHADERPROC		glDeleteShader;
PFNGLCREATEPROGRAMPROC		glCreateProgram;
PFNGLATTACHSHADERPROC		glAttachShader;
PFNGLDETACHSHADERPROC		glDetachShader;
PFNGLLINKPROGRAMPROC		glLinkProgram;
PFNGLGETPROGRAMIVPROC		glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC	glGetProgramInfoLog;
PFNGLDELETEPROGRAMPROC		glDeleteProgram;
PFNGLUSEPROGRAMPROC		glUseProgram;
PFNGLGETUNIFORMLOCATIONPROC	glGetUniformLocation;
PFNGLUNIFORM1IPROC		glUniform1i;
PFNGLUNIFORM4FVPROC		glUniform4fv;
PFNGLMULTTRANSPOSEMATRIXFPROC	glMultTransposeMatrixf;

static void load_extensions(void)
{
	glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
	glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
	glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
	glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
	glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
	glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
	glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
	glDetachShader = (PFNGLDETACHSHADERPROC)wglGetProcAddress("glDetachShader");
	glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
	glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
	glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
	glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
	glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
	glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
	glUniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
	glUniform4fv = (PFNGLUNIFORM4FVPROC)wglGetProcAddress("glUniform4fv");
	glMultTransposeMatrixf = (PFNGLMULTTRANSPOSEMATRIXFPROC)wglGetProcAddress("glMultTransposeMatrixf");
}
#endif

const char* vertex_shader = "\
uniform bool is_billboard; \n\
uniform bool use_light; \n\
uniform vec4 light1vec; \n\
uniform vec4 light1col; \n\
uniform vec4 light2vec; \n\
uniform vec4 light2col; \n\
uniform vec4 diffuse; \n\
uniform vec4 ambient; \n\
uniform vec4 specular; \n\
\n\
varying vec2 texcoord; \n\
varying vec4 color; \n\
\n\
void main() \n\
{ \n\
	if(is_billboard) { \n\
		gl_Position = gl_ProjectionMatrix * (gl_ModelViewMatrix * vec4(0.0, 0.0, 0.0, 1.0) + vec4(gl_Vertex.x, gl_Vertex.y, gl_Vertex.z, 0.0)); \n\
	} else { \n\
		gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; \n\
	} \n\
	if(use_light) { \n\
		// light 1 \n\
		float fixed_diffuse1 = dot(light1vec.xyz, gl_Normal); \n\
		vec3 neghalf1 = -(light1vec.xyz / 2.0); \n\
		float d1 = dot(neghalf1, gl_Normal); \n\
		float fixed_shininess1 = d1 > 0.0 ? 2.0 * d1 * d1 : 0.0; \n\
		vec4 spec1 = specular * light1col * fixed_shininess1; \n\
		vec4 diff1 = diffuse * light1col * fixed_diffuse1; \n\
		vec4 amb1 = ambient * light1col; \n\
		vec4 col1 = spec1 + diff1 + amb1; \n\
		// light 2 \n\
		float fixed_diffuse2 = dot(light2vec.xyz, gl_Normal); \n\
		vec3 neghalf2 = -(light2vec.xyz / 2.0); \n\
		float d2 = dot(neghalf2, gl_Normal); \n\
		float fixed_shininess2 = d2 > 0.0 ? 2.0 * d2 * d2 : 0.0; \n\
		vec4 spec2 = specular * light2col * fixed_shininess2; \n\
		vec4 diff2 = diffuse * light2col * fixed_diffuse2; \n\
		vec4 amb2 = ambient * light2col; \n\
		vec4 col2 = spec2 + diff2 + amb2; \n\
		color = gl_Color + col1 + col2; \n\
	} else { \n\
		color = gl_Color; \n\
	} \n\
	texcoord = vec2(gl_TextureMatrix[0] * gl_MultiTexCoord0); \n\
}";
const char* fragment_shader = "\
uniform bool is_billboard; \n\
uniform bool use_texture; \n\
uniform sampler2D tex; \n\
varying vec2 texcoord; \n\
varying vec4 color; \n\
\n\
void main() \n\
{ \n\
	if(use_texture) { \n\
		vec4 texcolor = texture2D(tex, texcoord); \n\
		gl_FragColor = color * texcolor; \n\
	} else  { \n\
		gl_FragColor = color; \n\
	} \n\
}";

static GLuint shader;
static GLuint is_billboard;
static GLuint use_light;
static GLuint use_texture;
static GLuint light1vec;
static GLuint light2vec;
static GLuint light1col;
static GLuint light2col;
static GLuint diffuse;
static GLuint ambient;
static GLuint specular;

static float l1v[4];
static float l1c[4];
static float l2v[4];
static float l2c[4];

void CModel_setLights(float l1vec[4], float l1col[4], float l2vec[4], float l2col[4])
{
	memcpy(l1v, l1vec, sizeof(float[4]));
	memcpy(l1c, l1col, sizeof(float[4]));
	memcpy(l2v, l2vec, sizeof(float[4]));
	memcpy(l2c, l2col, sizeof(float[4]));
}

void CModel_init(void)
{
#ifdef _WIN32
	load_extensions();
#endif

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vertex_shader, 0);
	glCompileShader(vs);

	GLint compiled = 0;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &compiled);
	if(compiled == GL_FALSE) {
		GLint len = 0;
		glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &len);

		char* log = (char*)malloc(len);
		glGetShaderInfoLog(vs, len, &len, log);
		glDeleteShader(vs);

		fatal(log);
	}

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fragment_shader, 0);
	glCompileShader(fs);

	glGetShaderiv(vs, GL_COMPILE_STATUS, &compiled);
	if(compiled == GL_FALSE) {
		GLint len = 0;
		glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &len);

		char* log = (char*)malloc(len);
		glGetShaderInfoLog(fs, len, &len, log);
		glDeleteShader(fs);

		fatal(log);
	}

	shader = glCreateProgram();

	glAttachShader(shader, vs);
	glAttachShader(shader, fs);
	glLinkProgram(shader);

	GLint linked = 0;
	glGetProgramiv(shader, GL_LINK_STATUS, (int*)&linked);
	if(linked == GL_FALSE) {
		GLint len = 0;
		glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &len);

		char* log = (char*)malloc(len);
		glGetProgramInfoLog(shader, len, &len, log);

		glDeleteProgram(shader);
		glDeleteShader(vs);
		glDeleteShader(fs);

		fatal(log);
	}

	glDetachShader(shader, vs);
	glDetachShader(shader, fs);

	is_billboard = glGetUniformLocation(shader, "is_billboard");
	use_light = glGetUniformLocation(shader, "use_light");
	use_texture = glGetUniformLocation(shader, "use_texture");
	light1vec = glGetUniformLocation(shader, "light1vec");
	light1col = glGetUniformLocation(shader, "light1col");
	light2vec = glGetUniformLocation(shader, "light2vec");
	light2col = glGetUniformLocation(shader, "light2col");
	diffuse = glGetUniformLocation(shader, "diffuse");
	ambient = glGetUniformLocation(shader, "ambient");
	specular = glGetUniformLocation(shader, "specular");
}

static void update_bounds(CModel* scene, float vtx_state[3])
{
	if(vtx_state[0] < scene->min_x) {
		scene->min_x = vtx_state[0];
	} else if(vtx_state[0] > scene->max_x) {
		scene->max_x = vtx_state[0];
	}
	if(vtx_state[1] < scene->min_y) {
		scene->min_y = vtx_state[1];
	} else if(vtx_state[1] > scene->max_y) {
		scene->max_y = vtx_state[1];
	}
	if(vtx_state[2] < scene->min_z) {
		scene->min_z = vtx_state[0];
	} else if(vtx_state[2] > scene->max_z) {
		scene->max_z = vtx_state[2];
	}
}

static void do_reg(u32 reg, u32** data_pp, float vtx_state[3], CModel* scene)
{
	u32* data = *data_pp;

	switch(reg) {
		//NOP
		case 0x400: {
		}
		break;

		//MTX_RESTORE
		case 0x450: {
			u32 idx = *(data++);
		}
		break;

		//COLOR
		case 0x480: {
			u32 rgb = *(data++);
			u32 r = (rgb >>  0) & 0x1F;
			u32 g = (rgb >>  5) & 0x1F;
			u32 b = (rgb >> 10) & 0x1F;
			glColor3f(((float)r) / 31.0f, ((float)g) / 31.0f, ((float)b) / 31.0f);
		}
		break;

		//NORMAL
		case 0x484: {
			u32 xyz = *(data++);
			s32 x = (xyz >>  0) & 0x3FF;			if(x & 0x200)			x |= 0xFFFFFC00;
			s32 y = (xyz >> 10) & 0x3FF;			if(y & 0x200)			y |= 0xFFFFFC00;
			s32 z = (xyz >> 20) & 0x3FF;			if(z & 0x200)			z |= 0xFFFFFC00;
			glNormal3f(((float)x) / 512.0f, ((float)y) / 512.0f, ((float)z) / 512.0f);
		}
		break;

		//TEXCOORD
		case 0x488: {
			u32 st = *(data++);
			s32 s = (st >>  0) & 0xFFFF;			if(s & 0x8000)		s |= 0xFFFF0000;
			s32 t = (st >> 16) & 0xFFFF;			if(t & 0x8000)		t |= 0xFFFF0000;
			glTexCoord2f(((float)s) / 16.0f, ((float)t) / 16.0f);
		}
		break;

		//VTX_16
		case 0x48C: {
			u32 xy = *(data++);
			s32 x = (xy >>  0) & 0xFFFF;			if(x & 0x8000)		x |= 0xFFFF0000;
			s32 y = (xy >> 16) & 0xFFFF;			if(y & 0x8000)		y |= 0xFFFF0000;
			s32 z = (*(data++)) & 0xFFFF;			if(z & 0x8000)		z |= 0xFFFF0000;

			vtx_state[0] = ((float)x) / 4096.0f;
			vtx_state[1] = ((float)y) / 4096.0f;
			vtx_state[2] = ((float)z) / 4096.0f;
			glVertex3fv(vtx_state);

			update_bounds(scene, vtx_state);
		}
		break;

		//VTX_10
		case 0x490: {
			u32 xyz = *(data++);
			s32 x = (xyz >>  0) & 0x3FF;			if(x & 0x200)		x |= 0xFFFFFC00;
			s32 y = (xyz >> 10) & 0x3FF;			if(y & 0x200)		y |= 0xFFFFFC00;
			s32 z = (xyz >> 20) & 0x3FF;			if(z & 0x200)		z |= 0xFFFFFC00;
			vtx_state[0] = ((float)x) / 64.0f;
			vtx_state[1] = ((float)y) / 64.0f;
			vtx_state[2] = ((float)z) / 64.0f;
			glVertex3fv(vtx_state);

			update_bounds(scene, vtx_state);
		}
		break;

		//VTX_XY
		case 0x494: {
			u32 xy = *(data++);
			s32 x = (xy >>  0) & 0xFFFF;			if(x & 0x8000)		x |= 0xFFFF0000;
			s32 y = (xy >> 16) & 0xFFFF;			if(y & 0x8000)		y |= 0xFFFF0000;
			vtx_state[0] = ((float)x) / 4096.0f;
			vtx_state[1] = ((float)y) / 4096.0f;
			glVertex3fv(vtx_state);

			update_bounds(scene, vtx_state);
		}
		break;

		//VTX_XZ
		case 0x498: {
			u32 xz = *(data++);
			s32 x = (xz >>  0) & 0xFFFF;			if(x & 0x8000)		x |= 0xFFFF0000;
			s32 z = (xz >> 16) & 0xFFFF;			if(z & 0x8000)		z |= 0xFFFF0000;
			vtx_state[0] = ((float)x) / 4096.0f;
			vtx_state[2] = ((float)z) / 4096.0f;
			glVertex3fv(vtx_state);

			update_bounds(scene, vtx_state);
		}
		break;

		//VTX_YZ
		case 0x49C: {
			u32 yz = *(data++);
			s32 y = (yz >>  0) & 0xFFFF;			if(y & 0x8000)		y |= 0xFFFF0000;
			s32 z = (yz >> 16) & 0xFFFF;			if(z & 0x8000)		z |= 0xFFFF0000;
			vtx_state[1] = ((float)y) / 4096.0f;
			vtx_state[2] = ((float)z) / 4096.0f;
			glVertex3fv(vtx_state);

			update_bounds(scene, vtx_state);
		}
		break;

		//VTX_DIFF
		case 0x4A0: {
			u32 xyz = *(data++);
			s32 x = (xyz >>  0) & 0x3FF;			if(x & 0x200)		x |= 0xFFFFFC00;
			s32 y = (xyz >> 10) & 0x3FF;			if(y & 0x200)		y |= 0xFFFFFC00;
			s32 z = (xyz >> 20) & 0x3FF;			if(z & 0x200)		z |= 0xFFFFFC00;
			vtx_state[0] += ((float)x) / 4096.0f;
			vtx_state[1] += ((float)y) / 4096.0f;
			vtx_state[2] += ((float)z) / 4096.0f;
			glVertex3fv(vtx_state);

			update_bounds(scene, vtx_state);
		}
		break;

		//DIF_AMB
		case 0x4C0: {
			u32 val = *(data++);
		}
		break;

		//BEGIN_VTXS
		case 0x500: {
			u32 type = *(data++);
			switch( type )
			{
				case 0:		glBegin(GL_TRIANGLES);			break;
				case 1:		glBegin(GL_QUADS);			break;
				case 2:		glBegin(GL_TRIANGLE_STRIP);		break;
				case 3:		glBegin(GL_QUAD_STRIP);			break;
				default:	fatal("Bogus geom type\n");		break;
			}
		}
		break;

		//END_VTXS
		case 0x504: {
			glEnd();
		}
		break;

		//Hmm?
		default: {
			printf("unhandled write to register 0x%x\n", 0x4000000 + reg);
			fatal("Unhandled reg write\n");
		}
		break;
	}

	*data_pp = data;
}


/*

	First word is 4 byte-sized register indexes (where index is quadrupled and added to 0x04000400 to yield an address)
	These are processed least significant byte first
	Register index 0 means NOP
	Following this is the data to be written to each register, which is variable depending on the register being written

*/

static void do_dlist(u32* data, u32 len, CModel* scene)
{
	u32* end = data + len / 4;

	float vtx_state[3] = { 0.0f, 0.0f, 0.0f };

	while(data < end) {
		u32 regs = *(data++);

		u32 c;
		for(c = 0; c < 4; c++,regs >>= 8) {
			u32 reg = ((regs & 0xFF) << 2) + 0x400;

			do_reg(reg, &data, vtx_state, scene);
		}
	}
}

static void build_meshes(CModel* scene, Mesh* meshes, Dlist* dlists, u8* scenedata, unsigned int scenesize)
{
	scene->meshes = (MESH*) malloc(scene->num_meshes * sizeof(MESH));
	if(!scene->meshes)
		fatal("not enough memory");

	Mesh* end = (Mesh*) (scenedata + scenesize);
	Mesh* mesh;
	MESH* m;

	scene->min_x = FLT_MAX;
	scene->min_y = FLT_MAX;
	scene->min_z = FLT_MAX;
	scene->max_x = -FLT_MAX;
	scene->max_y = -FLT_MAX;
	scene->max_z = -FLT_MAX;

	for(mesh = meshes, m = scene->meshes; mesh < end; mesh++, m++) {
		Dlist* dlist = &dlists[mesh->dlistid];
		u32* data = (u32*) (scenedata + dlist->start_ofs);
		m->matid = mesh->matid;
		m->dlistid = mesh->dlistid;
		if(scene->dlists[m->dlistid] != -1)
			continue;
		scene->dlists[m->dlistid] = glGenLists(1);
		glNewList(scene->dlists[m->dlistid], GL_COMPILE);
		do_dlist(data, dlist->size, scene);
		glEndList();
	}
}

/*

	Texture formats:

		0 = 2bit palettised
		1 = 4bit palettised
		2 = 8bit palettised
		4 = 8bit greyscale
		5 = 16bit RGBA

	There may be more, but these are the only ones used by Metroid

	Palette entries are 16bit RGBA

*/
static void make_textures(CModel* scn, Material* materials, unsigned int num_materials, Texture* textures, unsigned int num_textures, Palette* palettes, u8* data, u32 datasize)
{
	u32 m;
	for(m = 0; m < num_materials; m++) {
		Material* mat = &materials[m];
		scn->materials[m].render_mode = mat->render_mode;
		if(mat->texid == 0xFFFF)
			continue;
		if(mat->texid >= num_textures) {
			printf("invalid texture id %04X for material %d\n", mat->texid, m);
			continue;
		}

		Texture* tex = &textures[mat->texid];
		Palette* pal = &palettes[mat->palid];

		u8* texels = (u8*) (data + tex->image_ofs);
		u16* paxels = (u16*) (data + pal->entries_ofs);

		if(tex->image_ofs >= datasize) {
			printf("invalid texel offset for texture %d in material %d\n", mat->texid, m);
			continue;
		}
		if(pal->entries_ofs >= datasize) {
			printf("invalid paxel offset for palette %d in material %d\n", mat->palid, m);
			continue;
		}

		u32 num_pixels = (u32)tex->width * (u32)tex->height;

		u32 texsize = num_pixels;
		if(tex->format == 0)
			texsize /= 4;
		else if(tex->format == 1)
			texsize /= 2;
		else if(tex->format == 5)
			texsize *= 2;

		if((tex->image_ofs + texsize) > datasize) {
			printf("invalid texture size\n");
			continue;
		}
		//if((pal->entries_ofs + pal->count * 2) > datasize) {
		//	printf("invalid palette size (%d, %d)\n", pal->entries_ofs, pal->count);
		//	continue;
		//}

		u32* image = (u32*) malloc(sizeof(u32) * num_pixels);
		if(!image)
			fatal("not enough memory");

		float alpha = mat->alpha / 31.0;

		if(tex->format == 0) {				// 2bit palettised
			u32 p;
			for(p = 0; p < num_pixels; p++) {
				u32 index = texels[p / 4];
				index = (index >> ((p % 4) * 2)) & 0x3;
				u16 col = get16bit_LE((u8*)&paxels[index]);
				u32 r = ((col >>  0) & 0x1F) << 3;
				u32 g = ((col >>  5) & 0x1F) << 3;
				u32 b = ((col >> 10) & 0x1F) << 3;
				u32 a = (tex->opaque ? 0xFF : (index == 0 ? 0x00 : 0xFF)) * alpha;
				image[p] = (r << 0) | (g << 8) | (b << 16) | (a << 24);
			}
		} else if(tex->format == 1) {			// 4bit palettised
			u32 p;
			for(p = 0; p < num_pixels; p++) {
				u32 index = texels[p / 2];
				index = (index >> ((p % 2) * 4)) & 0xF;
				u16 col = get16bit_LE((u8*)&paxels[index]);
				u32 r = ((col >>  0) & 0x1F) << 3;
				u32 g = ((col >>  5) & 0x1F) << 3;
				u32 b = ((col >> 10) & 0x1F) << 3;
				u32 a = (tex->opaque ? 0xFF : (index == 0 ? 0x00 : 0xFF)) * alpha;
				image[p] = (r << 0) | (g << 8) | (b << 16) | (a << 24);
			}
		} else if(tex->format == 2) {			// 8bit palettised
			u32 p;
			for(p = 0; p < num_pixels; p++) {
				u32 index = texels[p];
				u16 col = get16bit_LE((u8*)&paxels[index]);
				u32 r = ((col >>  0) & 0x1F) << 3;
				u32 g = ((col >>  5) & 0x1F) << 3;
				u32 b = ((col >> 10) & 0x1F) << 3;
				u32 a = (tex->opaque ? 0xFF : (index == 0 ? 0x00 : 0xFF)) * alpha;
				image[p] = (r << 0) | (g << 8) | (b << 16) | (a << 24);
			}
		} else if(tex->format == 4) {			// A5I3
			u32 p;
			for(p = 0; p < num_pixels; p++) {
				u8 entry = texels[p];
				u8 i = (entry & 0x07);
				u16 col = get16bit_LE((u8*)&paxels[i]);
				u32 r = ((col >>  0) & 0x1F) << 3;
				u32 g = ((col >>  5) & 0x1F) << 3;
				u32 b = ((col >> 10) & 0x1F) << 3;
				u32 a = ((entry >> 3) / 31.0 * 255.0) * alpha;
				image[p] = (r << 0) | (g << 8) | (b << 16) | (a << 24);
			}
		} else if(tex->format == 5) {			// 16it RGB
			u32 p;
			for(p = 0; p < num_pixels; p++) {
				u16 col = (u16)texels[p * 2 + 0] | (((u16)texels[p * 2 + 1]) << 8);
				u32 r = ((col >> 0) & 0x1F) << 3;
				u32 g = ((col >> 5) & 0x1F) << 3;
				u32 b = ((col >> 10) & 0x1F) << 3;
				u32 a = ((col & 0x8000) ? 0x00 : 0xFF) * alpha;
				image[p] = (r << 0) | (g << 8) | (b << 16) | (a << 24);
			}
		} else if(tex->format == 6) {			// A3I5
			u32 p;
			for(p = 0; p < num_pixels; p++) {
				u8 entry = texels[p];
				u32 i = entry & 0x1F;
				u16 col = get16bit_LE((u8*)&paxels[i]);
				u32 r = ((col >>  0) & 0x1F) << 3;
				u32 g = ((col >>  5) & 0x1F) << 3;
				u32 b = ((col >> 10) & 0x1F) << 3;
				u32 a = ((entry >> 5) / 7.0 * 255.0) * alpha;
				image[p] = (r << 0) | (g << 8) | (b << 16) | (a << 24);
			}
		} else {
			BREAKPOINT();
			print_once("Unhandled texture-format: %d\n", tex->format);
			memset(image, 0x7F, num_pixels * 4);
		}

		u32 p;
		switch(mat->polygon_mode) {
			case GX_POLYGONMODE_MODULATE:
				break;
			case GX_POLYGONMODE_DECAL:
				for(p = 0; p < num_pixels; p++) {
					u32 col = image[p];
					u32 r = (col >>  0) & 0xFF;
					u32 g = (col >>  8) & 0xFF;
					u32 b = (col >> 16) & 0xFF;
					u32 a = 0xFF * alpha;
					image[p] = (r << 0) | (g << 8) | (b << 16) | (a << 24);
				}
				break;
			case GX_POLYGONMODE_TOON:
				break;
			case GX_POLYGONMODE_SHADOW:
				break;
			default:
				printf("unknown alpha mode %d\n", mat->alpha);
				break;
		}

		int translucent = 0;
		for(p = 0; p < num_pixels; p++) {
			u32 col = image[p];
			u32 a = (col >> 24) & 0xFF;
			if(a != 0xFF) {
				translucent = 1;
				break;
			}
		}
		if(mat->alpha < 31) {
			scn->materials[m].render_mode = TRANSLUCENT;
		}
		if(scn->materials[m].render_mode) {
			if(!translucent) {
				printf("%d [%s]: strange, this should be opaque (alpha: %d, fmt: %d, opaque: %d)\n", m, mat->name, mat->alpha, tex->format, tex->opaque);
				scn->materials[m].render_mode = NORMAL;
			}
		} else if(translucent) {
			// there are translucent pixels, but the material is not marked as translucent
			// -> we assume that alpha test is used
			printf("%d [%s]: strange, this should be translucent (alpha: %d, fmt: %d, opaque: %d)\n", m, mat->name, mat->alpha, tex->format, tex->opaque);
			scn->materials[m].render_mode = ALPHA_TEST;
		}

		if(scn->materials[m].render_mode == TRANSLUCENT) {
			if(alpha == 1.0f) {
				switch(tex->format) {
					case 0:
					case 1:
					case 2:
					case 5:
						printf("%d [%s]: using alpha test\n", m, mat->name);
						scn->materials[m].render_mode = ALPHA_TEST;
				}
			}
		}

		glGenTextures(1, &scn->materials[m].tex);
		glBindTexture(GL_TEXTURE_2D, scn->materials[m].tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->width, tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)image);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		switch(mat->x_repeat) {
			case CLAMP:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				break;
			case REPEAT:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				break;
			case MIRROR:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
				break;
			default:
				printf("unknown repeat mode %d\n", mat->x_repeat);
		}
		switch(mat->y_repeat) {
			case CLAMP:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				break;
			case REPEAT:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				break;
			case MIRROR:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
				break;
			default:
				printf("unknown repeat mode %d\n", mat->x_repeat);
		}
		glBindTexture(GL_TEXTURE_2D, 0);
		free(image);
	}
}

static char* get_room_node_name(NODE* nodes, unsigned int node_cnt)
{
	char* name = "rmMain";
	if(node_cnt > 0) {
		NODE* node = nodes;
		int i = 0;
		while(node->name[0] != 'r' || node->name[1] != 'm') {
			node++;
			i++;
			if(i >= node_cnt) {
				return name;
			}
		}
		name = node->name;
	}
	return name;
}

static int get_node_child(const char* name, CModel* scene)
{
	unsigned int i;
	if(scene->num_nodes <= 0)
		return -1;
	for(i = 0; i < scene->num_nodes; i++) {
		NODE* node = &scene->nodes[i];
		if(!strcmp(node->name, name))
			return node->child;
	}
	return -1;
}

void CModel_filter_nodes(CModel* scene, int layer_mask)
{
	unsigned int i;
	for(i = 0; i < scene->num_nodes; i++) {
		NODE* node = &scene->nodes[i];
		int flags = 0;
		if(strlen(node->name)) {
			unsigned int p;
			int keep = 0;
			for(p = 0; p < strlen(node->name); p += 4) {
				char* ch1 = &node->name[p];
				if(*ch1 != '_')
					break;
				if(*(u16*)ch1 == *(u16*)"_s") {
					int nr = node->name[p + 3] - '0' + 10 * (node->name[p + 2] - '0');
					if(nr)
						flags = flags & 0xC03F | ((((u32)flags << 18 >> 24) | (1 << nr)) << 6);
				}
				u32 tag = *(u32*) ch1;
				if(tag == *(u32*)"_ml0")
					flags |= LAYER_ML0;
				if(tag == *(u32*)"_ml1")
					flags |= LAYER_ML1;
				if(tag == *(u32*)"_mpu")
					flags |= LAYER_MPU;
				if(tag == *(u32*)"_ctf")
					flags |= LAYER_CTF;
			}

			if(!p || flags & layer_mask)
				keep = 1;
			if(!keep) {
				printf("filtering node '%s'\n", node->name);
				node->enabled = 0;
			} else if(node->name[0] == '_') {
				printf("not filtering node '%s'\n", node->name);
			}
		}
	}
}

void load_model(CModel** model, const char* filename, int flags)
{
	u8* data = NULL;
	int size = 0;

	if(flags & USE_ARCHIVE) {
		size = LoadFileFromArchive((void**)&data, filename);
	} else {
		size = LoadFile((void**)&data, filename);
	}

	*model = CModel_load(data, size, data, size, 0xFFFFFFFF);

	free_to_heap(data);
}

void load_room_model(CModel** model, const char* filename, const char* txtrfilename, int flags, int layer_mask)
{
	u8* data = NULL;
	int size = 0;
	u8* txtr = NULL;
	int txtrsz = 0;

	if(flags & USE_ARCHIVE) {
		size = LoadFileFromArchive((void**)&data, filename);
	} else {
		size = LoadFile((void**)&data, filename);
	}

	if(flags & USE_EXTERNAL_TXTR) {
		txtrsz = LoadFile((void**)&txtr, txtrfilename);
	} else {
		txtr = data;
		txtrsz = size;
	}

	*model = CModel_load(data, size, txtr, txtrsz, layer_mask);

	if(flags & USE_EXTERNAL_TXTR) {
		free_to_heap(txtr);
	}

	free_to_heap(data);
}

CModel* CModel_load(u8* scenedata, unsigned int scenesize, u8* texturedata, unsigned int texturesize, int layer_mask)
{
	unsigned int i;

	CModel* scene = (CModel*) malloc(sizeof(CModel));
	if(!scene)
		fatal("not enough memory");

	scene->animation = NULL;
	scene->texcoord_animations = NULL;

	HEADER* rawheader = (HEADER*) scenedata;

	scene->scale		= get32bit_LE((u8*)&rawheader->scale) / 4096.0f;
	scene->scale		*= 1 << get32bit_LE((u8*)&rawheader->modelview_mtx_shamt);

	Material* materials	= (Material*)	((uintptr_t)scenedata + (uintptr_t)get32bit_LE((u8*)&rawheader->materials));
	Dlist* dlists		= (Dlist*)	((uintptr_t)scenedata + (uintptr_t)get32bit_LE((u8*)&rawheader->dlists));
	Node* nodes		= (Node*)	((uintptr_t)scenedata + (uintptr_t)get32bit_LE((u8*)&rawheader->nodes));
	Mesh* meshes		= (Mesh*)	((uintptr_t)scenedata + (uintptr_t)get32bit_LE((u8*)&rawheader->meshes));
	Texture* textures	= (Texture*)	((uintptr_t)scenedata + (uintptr_t)get32bit_LE((u8*)&rawheader->textures));
	Palette* palettes	= (Palette*)	((uintptr_t)scenedata + (uintptr_t)get32bit_LE((u8*)&rawheader->palettes));
	VecFx32* node_pos	= (VecFx32*)	((uintptr_t)scenedata + (uintptr_t)get32bit_LE((u8*)&rawheader->node_pos));
	VecFx32* node_init_pos	= (VecFx32*)	((uintptr_t)scenedata + (uintptr_t)get32bit_LE((u8*)&rawheader->node_initial_pos));

	scene->num_textures	= get16bit_LE((u8*)&rawheader->num_textures);
	scene->num_materials	= get16bit_LE((u8*)&rawheader->num_materials);
	scene->num_nodes	= get16bit_LE((u8*)&rawheader->num_nodes);

	scene->materials = (MATERIAL*) malloc(scene->num_materials * sizeof(MATERIAL));
	scene->textures = (TEXTURE*) malloc(scene->num_textures * sizeof(TEXTURE));
	scene->nodes = (NODE*) malloc(scene->num_nodes * sizeof(NODE));

	scene->node_pos = NULL;
	if(rawheader->node_pos) {
		scene->node_pos = (Vec3*) malloc(scene->num_nodes * sizeof(NODE));
		for(i = 0; i < scene->num_nodes; i++) {
			scene->node_pos[i].x = FX_FX32_TO_F32((fx32) get32bit_LE((u8*)&node_pos[i].x));
			scene->node_pos[i].y = FX_FX32_TO_F32((fx32) get32bit_LE((u8*)&node_pos[i].y));
			scene->node_pos[i].z = FX_FX32_TO_F32((fx32) get32bit_LE((u8*)&node_pos[i].z));
		}
	}

	scene->node_initial_pos = NULL;
	if(rawheader->node_initial_pos) {
		scene->node_initial_pos = (Vec3*) malloc(scene->num_nodes * sizeof(NODE));
		for(i = 0; i < scene->num_nodes; i++) {
			scene->node_initial_pos[i].x = FX_FX32_TO_F32((fx32) get32bit_LE((u8*)&node_init_pos[i].x));
			scene->node_initial_pos[i].y = FX_FX32_TO_F32((fx32) get32bit_LE((u8*)&node_init_pos[i].y));
			scene->node_initial_pos[i].z = FX_FX32_TO_F32((fx32) get32bit_LE((u8*)&node_init_pos[i].z));
		}
	}

	for(i = 0; i < scene->num_nodes; i++) {
		NODE* node = &scene->nodes[i];
		Node* raw = &nodes[i];

		strncpy(node->name, raw->name, 64);
		node->parent = (s16) get16bit_LE((u8*)&raw->parent);
		node->child = (s16) get16bit_LE((u8*)&raw->child);
		node->next = (s16) get16bit_LE((u8*)&raw->next);
		node->mesh_count = get16bit_LE((u8*)&raw->mesh_count);
		node->mesh_id = (s16) get16bit_LE((u8*)&raw->mesh_id);
		node->enabled = get32bit_LE((u8*)&raw->enabled);
		node->type = raw->type;
		node->scale.x = FX_FX32_TO_F32((fx32) get32bit_LE((u8*)&raw->scale.x));
		node->scale.y = FX_FX32_TO_F32((fx32) get32bit_LE((u8*)&raw->scale.y));
		node->scale.z = FX_FX32_TO_F32((fx32) get32bit_LE((u8*)&raw->scale.z));
		node->angle.x = get16bit_LE((u8*)&raw->angle_x) / 65536.0 * 2.0 * M_PI;
		node->angle.y = get16bit_LE((u8*)&raw->angle_x) / 65536.0 * 2.0 * M_PI;
		node->angle.z = get16bit_LE((u8*)&raw->angle_x) / 65536.0 * 2.0 * M_PI;
		node->pos.x = FX_FX32_TO_F32((fx32) get32bit_LE((u8*)&raw->pos.x));
		node->pos.y = FX_FX32_TO_F32((fx32) get32bit_LE((u8*)&raw->pos.y));
		node->pos.z = FX_FX32_TO_F32((fx32) get32bit_LE((u8*)&raw->pos.z));
	}

	scene->room_node_name = get_room_node_name(scene->nodes, scene->num_nodes);
	scene->room_node_id = get_node_child(scene->room_node_name, scene);

	CModel_filter_nodes(scene, layer_mask);

	printf("scale: %f\n", scene->scale);
	printf("room node: '%s' (%d)\n", scene->room_node_name, scene->room_node_id);

	if(!scene->materials || !scene->textures)
		fatal("not enough memory");

	for(i = 0; i < scene->num_materials; i++) {
		Material* m = &materials[i];
		m->palid = get16bit_LE((u8*)&m->palid);
		m->texid = get16bit_LE((u8*)&m->texid);

		MATERIAL* mat = &scene->materials[i];
		strcpy(mat->name, m->name);
		mat->texid = m->texid;
		mat->light = m->light;
		mat->culling = m->culling;
		mat->alpha = m->alpha;
		mat->x_repeat = m->x_repeat;
		mat->y_repeat = m->y_repeat;
		mat->polygon_mode = get32bit_LE((u8*)&m->polygon_mode);
		mat->scale_s = FX_FX32_TO_F32(get32bit_LE((u8*)&m->scale_s));
		mat->scale_t = FX_FX32_TO_F32(get32bit_LE((u8*)&m->scale_t));
		mat->translate_s = FX_FX32_TO_F32(get32bit_LE((u8*)&m->translate_s));
		mat->translate_t = FX_FX32_TO_F32(get32bit_LE((u8*)&m->translate_t));
		mat->diffuse = m->diffuse;
		mat->ambient = m->ambient;
		mat->specular = m->specular;
		// mat->diffuse.r = (u8) (m->diffuse.r / (float)0x1F * 255.0);
		// mat->diffuse.g = (u8) (m->diffuse.g / (float)0x1F * 255.0);
		// mat->diffuse.b = (u8) (m->diffuse.b / (float)0x1F * 255.0);
		// mat->ambient.r = (u8) (m->ambient.r / (float)0x1F * 255.0);
		// mat->ambient.g = (u8) (m->ambient.g / (float)0x1F * 255.0);
		// mat->ambient.b = (u8) (m->ambient.b / (float)0x1F * 255.0);
		// mat->specular.r = (u8) (m->specular.r / (float)0x1F * 255.0);
		// mat->specular.g = (u8) (m->specular.g / (float)0x1F * 255.0);
		// mat->specular.b = (u8) (m->specular.b / (float)0x1F * 255.0);

		unsigned int p = m->palid;
		if(p == 0xFFFF)
			continue;
		palettes[p].count = get32bit_LE((u8*)&palettes[p].count);
		palettes[p].entries_ofs = get32bit_LE((u8*)&palettes[p].entries_ofs);
	}
	for(i = 0; i < scene->num_textures; i++) {
		TEXTURE* tex = &scene->textures[i];
		Texture* t = &textures[i];
		t->format = get16bit_LE((u8*)&t->format);
		t->width = get16bit_LE((u8*)&t->width);
		t->height = get16bit_LE((u8*)&t->height);
		t->image_ofs = get32bit_LE((u8*)&t->image_ofs);
		t->imagesize = get32bit_LE((u8*)&t->imagesize);
		tex->width = t->width;
		tex->height = t->height;
		tex->opaque = t->opaque;
	}

	unsigned int meshcount = 0;
	Mesh* end = (Mesh*) (scenedata + scenesize);
	Mesh* mesh;
	scene->num_dlists = 0;
	for(mesh = meshes; mesh < end; mesh++) {
		meshcount++;
		mesh->matid = get16bit_LE((u8*)&mesh->matid);
		mesh->dlistid = get16bit_LE((u8*)&mesh->dlistid);
		if(mesh->dlistid >= scene->num_dlists)
			scene->num_dlists = mesh->dlistid + 1;
		Dlist* dlist = &dlists[mesh->dlistid];
		dlist->start_ofs = get32bit_LE((u8*)&dlist->start_ofs);
		dlist->size = get32bit_LE((u8*)&dlist->size);
		u32* data = (u32*) (scenedata + dlist->start_ofs);
		u32* end = data + dlist->size / 4;
		while(data < end) {
			*data = get32bit_LE((u8*)data);
			data++;
		}
	}
	scene->num_meshes = meshcount;

	scene->dlists = (int*) malloc(scene->num_dlists * sizeof(int));
	for(i = 0; i < scene->num_dlists; i++)
		scene->dlists[i] = -1;

	make_textures(scene, materials, scene->num_materials, textures, scene->num_textures, palettes, texturedata, texturesize);

	i = 0;
	scene->meshes = (MESH*) malloc(sizeof(MESH));
	if(!scene->meshes)
		fatal("not enough memory");
	build_meshes(scene, meshes, dlists, scenedata, scenesize);

	scene->apply_transform = 0;
	CModel_compute_node_matrices(scene, 0);

	return scene;
}

void scale_rotate_translate(Mtx44* mtx, float sx, float sy, float sz, float ax, float ay, float az, float x, float y, float z)
{
	float sin_ax = sinf(ax);
	float sin_ay = sinf(ay);
	float sin_az = sinf(az);
	float cos_ax = cosf(ax);
	float cos_ay = cosf(ay);
	float cos_az = cosf(az);

	float v18 = cos_ax * cos_az;
	float v19 = cos_ax * sin_az;
	float v20 = cos_ax * cos_ay;

	float v22 = sin_ax * sin_ay;

	float v17 = v19 * sin_ay;

	mtx->_00 = sx * cos_ay * cos_az;
	mtx->_01 = sx * cos_ay * sin_az;
	mtx->_02 = sx * -sin_ay;

	mtx->_10 = sy * ((v22 * cos_az) - v19);
	mtx->_11 = sy * ((v22 * sin_az) + v18);
	mtx->_12 = sy * sin_ax * cos_ay;

	mtx->_20 = sz * (v18 * sin_ay + sin_ax * sin_az);
	mtx->_21 = sz * ((v17 + (v19 * sin_ay)) - (sin_ax * cos_az));
	mtx->_22 = sz * v20;

	mtx->_30 = x;
	mtx->_31 = y;
	mtx->_32 = z;

	mtx->_03 = 0;
	mtx->_13 = 0;
	mtx->_23 = 0;
	mtx->_33 = 1;
}

void CModel_compute_node_matrices(CModel* model, int start_idx)
{
	int idx;
	Mtx44 transform;
	NODE* node;

	if(!model->nodes || start_idx == -1)
		return;

	for(idx = start_idx; idx != -1; idx = node->next) {
		node = &model->nodes[idx];

		scale_rotate_translate(&transform, node->scale.x, node->scale.y, node->scale.z,
				node->angle.x, node->angle.y, node->angle.z,
				node->pos.x, node->pos.y, node->pos.z);

		if(node->parent == -1)
			MTX44Copy(&transform, &node->node_transform);
		else
			MTX44Concat(&transform, &model->nodes[node->parent].node_transform, &node->node_transform);

		if(node->child != -1)
			CModel_compute_node_matrices(model, node->child);

		if(model->node_pos) {
			MTX44Identity(&transform);
			transform._30 = -model->node_initial_pos[idx].x;
			transform._31 = -model->node_initial_pos[idx].y;
			transform._32 = -model->node_initial_pos[idx].z;
			MTX44Concat(&transform, &node->node_transform, &node->node_transform);
		}
	}
}

CModel* CModel_load_file(const char* model, const char* textures, int layer_mask)
{
	FILE* file = fopen(model, "rb");
	if(!file) {
		printf("%s not found\n", model);
		exit(1);
	}

	fseek(file, 0, SEEK_END);
	u32 scenesize = ftell(file);
	fseek(file, 0, SEEK_SET);
	u8* content = (u8*) malloc(scenesize);
	fread(content, 1, scenesize, file);
	fclose(file);

	u32 texturesize = scenesize;
	u8* texturedata = content;

	if(textures != NULL) {
		file = fopen(textures, "rb");
		if(!file) {
			printf("%s not found\n", textures);
			exit(1);
		}
		fseek(file, 0, SEEK_END);
		texturesize = ftell(file);
		fseek(file, 0, SEEK_SET);
		texturedata = (u8*) malloc(texturesize);
		fread(texturedata, 1, texturesize, file);
		fclose(file);
	}

	CModel* scene = CModel_load(content, scenesize, texturedata, texturesize, layer_mask);
	if(texturedata != content)
		free(texturedata);
	free(content);
	return scene;
}

void CModel_free(CModel* scene)
{
	unsigned int i;
	for(i = 0; i < scene->num_materials; i++) {
		glDeleteTextures(1, &scene->materials[i].tex);
	}
	for(i = 0; i < scene->num_dlists; i++) {
		glDeleteLists(1, scene->dlists[i]);
	}
	free(scene->textures);
	free(scene->materials);
	free(scene->meshes);
	free(scene->dlists);
	free(scene->nodes);
	free(scene->node_pos);
	free(scene->node_initial_pos);
	free(scene);
}

extern bool lighting;

void CModel_render_mesh(CModel* scene, int mesh_id, int render_notex)
{
	if(mesh_id >= scene->num_meshes) {
		printf("trying to render mesh %d, but scene only has %d meshes\n", mesh_id, scene->num_meshes);
		return;
	}

	MESH* mesh = &scene->meshes[mesh_id];
	MATERIAL* material = &scene->materials[mesh->matid];
	TEXTURE* texture = &scene->textures[material->texid];

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	if(material->texid != 0xFFFF) {
		glBindTexture(GL_TEXTURE_2D, material->tex);

		//Convert pixel coords to normalised STs
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();

		if(scene->texcoord_animations && material->texcoord_anim_id != -1) {
			glScalef(1.0f / texture->width, 1.0f / texture->height, 1.0f);
			process_texcoord_animation(scene->texcoord_animations, material->texcoord_anim_id, texture->width, texture->height);
		} else {
			glTranslatef(material->translate_s, material->translate_t, 0.0f);
			glScalef(material->scale_s, material->scale_t, 1.0f);
			glScalef(1.0f / texture->width, 1.0f / texture->height, 1.0f);
		}

		glUniform1i(use_texture, glIsEnabled(GL_TEXTURE_2D));
	} else {
		glBindTexture(GL_TEXTURE_2D, 0);
		glUniform1i(use_texture, 0);
		if(!render_notex)
			return;
	}

	if(lighting && material->light) {
		float amb[4] = { (float)material->ambient.r / 255.0f, (float)material->ambient.g / 255.0f, (float)material->ambient.b / 255.0f, 1.0f };
		float diff[4] = { material->diffuse.r / 255.0f, material->diffuse.g / 255.0f, material->diffuse.b / 255.0f, 1.0f };
		float spec[4] = { material->specular.r / 255.0f, material->specular.g / 255.0f, material->specular.b / 255.0f, 1.0f };
		//if(amb[0] != 0)
		//	printf("amb: [%d] %f,%f,%f,%f\n", material->ambient.r, amb[0], amb[1], amb[2], amb[3]);
		glEnable(GL_LIGHTING);
		glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, diff);
		glUniform4fv(ambient, 1, amb);
		glUniform4fv(diffuse, 1, diff);
		glUniform4fv(specular, 1, spec);
		glUniform1i(use_light, 1);
	} else {
		glUniform1i(use_light, 0);
	}

	switch(material->culling) {
		case DOUBLE_SIDED:
			glDisable(GL_CULL_FACE);
			break;
		case BACK_SIDE:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			break;
		case FRONT_SIDE:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			break;
	}

	glCallList(scene->dlists[mesh->dlistid]);

	if(lighting) {
		glDisable(GL_LIGHTING);
	}
}

static void CModel_billboard(NODE* node)
{
	glUniform1i(is_billboard, node->type == 1);
	if(node->type) {
		// this is a billboard
	}
}

static void CModel_update_uniforms(void)
{
	glUniform4fv(light1vec, 1, l1v);
	glUniform4fv(light1col, 1, l1c);
	glUniform4fv(light2vec, 1, l2v);
	glUniform4fv(light2col, 1, l2c);
}

void CModel_render_all(CModel* scene)
{
	// GLboolean cull;
	unsigned int i, j;

	glUseProgram(shader);

	CModel_update_uniforms();

	// pass 1: opaque
	glDepthMask(GL_TRUE);
	for(i = 0; i < scene->num_nodes; i++) {
		NODE* node = &scene->nodes[i];
		if(node->mesh_count) {
			int mesh_id = node->mesh_id / 2;

			if(scene->apply_transform) {
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glMultTransposeMatrixf(node->node_transform.a);
			}

			for(j = 0; j < node->mesh_count; j++) {
				int id = mesh_id + j;
				MESH* mesh = &scene->meshes[id];
				MATERIAL* material = &scene->materials[mesh->matid];

				if(material->render_mode != NORMAL)
					continue;

				CModel_billboard(node);
				CModel_render_mesh(scene, id, 1);
			}

			if(scene->apply_transform) {
				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();
			}
		}
	}

	// pass 2: translucent
	// glGetBooleanv(GL_CULL_FACE, &cull);
	// glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for(i = 0; i < scene->num_nodes; i++) {
		NODE* node = &scene->nodes[i];
		if(node->mesh_count) {
			int mesh_id = node->mesh_id / 2;

			if(scene->apply_transform) {
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glMultTransposeMatrixf(node->node_transform.a);
			}

			for(j = 0; j < node->mesh_count; j++) {
				int id = mesh_id + j;
				MESH* mesh = &scene->meshes[id];
				MATERIAL* material = &scene->materials[mesh->matid];

				if(material->render_mode == NORMAL)
					continue;

				CModel_billboard(node);
				CModel_render_mesh(scene, id, 1);
			}

			if(scene->apply_transform) {
				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();
			}
		}
	}
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	// if(cull) {
	// 	glEnable(GL_CULL_FACE);
	// }

	glUseProgram(0);
}

void CModel_render_node_tree(CModel* scene, int node_idx)
{
	unsigned int i;
	int start_node = node_idx;
	while(node_idx != -1) {
		NODE* node = &scene->nodes[node_idx];
		if(node->mesh_count > 0 && node->enabled == 1) {
			int mesh_id = node->mesh_id / 2;

			if(scene->apply_transform) {
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glMultTransposeMatrixf(node->node_transform.a);
			}

			for(i = 0; i < node->mesh_count; i++)
				CModel_render_mesh(scene, mesh_id + i, 0);

			if(scene->apply_transform) {
				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();
			}
		}
		if(node->child != -1)
			CModel_render_node_tree(scene, node->child);
		node_idx = node->next;
	}
}

void CModel_render_all_nodes(CModel* scene)
{
	unsigned int i, j;

	glUseProgram(shader);

	CModel_update_uniforms();

	// pass 1: opaque
	glDepthMask(GL_TRUE);
	for(i = 0; i < scene->num_nodes; i++) {
		NODE* node = &scene->nodes[i];
		if(node->mesh_count > 0 && node->enabled == 1) {
			int mesh_id = node->mesh_id / 2;

			if(scene->apply_transform) {
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glMultTransposeMatrixf(node->node_transform.a);
			}

			for(j = 0; j < node->mesh_count; j++) {
				int id = mesh_id + j;
				MESH* mesh = &scene->meshes[id];
				MATERIAL* material = &scene->materials[mesh->matid];

				if(material->render_mode != NORMAL)
					continue;

				CModel_billboard(node);
				CModel_render_mesh(scene, id, 0);
			}

			if(scene->apply_transform) {
				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();
			}
		}
	}

	// pass 2: decal
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-1, -1);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for(i = 0; i < scene->num_nodes; i++) {
		NODE* node = &scene->nodes[i];
		if(node->mesh_count > 0 && node->enabled == 1) {
			int mesh_id = node->mesh_id / 2;

			if(scene->apply_transform) {
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glMultTransposeMatrixf(node->node_transform.a);
			}

			for(j = 0; j < node->mesh_count; j++) {
				int id = mesh_id + j;
				MESH* mesh = &scene->meshes[id];
				MATERIAL* material = &scene->materials[mesh->matid];

				if(material->render_mode != DECAL)
					continue;

				CModel_billboard(node);
				CModel_render_mesh(scene, id, 0);
			}

			if(scene->apply_transform) {
				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();
			}
		}
	}

	// pass 3: translucent with alpha test
	glEnable(GL_ALPHA_TEST);
	glDepthMask(GL_TRUE);
	glAlphaFunc(GL_GEQUAL, 0.5f);
	for(i = 0; i < scene->num_nodes; i++) {
		NODE* node = &scene->nodes[i];
		if(node->mesh_count > 0 && node->enabled == 1) {
			int mesh_id = node->mesh_id / 2;

			if(scene->apply_transform) {
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glMultTransposeMatrixf(node->node_transform.a);
			}

			for(j = 0; j < node->mesh_count; j++) {
				int id = mesh_id + j;
				MESH* mesh = &scene->meshes[id];
				MATERIAL* material = &scene->materials[mesh->matid];
				TEXTURE* texture = &scene->textures[material->texid];

				if(material->render_mode != ALPHA_TEST)
					continue;

				CModel_billboard(node);
				CModel_render_mesh(scene, id, 0);
			}

			if(scene->apply_transform) {
				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();
			}
		}
	}
	glDisable(GL_ALPHA_TEST);

	// pass 4: translucent
	glDepthMask(GL_FALSE);
	for(i = 0; i < scene->num_nodes; i++) {
		NODE* node = &scene->nodes[i];
		if(node->mesh_count > 0 && node->enabled == 1) {
			int mesh_id = node->mesh_id / 2;

			if(scene->apply_transform) {
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glMultTransposeMatrixf(node->node_transform.a);
			}

			for(j = 0; j < node->mesh_count; j++) {
				int id = mesh_id + j;
				MESH* mesh = &scene->meshes[id];
				MATERIAL* material = &scene->materials[mesh->matid];
				TEXTURE* texture = &scene->textures[material->texid];

				if(material->render_mode != TRANSLUCENT)
					continue;

				CModel_billboard(node);
				CModel_render_mesh(scene, id, 0);
			}

			if(scene->apply_transform) {
				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();
			}
		}
	}

	glPolygonOffset(0, 0);
	glDisable(GL_POLYGON_OFFSET_FILL);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	glUseProgram(0);
}

void CModel_render(CModel* scene)
{
	if(scene->room_node_id == -1) {
		CModel_render_all(scene);
	} else {
		CModel_render_all_nodes(scene);
	}
}
