// Mesh.h - 3D mesh of triangles (c) 2019-2022 Jules Bloomenthal

#ifndef MESH_HDR
#define MESH_HDR

#include <glad.h>
#include <stdio.h>
#include <vector>
#include "CameraArcball.h"
#include "Quaternion.h"
#include "VecMat.h"

using std::string;
using std::vector;

// Mesh Class and Operations

GLuint GetMeshShader(bool lines = false);
GLuint UseMeshShader(bool lines = false);
	// lines true uses geometry shader to draw lines along triangle edges
	// lines false is slightly more efficient

class Frame {
public:
	Frame() { };
	Frame(Quaternion q, vec3 p, float s) : orientation(q), position(p), scale(s) { };
	Quaternion orientation;
	vec3 position;
	float scale = 1;
};

struct Group {
	string name;
	int startTriangle = 0, nTriangles = 0;
	vec3 color = vec3(1, 1, 1);
	Group(int start = 0, string n = "", vec3 c = vec3(1, 1, 1)) : startTriangle(start), name(n), color(c) { }
};

struct Mtl {
	string name;
	vec3 ka, kd, ks;
	int startTriangle = 0, nTriangles = 0;
	Mtl() {startTriangle = -1, nTriangles = 0; }
	Mtl(int start, string n, vec3 a, vec3 d, vec3 s) : startTriangle(start), name(n), ka(a), kd(d), ks(s) { }
};

class Mesh {
public:
	Mesh() { };
	Mesh(const char *filename) { Read(string(filename)); }
	~Mesh() { glDeleteBuffers(1, &vBufferId); };
	string objFilename, texFilename;
	// vertices and facets
	vector<vec3>	points;
	vector<vec3>	normals;
	vector<vec2>	uvs;
	vector<int3>	triangles;
	vector<int4>	quads;
	// ancillary data
	vector<Group>	triangleGroups;
	vector<Mtl>		triangleMtls;
	// position/orientation
	mat4			transform;				// object to world space, set during drag
	Frame			frameDown;				// reference frame on mouse down
	// hierarchy
	vector<Mesh *>	children;
	// GPU vertex buffer and texture
	GLuint			vao = 0;				// vertex array object
	GLuint			vBufferId = 0;			// vertex buffer
	GLuint			eBufferId = 0;			// element (triangle) buffer
	GLuint			textureName = 0;
	// operations
	void Buffer();
	void Buffer(vector<vec3> &pts, vector<vec3> *nrms = NULL, vector<vec2> *uvs = NULL);
		// if non-null, nrms and uvs assumed same size as pts
	void Set(vector<vec3> &pts, vector<vec3> *nrms = NULL, vector<vec2> *tex = NULL,
			 vector<int> *tris = NULL, vector<int> *quas = NULL);
			 // **** maybe we don't want this routine
	void Display(CameraAB camera, int textureUnit = 0, bool lines = false, bool useGroupColor = false);
		// texture is enabled if textureUnit >= 0 and textureName previously set
		// before this call, app must optionally change uniforms from their default, including:
		//     nLights, lights, color, opacity, ambient
		//     useLight, useTint, fwdFacingOnly, facetedShading
		//     outlineColor, outlineWidth, transition
//	void Display(CameraAB camera, bool lines = false, int textureUnit = -1, bool useGroupColor = false);
	bool Read(string objFile, mat4 *m = NULL, bool normalize = true, bool buffer = true);
		// read in object file (with normals, uvs), initialize matrix, build vertex buffer
	bool Read(string objFile, string texFile, mat4 *m = NULL, bool normalize = true, bool buffer = true);
		// read in object file (with normals, uvs) and texture file, initialize matrix, build vertex buffer
		// textureUnit must be > 0
};

class MeshFramer { // rename Articulater? derive from Widgets::Framer?
public:
	Mesh *mesh = NULL;
	Arcball arcball;
	MeshFramer() { }
	void Set(Mesh *m, float radius, mat4 fullview);
	void SetFramedown(Mesh *m);
		// set m.qstart from m.transform and recurse on m.children
	void RotateTransform(Mesh *m, Quaternion qrot, vec3 *center = NULL);
		// apply qrot to qstart, optionally rotate base around center
		// set m.transform, recurse on m.children
	void TranslateTransform(Mesh *m, vec3 pDif);
	bool Hit(int x, int y);
	void Down(int x, int y, mat4 modelview, mat4 persp, bool control = false);
	void Drag(int x, int y, mat4 modelview, mat4 persp);
		// recursively apply to mesh.children
	void Up();
	void Wheel(double spin, bool shift);
	void Draw(mat4 fullview);
private:
	bool moverPicked = false;
	Mover mover;
};

// Read STL Format

struct VertexSTL {
	vec3 point, normal;
	VertexSTL() { }
	VertexSTL(float *p, float *n) : point(vec3(p[0], p[1], p[2])), normal(vec3(n[0], n[1], n[2])) { }
};

int ReadSTL(const char *filename, vector<VertexSTL> &vertices);
	// read vertices from file, three per triangle; return # triangles

// Read OBJ Format

bool ReadAsciiObj(const char    *filename,                  // must be ASCII file
				  vector<vec3>  &points,                    // unique set of points determined by vertex/normal/uv triplets in file
				  vector<int3>  &triangles,                 // array of triangle vertex ids
				  vector<vec3>  *normals  = NULL,           // if non-null, read normals from file, correspond with points
				  vector<vec2>  *textures = NULL,           // if non-null, read uvs from file, correspond with points
				  vector<Group> *triangleGroups = NULL,     // correspond with triangle groups
				  vector<Mtl>   *triangleMtls = NULL,		// correspond with triangle groups
				  vector<int4>  *quads = NULL,              // optional quadrilaterals
				  vector<int2>  *segs = NULL);				// optional line segments
	// set points and triangles; normals, textures, quads optional
	// return true if successful

bool WriteAsciiObj(const char      *filename,
				   vector<vec3>    &points,
				   vector<vec3>    &normals,
				   vector<vec2>    &uvs,
				   vector<int3>    *triangles = NULL,
				   vector<int4>    *quads = NULL,
				   vector<int2>    *segs = NULL,
				   vector<Group>   *triangleGroups = NULL);
	// write to file mesh points, normals, and uvs
	// optionally write triangles and/or quadrilaterals

// Bounding Box

void MinMax(vec2 *points, int npoints, vec2 &min, vec2 &max);

void MinMax(vec3 *points, int npoints, vec3 &min, vec3 &max);

mat4 NormalizeMat(vec3 *points, int npoints, float scale = 1);

void Normalize(vec3 *points, int npoints, float scale = 1);
	// translate and apply uniform scale so that vertices fit in -scale,+scale in X,Y,Z

void Normalize(vector<vec3> &points, float scale = 1);

void Normalize(vector<VertexSTL> &vertices, float scale = 1);

// Normals

void SetVertexNormals(vector<vec3> &points, vector<int3> &triangles, vector<vec3> &normals);
	// compute/recompute vertex normals as the average of surrounding triangle normals

// Intersections

bool IsInside(const vec2 &p, vector<vec2> &pts);

bool IsInside(const vec2 &p, const vec2 &a, const vec2 &b, const vec2 &c);

struct TriInfo {
	vec4 plane;
	int majorPlane = 0; // 0: XY, 1: XZ, 2: YZ
	vec2 p1, p2, p3;    // vertices projected to majorPlane
	TriInfo() { };
	TriInfo(vec3 p1, vec3 p2, vec3 p3);
};

void BuildTriInfos(vector<vec3> &points, vector<int3> &triangles, vector<TriInfo> &triInfos);
	// for interactive selection

int IntersectWithLine(vec3 p1, vec3 p2, vector<TriInfo> &triInfos, float &alpha);
	// return triangle index of nearest intersected triangle, or -1 if none
	// intersection = p1+alpha*(p2-p1)

#endif
