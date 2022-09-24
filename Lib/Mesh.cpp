// Mesh.cpp - mesh IO and operations (c) 2019-2022 Jules Bloomenthal

#include "CameraArcball.h"
#include "GLXtras.h"
#include "Draw.h"
#include "Mesh.h"
#include "Misc.h"
#include "Quaternion.h"
#include <assert.h>
#include <iostream>
#include <fstream>
#include <direct.h>
#include <float.h>
#include <string.h>
#include <cstdlib>
#include "VecMat.h"
#include "Widgets.h"

using std::string;
using std::vector;
using std::ios;
using std::ifstream;

// Mesh Framer

bool MeshFramer::Hit(int x, int y) {
	return arcball.Hit(x, y);
}

void MeshFramer::Up() {
	arcball.Up();
}

void MeshFramer::Set(Mesh *m, float radius, mat4 fullview) {
	mesh = m;
	m->frameDown = Frame(Quaternion(m->transform), MatrixOrigin(m->transform), MatrixScale(m->transform));
	arcball.SetBody(m->transform, radius);
	arcball.SetCenter(ScreenPoint(m->frameDown.position, fullview));
	moverPicked = false;
}

void MeshFramer::SetFramedown(Mesh *m) {
	m->frameDown = Frame(Quaternion(m->transform), MatrixOrigin(m->transform), MatrixScale(m->transform));
	for (int i = 0; i < (int) m->children.size(); i++)
		SetFramedown(m->children[i]);
}

void MeshFramer::Down(int x, int y, mat4 modelview, mat4 persp, bool control) {
	moverPicked = arcball.MouseOver(x, y);
	SetFramedown(mesh);
	if (moverPicked)
		mover.Down(&mesh->frameDown.position, x, y, modelview, persp);
	else
		arcball.Down(x, y, control, &mesh->transform);
			// mesh->transform used by arcball.SetNearestAxis
}

void MeshFramer::Drag(int x, int y, mat4 modelview, mat4 persp) {
	if (moverPicked) {
		vec3 pDif = mover.Drag(x, y, modelview, persp);
		SetMatrixOrigin(mesh->transform, mesh->frameDown.position);
		for (int i = 0; i < (int) mesh->children.size(); i++)
			TranslateTransform(mesh->children[i], pDif);
		arcball.SetCenter(ScreenPoint(mesh->frameDown.position, persp*modelview));
	}
	else {
		Quaternion qrot = arcball.Drag(x, y);
		// recurse on children
		RotateTransform(mesh, qrot, NULL);
	}
}

void MeshFramer::RotateTransform(Mesh *m, Quaternion qrot, vec3 *center) {
	// rotate selected mesh and child meshes by qrot (returned by Arcball::Drag)
	//   apply qrot to rotation elements of m->transform (upper left 3x3)
	//   if non-null center, rotate origin of m about center
	// recursive routine initially called with null center
	Quaternion qq = m->frameDown.orientation*qrot; // arcball:use=Camera(?) works (qrot*m->qstart Body? fails)
	// rotate m
	qq.SetMatrix(m->transform, m->frameDown.scale);
	if (center) {
		// this is a child mesh: rotate origin of mesh around center
		mat4 rot = qrot.GetMatrix();
		mat4 x = Translate((*center))*rot*Translate(-(*center));
		vec4 xbase = x*vec4(m->frameDown.position, 1);
		SetMatrixOrigin(m->transform, vec3(xbase.x, xbase.y, xbase.z));
	}
	for (int i = 0; i < (int) m->children.size(); i++)
		RotateTransform(m->children[i], qrot, center? center : &m->frameDown.position);
			// rotate descendant children around initial mesh base  
}

void MeshFramer::TranslateTransform(Mesh *m, vec3 pDif) {
	SetMatrixOrigin(m->transform, m->frameDown.position+pDif);
	for (int i = 0; i < (int) m->children.size(); i++)
		TranslateTransform(m->children[i], pDif);
}

void MeshFramer::Wheel(double spin, bool shift) {
	mesh->frameDown.scale *= (spin > 0? 1.01f : .99f);
	Scale3x3(mesh->transform, mesh->frameDown.scale/MatrixScale(mesh->transform));
}

void MeshFramer::Draw(mat4 fullview) {
	UseDrawShader(ScreenMode());
	arcball.Draw(Control(), &mesh->transform);
	UseDrawShader(fullview);
	Disk(mesh->frameDown.position, 9, arcball.pink);
}

namespace {

GLuint meshShaderLines = 0, meshShaderNoLines = 0;

// vertex shader
const char *meshVertexShader = R"(
	#version 330
	layout (location = 0) in vec3 point;
	layout (location = 1) in vec3 normal;
	layout (location = 2) in vec2 uv;
	layout (location = 3) in mat4 instance; // for use with glDrawArrays/ElementsInstanced
											// uses locations 3,4,5,6 for 4 vec4s = mat4
	layout (location = 7) in vec3 color;	// for instanced color (vec4?)
	out vec3 vPoint;
	out vec3 vNormal;
	out vec2 vUv;
//	out vec3 vColor;
	uniform bool useInstance = false;
	uniform mat4 modelview;
	uniform mat4 persp;
	void main() {
		mat4 m = useInstance? modelview*instance : modelview;
		vPoint = (m*vec4(point, 1)).xyz;
		vNormal = (m*vec4(normal, 0)).xyz;
		gl_Position = persp*vec4(vPoint, 1);
		vUv = uv;
//		vColor = color;
	}
)";

// geometry shader
const char *meshGeometryShader = R"(
	#version 330
	layout (triangles) in;
	layout (triangle_strip, max_vertices = 3) out;
	in vec3 vPoint[], vNormal[];
	in vec2 vUv[];
	out vec3 gPoint, gNormal;
	out vec2 gUv;
	noperspective out vec3 gEdgeDistance;
	uniform mat4 vp;
	vec3 ViewPoint(int i) { return vec3(vp*(gl_in[i].gl_Position/gl_in[i].gl_Position.w)); }
	void main() {
		float ha = 0, hb = 0, hc = 0;
		// transform each vertex to viewport space
		vec3 p0 = ViewPoint(0), p1 = ViewPoint(1), p2 = ViewPoint(2);
		// find altitudes ha, hb, hc
		float a = length(p2-p1), b = length(p2-p0), c = length(p1-p0);
		float alpha = acos((b*b+c*c-a*a)/(2.*b*c));
		float beta = acos((a*a+c*c-b*b)/(2.*a*c));
		ha = abs(c*sin(beta));
		hb = abs(c*sin(alpha));
		hc = abs(b*sin(alpha));
		// send triangle vertices and edge distances
		for (int i = 0; i < 3; i++) {
			gEdgeDistance = i==0? vec3(ha, 0, 0) : i==1? vec3(0, hb, 0) : vec3(0, 0, hc);
			gPoint = vPoint[i];
			gNormal = vNormal[i];
			gUv = vUv[i];
			gl_Position = gl_in[i].gl_Position;
			EmitVertex();
		}
		EndPrimitive();
	}
)";

// pixel shader
const char *meshPixelShaderLines = R"(
	#version 330
	in vec3 gPoint, gNormal;
	in vec2 gUv;
	noperspective in vec3 gEdgeDistance;
	uniform sampler2D textureImage;
	uniform int nLights = 1;
	uniform vec3 lights[20] = vec3[20](vec3(1, 1, 1));
	uniform vec3 color = vec3(1);
	uniform float opacity = 1;
	uniform float ambient = .2;
	uniform bool useLight = true;
	uniform bool useTexture = true;
	uniform bool useTint = false;
	uniform bool fwdFacingOnly = false;
	uniform bool facetedShading = false;
	uniform vec4 outlineColor = vec4(0, 0, 0, 1);
	uniform float outlineWidth = 1;
	uniform float outlineTransition = 1;
	out vec4 pColor;
	float Intensity(vec3 normalV, vec3 eyeV, vec3 point, vec3 light) {
		vec3 lightV = normalize(light-point);		// light vector
		vec3 reflectV = reflect(lightV, normalV);   // highlight vector
		float d = max(0, dot(normalV, lightV));     // one-sided diffuse
		float s = max(0, dot(reflectV, eyeV));      // one-sided specular
		return clamp(d+pow(s, 50), 0, 1);
	}
	void main() {
		vec3 N = normalize(facetedShading? cross(dFdx(gPoint), dFdy(gPoint)) : gNormal);
		if (fwdFacingOnly && N.z < 0)
			discard;
		vec3 E = normalize(gPoint);					// eye vector
		float intensity = useLight? 0 : 1;
		if (useLight)
			for (int i = 0; i < nLights; i++)
				intensity += Intensity(N, E, gPoint, lights[i]);
		intensity = clamp(intensity, 0, 1);
		if (useTexture) {
			pColor = vec4(intensity*texture(textureImage, gUv).rgb, opacity);
			if (useTint) {
				pColor.r *= color.r;
				pColor.g *= color.g;
				pColor.b *= color.b;
			}
		}
		else
			pColor = vec4(intensity*color, opacity);
		float minDist = min(gEdgeDistance.x, gEdgeDistance.y);
		minDist = min(minDist, gEdgeDistance.z);
		float t = smoothstep(outlineWidth-outlineTransition, outlineWidth+outlineTransition, minDist);
		// mix edge and surface colors(t=0: edgeColor, t=1: surfaceColor)
		pColor = mix(outlineColor, pColor, t);
	}
)";

const char *meshPixelShaderNoLines = R"(
	#version 330
	in vec3 vPoint, vNormal;
	in vec2 vUv;
	uniform sampler2D textureImage;
	uniform int nLights = 1;
	uniform vec3 lights[20] = vec3[20](vec3(1, 1, 1));
	uniform vec3 color = vec3(1);
	uniform float opacity = 1;
	uniform float ambient = .2;
	uniform bool useLight = true;
	uniform bool useTexture = true;
	uniform bool useTint = false;
	uniform bool fwdFacingOnly = false;
	uniform bool facetedShading = false;
	out vec4 pColor;
	float Intensity(vec3 normalV, vec3 eyeV, vec3 point, vec3 light) {
		vec3 lightV = normalize(light-point);		// light vector
		vec3 reflectV = reflect(lightV, normalV);   // highlight vector
		float d = max(0, dot(normalV, lightV));     // one-sided diffuse
		float s = max(0, dot(reflectV, eyeV));      // one-sided specular
		return clamp(d+pow(s, 50), 0, 1);
	}
	void main() {
		vec3 N = normalize(facetedShading? cross(dFdx(vPoint), dFdy(vPoint)) : vNormal);
		if (fwdFacingOnly && N.z < 0)
			discard;
		vec3 E = normalize(vPoint);					// eye vector
		float intensity = useLight? 0 : 1;
		if (useLight)
			for (int i = 0; i < nLights; i++)
				intensity += Intensity(N, E, vPoint, lights[i]);
		intensity = clamp(intensity, 0, 1);
		if (useTexture) {
			pColor = vec4(intensity*texture(textureImage, vUv).rgb, opacity);
			if (useTint) {
				pColor.r *= color.r;
				pColor.g *= color.g;
				pColor.b *= color.b;
			}
		}
		else
			pColor = vec4(intensity*color, opacity);
	}
)";

} // end namespace

GLuint GetMeshShader(bool lines) {
	if (lines) {
		if (!meshShaderLines)
			meshShaderLines = LinkProgramViaCode(&meshVertexShader, NULL, NULL, &meshGeometryShader, &meshPixelShaderLines);
		return meshShaderLines;
	}
	else {
		if (!meshShaderNoLines)
			meshShaderNoLines = LinkProgramViaCode(&meshVertexShader, &meshPixelShaderNoLines);
		return meshShaderNoLines;
	}
}

GLuint UseMeshShader(bool lines) {
	GLuint s = GetMeshShader(lines);
	glUseProgram(s);
	return s;
}

// Mesh Class

void Mesh::Display(CameraAB camera, int textureUnit, bool lines, bool useGroupColor) {
	int nTris = triangles.size(), nQuads = quads.size();
	// enable shader and vertex array object
	int shader = UseMeshShader(lines);
	glBindVertexArray(vao);
	// texture
	if (!textureName || !uvs.size() || textureUnit < 0)
		SetUniform(shader, "useTexture", false);
	else {
		glActiveTexture(GL_TEXTURE0+textureUnit);
		glBindTexture(GL_TEXTURE_2D, textureName);
		SetUniform(shader, "textureImage", textureUnit); // but app can unset useTexture
	}
	// set custom transform and draw (xform = mesh transform X view transform)
	SetUniform(shader, "modelview", camera.modelview*transform);
	SetUniform(shader, "persp", camera.persp);
	int textureSet = 0;
	glGetUniformiv(shader, glGetUniformLocation(shader, "useTexture"), &textureSet);
	if (lines)
		SetUniform(shader, "vp", Viewport());
	if (useGroupColor) {
		// show ungrouped triangles without texture mapping
		int nGroups = triangleGroups.size(), nUngrouped = nGroups? triangleGroups[0].startTriangle : nTris;
		SetUniform(shader, "useTexture", false);
		glDrawElements(GL_TRIANGLES, 3*nUngrouped, GL_UNSIGNED_INT, triangles.data());
		// show grouped triangles with texture mapping
		SetUniform(shader, "useTexture", textureSet == 1);
		for (int i = 0; i < nGroups; i++) {
			Group g = triangleGroups[i];
			SetUniform(shader, "color", g.color);
			glDrawElements(GL_TRIANGLES, 3*g.nTriangles, GL_UNSIGNED_INT, &triangles[g.startTriangle]);
		}
	}
	else {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eBufferId);
		glDrawElements(GL_TRIANGLES, 3*nTris, GL_UNSIGNED_INT, 0);
//		glDrawElements(GL_TRIANGLES, 3*nTris, GL_UNSIGNED_INT, triangles.data());
#ifdef GL_QUADS
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glDrawElements(GL_QUADS, 4*nQuads, GL_UNSIGNED_INT, quads.data());
#endif
	}
	glBindVertexArray(0);
}

void Enable(int id, int ncomps, int offset) {
	glEnableVertexAttribArray(id);
	glVertexAttribPointer(id, ncomps, GL_FLOAT, GL_FALSE, 0, (void *) offset);
}

void Mesh::Buffer(vector<vec3> &pts, vector<vec3> *nrms, vector<vec2> *tex) {
	int nPts = pts.size(), nNrms = nrms? nrms->size() : 0, nUvs = tex? tex->size() : 0;
	if (!nPts) { printf("Buffer: no points!\n"); return; }
	// create vertex buffer
	if (!vBufferId)
		glGenBuffers(1, &vBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, vBufferId);
	// allocate GPU memory for vertex position, texture, normals
	int sizePoints = nPts*sizeof(vec3), sizeNormals = nNrms*sizeof(vec3), sizeUvs = nUvs*sizeof(vec2);
	int bufferSize = sizePoints+sizeUvs+sizeNormals;
	glBufferData(GL_ARRAY_BUFFER, bufferSize, NULL, GL_STATIC_DRAW);
	// load vertex buffer
	if (nPts) glBufferSubData(GL_ARRAY_BUFFER, 0, sizePoints, pts.data());
	if (nNrms) glBufferSubData(GL_ARRAY_BUFFER, sizePoints, sizeNormals, nrms->data());
	if (nUvs) glBufferSubData(GL_ARRAY_BUFFER, sizePoints+sizeNormals, sizeUvs, tex->data());
	// create and load element buffer for triangles
	int sizeTriangles = sizeof(int3)*triangles.size();
	glGenBuffers(1, &eBufferId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eBufferId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeTriangles, triangles.data(), GL_STATIC_DRAW);
	// create vertex array object for mesh
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	// enable attributes
	if (nPts) Enable(0, 3, 0);						// VertexAttribPointer(shader, "point", 3, 0, (void *) 0);
	if (nNrms) Enable(1, 3, sizePoints);			// VertexAttribPointer(shader, "normal", 3, 0, (void *) sizePoints);
	if (nUvs) Enable(2, 2, sizePoints+sizeNormals); // VertexAttribPointer(shader, "uv", 2, 0, (void *) (sizePoints+sizeNormals));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void Mesh::Buffer() { Buffer(points, normals.size()? &normals : NULL, uvs.size()? &uvs : NULL); }

void Mesh::Set(vector<vec3> &pts, vector<vec3> *nrms, vector<vec2> *tex, vector<int> *tris, vector<int> *quas) {
	if (tris) {
		triangles.resize(tris->size()/3);
		for (int i = 0; i < (int) triangles.size(); i++)
			triangles[i] = { (*tris)[3*i], (*tris)[3*i+1], (*tris)[3*i+2] };
	}
	if (quas) {
		quads.resize(quas->size()/4);
		for (int i = 0; i < (int) quads.size(); i++)
			quads[i] = { (*quas)[4*i], (*quas)[4*i+1], (*quas)[4*i+2], (*quas)[4*i+3] };
	}
	Buffer(pts, nrms, tex);
}

bool Mesh::Read(string objFile, mat4 *m, bool normalize, bool buffer) {
	if (!ReadAsciiObj((char *) objFile.c_str(), points, triangles, &normals, &uvs, &triangleGroups, &triangleMtls, &quads, NULL)) {
		printf("Mesh.Read: can't read %s\n", objFile.c_str());
		return false;
	}
	objFilename = objFile;
	if (normalize)
		Normalize(points, 1);
	if (buffer)
		Buffer();
	if (m)
		transform = *m;
	return true;
}

bool Mesh::Read(string objFile, string texFile, mat4 *m, bool normalize, bool buffer) {
	if (!Read(objFile, m, normalize, buffer))
		return false;
	objFilename = objFile;
	texFilename = texFile;
	textureName = LoadTexture((char *) texFile.c_str());
	if (!textureName)
		printf("Mesh.Read: bad texture name\n");
	return textureName > 0;
}

// intersections

vec2 MajPln(vec3 &p, int mp) { return mp == 1? vec2(p.y, p.z) : mp == 2? vec2(p.x, p.z) : vec2(p.x, p.y); }

TriInfo::TriInfo(vec3 a, vec3 b, vec3 c) {
	vec3 v1(b-a), v2(c-b), x = normalize(cross(v1, v2));
	plane = vec4(x.x, x.y, x.z, -dot(a, x));
	float ax = fabs(x.x), ay = fabs(x.y), az = fabs(x.z);
	majorPlane = ax > ay? (ax > az? 1 : 3) : (ay > az? 2 : 3);
	p1 = MajPln(a, majorPlane);
	p2 = MajPln(b, majorPlane);
	p3 = MajPln(c, majorPlane);
}

bool LineIntersectPlane(vec3 p1, vec3 p2, vec4 plane, vec3 *intersection, float *alpha) {
  vec3 normal(plane.x, plane.y, plane.z);
  vec3 axis(p2-p1);
  float pdDot = dot(axis, normal);
  if (fabs(pdDot) < FLT_MIN)
	  return false;
  float a = (-plane.w-dot(p1, normal))/pdDot;
  if (intersection != NULL)
	  *intersection = p1+a*axis;
  if (alpha)
	  *alpha = a;
  return true;
}

static bool IsZero(float d) { return d < FLT_EPSILON && d > -FLT_EPSILON; };

int CompareVs(vec2 &v1, vec2 &v2) {
	if ((v1.y > 0 && v2.y > 0) ||           // edge is fully above query point p'
		(v1.y < 0 && v2.y < 0) ||           // edge is fully below p'
		(v1.x < 0 && v2.x < 0))             // edge is fully left of p'
		return 0;                           // can't cross
	float zcross = v2.y*v1.x-v1.y*v2.x;     // right-handed cross-product
	zcross /= length(v1-v2);
	if (IsZero(zcross) && (v1.x <= 0 || v2.x <= 0))
		return 1;                           // on or very close to edge
	if ((v1.y > 0 || v2.y > 0) && ((v1.y-v2.y < 0) != (zcross < 0)))
		return 2;                           // edge is crossed
	else
		return 0;                           // edge not crossed
}

bool IsInside(const vec2 &p, vector<vec2> &pts) {
	bool odd = false;
	int npts = pts.size();
	vec2 q = p, v2 = pts[npts-1]-q;
	for (int n = 0; n < npts; n++) {
		vec2 v1 = v2;
		v2 = pts[n]-q;
		if (CompareVs(v1, v2) == 2)
			odd = !odd;
	}
	return odd;
}

bool IsInside(const vec2 &p, const vec2 &a, const vec2 &b, const vec2 &c) {
	bool odd = false;
	vec2 q = p, v2 = c-q;
	for (int n = 0; n < 3; n++) {
		vec2 v1 = v2;
		v2 = (n==0? a : n==1? b : c)-q;
		if (CompareVs(v1, v2) == 2)
			odd = !odd;
	}
	return odd;
}

void BuildTriInfos(vector<vec3> &points, vector<int3> &triangles, vector<TriInfo> &triInfos) {
	triInfos.resize(triangles.size());
	for (size_t i = 0; i < triangles.size(); i++) {
		int3 &t = triangles[i];
		triInfos[i] = TriInfo(points[t.i1], points[t.i2], points[t.i3]);
	}
}

int IntersectWithLine(vec3 p1, vec3 p2, vector<TriInfo> &triInfos, float &retAlpha) {
	int picked = -1;
	float alpha, minAlpha = FLT_MAX;
	for (size_t i = 0; i < triInfos.size(); i++) {
		TriInfo &t = triInfos[i];
		vec3 inter;
		if (LineIntersectPlane(p1, p2, t.plane, &inter, &alpha)) {
			if (alpha < minAlpha) {
				if (IsInside(MajPln(inter, t.majorPlane), t.p1, t.p2, t.p3)) {
					minAlpha = alpha;
					picked = i;
				}
			}
		}
	}
	retAlpha = minAlpha;
	return picked;
}

// center/scale for unit size models

void UpdateMinMax(vec3 p, vec3 &min, vec3 &max) {
	for (int k = 0; k < 3; k++) {
		if (p[k] < min[k]) min[k] = p[k];
		if (p[k] > max[k]) max[k] = p[k];
	}
}

float GetScaleCenter(vec3 &min, vec3 &max, float scale, vec3 &center) {
	center = .5f*(min+max);
	float maxrange = 0;
	for (int k = 0; k < 3; k++)
		if ((max[k]-min[k]) > maxrange)
			maxrange = max[k]-min[k];
	return scale*2.f/maxrange;
}

// normalize STL models

void MinMax(vector<VertexSTL> &points, vec3 &min, vec3 &max) {
	min.x = min.y = min.z = FLT_MAX;
	max.x = max.y = max.z = -FLT_MAX;
	for (int i = 0; i < (int) points.size(); i++)
		UpdateMinMax(points[i].point, min, max);
}

void Normalize(vector<VertexSTL> &vertices, float scale) {
	vec3 min, max, center;
	MinMax(vertices, min, max);
	float s = GetScaleCenter(min, max, scale, center);
	for (int i = 0; i < (int) vertices.size(); i++) {
		vec3 &v = vertices[i].point;
		v = s*(v-center);
	}
}

// normalize vec3 models

mat4 NDCfromMinMax(vec3 min, vec3 max, float scale = 1) {
	// matrix to transform min/max to -1/+1 (uniformly)
	float maxrange = 0;
	for (int k = 0; k < 3; k++)
		if ((max[k]-min[k]) > maxrange)
			maxrange = max[k]-min[k];
	float s = scale*2.f/maxrange;
	vec3 center = .5f*(max+min);
	return Scale(s)*Translate(-center);
}

void MinMax(vec2 *points, int npoints, vec2 &min, vec2 &max) {
	min[0] = min[1] = FLT_MAX;
	max[0] = max[1] = -FLT_MAX;
	for (int i = 0; i < npoints; i++) {
		vec2 &v = points[i];
		for (int k = 0; k < 2; k++) {
			if (v[k] < min[k]) min[k] = v[k];
			if (v[k] > max[k]) max[k] = v[k];
		}
	}
}

void MinMax(vec3 *points, int npoints, vec3 &min, vec3 &max) {
	min[0] = min[1] = min[2] = FLT_MAX;
	max[0] = max[1] = max[2] = -FLT_MAX;
	for (int i = 0; i < npoints; i++) {
		vec3 &v = points[i];
		for (int k = 0; k < 3; k++) {
			if (v[k] < min[k]) min[k] = v[k];
			if (v[k] > max[k]) max[k] = v[k];
		}
	}
}

void MinMax(vector<vec3> &points, vec3 &min, vec3 &max) {
	MinMax(points.data(), points.size(), min, max);
}

mat4 NormalizeMat(vec3 *points, int npoints, float scale) {
	vec3 min, max;
	MinMax(points, npoints, min, max);
	return NDCfromMinMax(min, max, scale);
}

void Normalize(vec3 *points, int npoints, float scale) {
	mat4 m = NormalizeMat(points, npoints, scale);
	for (int i = 0; i < npoints; i++) {
		vec4 xp = m*vec4(points[i]);
		points[i] = vec3(xp.x, xp.y, xp.z);
	}
/*	vec3 center(.5f*(min[0]+max[0]), .5f*(min[1]+max[1]), .5f*(min[2]+max[2]));
	float maxrange = 0;
	for (int k = 0; k < 3; k++)
		if ((max[k]-min[k]) > maxrange)
			maxrange = max[k]-min[k];
	float s = scale*2.f/maxrange;
	for (int i = 0; i < npoints; i++) {
		vec3 &v = points[i];
		for (int k = 0; k < 3; k++)
			v[k] = s*(v[k]-center[k]);
	} */
}

void Normalize(vector<vec3> &points, float scale) {
	Normalize(points.data(), points.size(), scale);
}

void SetVertexNormals(vector<vec3> &points, vector<int3> &triangles, vector<vec3> &normals) {
	// size normals array and initialize to zero
	int nverts = (int) points.size();
	normals.resize(nverts, vec3(0,0,0));
	// accumulate each triangle normal into its three vertex normals
	for (int i = 0; i < (int) triangles.size(); i++) {
		int3 &t = triangles[i];
		vec3 &p1 = points[t.i1], &p2 = points[t.i2], &p3 = points[t.i3];
		vec3 n(cross(p2-p1, p3-p2));
//		vec3 n(normalize(cross(p2-p1, p3-p2)));
		normals[t.i1] += n;
		normals[t.i2] += n;
		normals[t.i3] += n;
	}
	// set to unit length
	for (int i = 0; i < nverts; i++)
		normals[i] = normalize(normals[i]);
}

// ASCII support

bool ReadWord(char* &ptr, char *word, int charLimit) {
	ptr += strspn(ptr, " \t");                  // skip white space
	int nChars = strcspn(ptr, " \t");           // get # non-white-space characters
	if (!nChars)
		return false;                           // no non-space characters
	int nRead = charLimit-1 < nChars? charLimit-1 : nChars;
	strncpy(word, ptr, nRead);
	word[nRead] = 0;                            // strncpy does not null terminate
	ptr += nChars;
	return true;
}

// STL

char *Lower(char *word) {
	for (char *c = word; *c; c++)
		*c = tolower(*c);
	return word;
}

int ReadSTL(const char *filename, vector<VertexSTL> &vertices) {
	// the facet normal should point outwards from the solid object; if this is zero,
	// most software will calculate a normal from the ordered triangle vertices using the right-hand rule
	class Helper {
	public:
		bool status = false;
		int nTriangles = 0;
		vector<VertexSTL> *verts;
		vector<string> vSpecs;                              // ASCII only
		Helper(const char *filename, vector<VertexSTL> *verts) : verts(verts) {
			char line[1000], word[1000], *ptr = line;
			ifstream inText(filename, ios::in);             // text default mode
			inText.getline(line, 10);
			bool ascii = ReadWord(ptr, word, 10) && !strcmp(Lower(word), "solid");
			// bool ascii = ReadWord(ptr, word, 10) && !_stricmp(word, "solid");
			ascii = false; // hmm!
			if (ascii)
				status = ReadASCII(inText);
			inText.close();
			if (!ascii) {
				FILE *inBinary = fopen(filename, "rb");     // inText.setmode(ios::binary) fails
				if (inBinary) {
					nTriangles = 0;
					status = ReadBinary(inBinary);
					fclose(inBinary);
				}
				else
					status = false;
			}
		}
		bool ReadASCII(ifstream &in) {
			printf("can't read ASCII STL\n");
			return true;
		}
		bool ReadBinary(FILE *in) {
				  // # bytes      use                  significance
				  // -------      ---                  ------------
				  //      80      header               none
				  //       4      unsigned long int    number of triangles
				  //      12      3 floats             triangle normal
				  //      12      3 floats             x,y,z for vertex 1
				  //      12      3 floats             vertex 2
				  //      12      3 floats             vertex 3
				  //       2      unsigned short int   attribute (0)
				  // endianness is assumed to be little endian
			// in.setmode(ios::binary); doc says setmode good, but compiler says not so
			// sizeof(bool)=1, sizeof(char)=1, sizeof(short)=2, sizeof(int)=4, sizeof(float)=4
			char buf[81];
			int nTriangle = 0;//, vid1, vid2, vid3;
			if (fread(buf, 1, 80, in) != 80) // header
				return false;
			if (fread(&nTriangles, sizeof(int), 1, in) != 1)
				return false;
			while (!feof(in)) {
				vec3 v[3], n;
				if (nTriangle == nTriangles)
					break;
				if (nTriangles > 5000 && nTriangle && nTriangle%1000 == 0)
					printf("\rread %i/%i triangles", nTriangle, nTriangles);
				if (fread(&n.x, sizeof(float), 3, in) != 3)
					printf("\ncan't read triangle %d normal\n", nTriangle);
				for (int k = 0; k < 3; k++)
					if (fread(&v[k].x, sizeof(float), 3, in) != 3)
						printf("\ncan't read vid %d\n", verts->size());
				vec3 a(v[1]-v[0]), b(v[2]-v[1]);
				vec3 ntmp = cross(a, b);
				if (dot(ntmp, n) < 0) {
					vec3 vtmp = v[0];
					v[0] = v[2];
					v[2] = vtmp;
				}
				for (int k = 0; k < 3; k++)
					verts->push_back(VertexSTL((float *) &v[k].x, (float *) &n.x));
				unsigned short attribute;
				if (fread(&attribute, sizeof(short), 1, in) != 1)
					printf("\ncan't read attribute\n");
				nTriangle++;
			}
			printf("\r\t\t\t\t\t\t\r");
			return true;
		}
	};
	Helper h(filename, &vertices);
	return h.nTriangles;
} // end ReadSTL

// ASCII OBJ

#include <map>

static const int LineLim = 10000, WordLim = 1000;

struct CompareS {
	bool operator() (const string &a, const string &b) const { return (a < b); }
};

typedef std::map<string, Mtl, CompareS> MtlMap;
	// string is key, Mtl is value

MtlMap ReadMaterial(const char *filename) {
	MtlMap mtlMap;
	char line[LineLim], word[WordLim];
	Mtl m;
	FILE *in = fopen(filename, "r");
	string key;
	Mtl value;
	if (in)
		for (int lineNum = 0;; lineNum++) {
			line[0] = 0;
			fgets(line, LineLim, in);                   // \ line continuation not supported
			if (feof(in))                               // hit end of file
				break;
			if (strlen(line) >= LineLim-1) {            // getline reads LineLim-1 max
				printf("line %d too long\n", lineNum);
				continue;
			}
			line[strlen(line)-1] = 0;							// remove carriage-return
			char *ptr = line;
			if (!ReadWord(ptr, word, WordLim) || *word == '#')
				continue;
			Lower(word);
			if (!strcmp(word, "newmtl") && ReadWord(ptr, word, WordLim)) {
				key = string(word);
				value.name = string(word);
			}
			if (!strcmp(word, "kd")) {
				if (sscanf(ptr, "%g%g%g", &value.kd.x, &value.kd.y, &value.kd.z) != 3)
					printf("bad line %d in material file", lineNum);
				else
					mtlMap[key] = value;
			}
		}
//	else printf("can't open %s\n", filename);
	return mtlMap;
}

struct CompareVid {
	bool operator() (const int3 &a, const int3 &b) const {
		return (a.i1==b.i1? (a.i2==b.i2? a.i3 < b.i3 : a.i2 < b.i2) : a.i1 < b.i1);
	}
};

typedef std::map<int3, int, CompareVid> VidMap;
	// int3 is key, int is value

bool ReadAsciiObj(const char      *filename,
				  vector<vec3>    &points,
				  vector<int3>    &triangles,
				  vector<vec3>    *normals,
				  vector<vec2>    *textures,
				  vector<Group>   *triangleGroups,
				  vector<Mtl>     *triangleMtls,
				  vector<int4>    *quads,
				  vector<int2>	  *segs) {
	// read 'object' file (Alias/Wavefront .obj format); return true if successful;
	// polygons are assumed simple (ie, no holes and not self-intersecting);
	// some file attributes are not supported by this implementation;
	// obj format indexes vertices from 1
	FILE *in = fopen(filename, "r");
	if (!in)
		return false;
	vec2 t;
	vec3 v;
	int group = 0;
	char line[LineLim], word[WordLim];
	bool hashedTriangles = false;	// true if any triangle vertex specified with different point/normal/texture id
	bool hashedVertices = false;	// true if point/normal/texture arrays different (non-zero) size
	vector<vec3> tmpVertices, tmpNormals;
	vector<vec2> tmpTextures;
	VidMap vidMap;
	MtlMap mtlMap;
	for (int lineNum = 0;; lineNum++) {
		line[0] = 0;
		fgets(line, LineLim, in);                           // \ line continuation not supported
		if (feof(in))                                       // hit end of file
			break;
		if (strlen(line) >= LineLim-1) {                    // getline reads LineLim-1 max
			printf("line %d too long\n", lineNum);
			return false;
		}
		line[strlen(line)-1] = 0;							// remove carriage-return
		char *ptr = line;
		if (!ReadWord(ptr, word, WordLim))
			continue;
		Lower(word);
		if (*word == '#') {
			continue;
		}
		else if (!strcmp(word, "mtllib")) {
			if (ReadWord(ptr, word, WordLim)) {
				char name[100];
				const char *p = strrchr(filename, '/'); //-filename, count = 0;
				if (p) {
					int nchars = p-filename;
					strncpy(name, filename, nchars+1);
					name[nchars+1] = 0;
					strcat(name, word);
				}
				else
					strcpy(name, word);
				mtlMap = ReadMaterial(name);
				if (false) {
					int count = 0;
					for (MtlMap::iterator iter = mtlMap.begin(); iter != mtlMap.end(); iter++) {
						string s = (string) iter->first;
						Mtl m = (Mtl) iter->second;
						printf("m[%i].name=%s,.kd=(%3.2f,%3.2f,%3.2f),s=%s\n", count++, m.name.c_str(), m.kd.x, m.kd.y, m.kd.z, s.c_str());
					}
				}					
			}
		}
		else if (!strcmp(word, "usemtl")) {
			if (ReadWord(ptr, word, WordLim)) {
				MtlMap::iterator it = mtlMap.find(string(word));
				if (it == mtlMap.end())
					printf(""); // "no such material: %s\n", word);
				else {
					Mtl m = it->second;
					m.startTriangle = triangles.size();
					if (triangleMtls)
						triangleMtls->push_back(m);
				}
			}
		}
		else if (!strcmp(word, "g")) {
			char *s = strchr(ptr, '(');
			if (s) *s = 0;
			if (triangleGroups) {
			//	printf("adding to triangleGroups: name = %s\n", ptr);
				triangleGroups->push_back(Group(triangles.size(), string(ptr)));
			}
		}
		else if (!strcmp(word, "v")) {                      // read vertex coordinates
			if (sscanf(ptr, "%g%g%g", &v.x, &v.y, &v.z) != 3) {
				printf("bad line %d in object file", lineNum);
				return false;
			}
			tmpVertices.push_back(vec3(v.x, v.y, v.z));
		}
		else if (!strcmp(word, "vn")) {                     // read vertex normal
			if (sscanf(ptr, "%g%g%g", &v.x, &v.y, &v.z) != 3) {
				printf("bad line %d in object file", lineNum);
				return false;
			}
			tmpNormals.push_back(vec3(v.x, v.y, v.z));
		}
		else if (!strcmp(word, "vt")) {                     // read vertex texture
			if (sscanf(ptr, "%g%g", &t.x, &t.y) != 2) {
				printf("bad line in object file");
				return false;
			}
			tmpTextures.push_back(vec2(t.x, t.y));
		}
		else if (!strcmp(word, "f")) {                      // read triangle or polygon
			int nvids = tmpVertices.size(), ntids = tmpTextures.size(), nnids = tmpNormals.size();
			if ((ntids && ntids != nvids) || (nnids && nnids != nvids))
				hashedVertices = true;
			static vector<int> vids;
			vids.resize(0);
			while (ReadWord(ptr, word, WordLim)) {          // read arbitrary # face vid/tid/nid
				// set texture and normal pointers to preceding /
				char *tPtr = strchr(word+1, '/');           // pointer to /, or null if not found
				char *nPtr = tPtr? strchr(tPtr+1, '/') : NULL;
				// use of / is optional (ie, '3' is same as '3/3/3')
				// convert to vid, tid, nid indices (vertex, texture, normal)
				int vid = atoi(word);
				if (!vid)                                   // atoi returns 0 if failure to convert
					break;
				int tid = tPtr && *++tPtr != '/'? atoi(tPtr) : vid;
				int nid = nPtr && *++nPtr != 0? atoi(nPtr) : vid;
				// standard .obj is indexed from 1, mesh indexes from 0
				vid--;
				tid--;
				nid--;
				if (vid < 0 || tid < 0 || nid < 0) {        // atoi conversion failure
					printf("bad format on line %d\n", lineNum);
					break;
				}
				if ((tid >= 0 && tid != vid) || (nid >= 0 && nid != vid))
					hashedTriangles = true;
				bool hashed = hashedVertices || hashedTriangles;
				if (!hashedVertices && !hashedTriangles) {
					vids.push_back(vid);
				}
				if (hashedVertices || hashedTriangles) {
					int3 key(vid, tid, nid);
					VidMap::iterator it = vidMap.find(key);
					// following can fail on early vertices
					// to support OBJ must support triangle vid1/tid1/nid1, vid2/tid2/nid2, vid3/tid3/nid3
					// which would mean changing current implementation
					// instead, test for hashed vertices (ie, has vid ever not equaled tid or nid?)
					// but we add specific test for simple impl
					// need a straightforward implementation for when
					// vid=tid=nid and/or there is no tid, no nid
				//	printf("incoming vid = %i, ", vid);
					if (it == vidMap.end()) {
						int nvrts = points.size();
						vidMap[key] = nvrts;
						points.push_back(tmpVertices[vid]); // *** suspect
						if (normals && (int) tmpNormals.size() > nid)
							normals->push_back(tmpNormals[nid]);
						if (textures && (int) tmpTextures.size() > tid)
							textures->push_back(tmpTextures[tid]);
						vids.push_back(nvrts);
				//		printf("pushed %i\n", nvrts);
					}
					else {
						vids.push_back(it->second);
				//		printf("pushed second = %i\n", it->second);
					}
				}
			}
			int nids = vids.size();
			if (nids == 3) {
				int id1 = vids[0], id2 = vids[1], id3 = vids[2];
				if (normals && (int) normals->size() > id1) {
					vec3 p1, p2, p3;
					if (hashedVertices || hashedTriangles) { p1 = points[id1]; p2 = points[id2]; p3 = points[id3]; }
					else { p1 = tmpVertices[id1]; p2 = tmpVertices[id2]; p3 = tmpVertices[id3]; }
				//	vec3 &p1 = points[id1], &p2 = points[id2], &p3 = points[id3];
					vec3 a(p2-p1), b(p3-p2), n(cross(a, b));
					if (dot(n, (*normals)[id1]) < 0) {
						// reverse triangle order to correspond with vertex normal
						int tmp = id1;
						id1 = id3;
						id3 = tmp;
					}
				}
				// create triangle
				triangles.push_back(int3(id1, id2, id3));
			//	printf("triangle[%i] = (%i, %i, %i)\n", triangles.size()-1, id1+1, id2+1, id3+1);
			}
			else if (nids == 4 && quads)
				quads->push_back(int4(vids[0], vids[1], vids[2], vids[3]));
			else if (nids == 2 && segs)
				segs->push_back(int2(vids[0], vids[1]));
			else
				// create polygon as nvids-2 triangles
				for (int i = 1; i < nids-1; i++) {
					triangles.push_back(int3(vids[0], vids[i], vids[(i+1)%nids]));
				}
		} // end "f"
		else if (*word == 0 || *word == '\n')               // skip blank line
			continue;
		else {                                              // unrecognized attribute
			// printf("unsupported attribute in object file: %s", word);
			continue; // return false;
		}
	} // end read til end of file
//	printf("hashedVertices = %s, hashedTriangles = %s\n", hashedVertices? "true" : "false", hashedTriangles? "true" : "false");
	if (!hashedVertices && !hashedTriangles) {
		int nPoints = tmpVertices.size();
		points.resize(nPoints);
		for (int i = 0; i < nPoints; i++)
			points[i] = tmpVertices[i];
		if (normals) {
			int nNormals = tmpNormals.size();
			normals->resize(nNormals);
			for (int i = 0; i < nNormals; i++)
				(*normals)[i] = tmpNormals[i];
		}
		if (textures) {
			int nTextures = tmpTextures.size();
			textures->resize(nTextures);
			for (int i = 0; i < nTextures; i++)
				(*textures)[i] = tmpTextures[i];
		}
	}
	if (triangleGroups) {
		int nGroups = triangleGroups->size();
		for (int i = 0; i < nGroups; i++) {
			int next = i < nGroups-1? (*triangleGroups)[i+1].startTriangle : triangles.size();
			(*triangleGroups)[i].nTriangles = next-(*triangleGroups)[i].startTriangle;
		}
	}
	if (triangleMtls) {
		int nMtls = triangleMtls->size();
		for (int i = 0; i < nMtls; i++) {
			int next = i < nMtls-1? (*triangleMtls)[i+1].startTriangle : triangles.size();
			(*triangleMtls)[i].nTriangles = next-(*triangleMtls)[i].startTriangle;
		}
	}
	return true;
} // end ReadAsciiObj

bool WriteAsciiObj(const char    *filename,
				   vector<vec3>  &points,
				   vector<vec3>  &normals,
				   vector<vec2>  &uvs,
				   vector<int3>  *triangles,
				   vector<int4>  *quads,
				   vector<int2>  *segs,
				   vector<Group> *triangleGroups) {
	FILE *file = fopen(filename, "w");
	if (!file) {
		printf("can't write %s\n", filename);
		return false;
	}
	int nPoints = points.size(), nNormals = normals.size(), nUvs = uvs.size(), nTriangles = triangles? triangles->size() : 0;
	if (nPoints) {
		fprintf(file, "# %i vertices\n", nPoints);
		for (int i = 0; i < nPoints; i++)
			fprintf(file, "v %f %f %f \n", points[i].x, points[i].y, points[i].z);
		fprintf(file, "\n");
	}
	if (nNormals) {
		fprintf(file, "# %i normals\n", nNormals);
		for (int i = 0; i < nNormals; i++)
			fprintf(file, "vn %f %f %f \n", normals[i].x, normals[i].y, normals[i].z);
		fprintf(file, "\n");
	}
	if (nUvs) {
		fprintf(file, "# %i textures\n", nUvs);
		for (int i = 0; i < nUvs; i++)
			fprintf(file, "vt %f %f \n", uvs[i].x, uvs[i].y);
		fprintf(file, "\n");
	}
	// write triangles, quads (adding 1 to all vertex indices per OBJ format)
	if (triangles) {
		// non-grouped triangles
		size_t nNonGrouped = triangleGroups && triangleGroups->size()? (*triangleGroups)[0].startTriangle : nTriangles; 
		if (nTriangles) fprintf(file, "# %i triangles\n", nTriangles);
		for (size_t i = 0; i < nNonGrouped; i++)
			fprintf(file, "f %d %d %d \n", 1+(*triangles)[i].i1, 1+(*triangles)[i].i2, 1+(*triangles)[i].i3);
		if (triangleGroups)
			for (size_t i = 0; i < triangleGroups->size(); i++) {
				Group g = (*triangleGroups)[i];
				if (g.nTriangles) {
					fprintf(file, "g %s (%i triangles)\n", g.name.c_str(), g.nTriangles);
					for (int t = g.startTriangle; t < g.startTriangle+g.nTriangles; t++)
						fprintf(file, "f %d %d %d \n", 1+(*triangles)[t].i1, 1+(*triangles)[t].i2, 1+(*triangles)[t].i3);
				}
			}
	//	else
	//		for (size_t i = 0; i < triangles->size(); i++)
	//			fprintf(file, "f %d %d %d \n", 1+(*triangles)[i].i1, 1+(*triangles)[i].i2, 1+(*triangles)[i].i3);
		fprintf(file, "\n");
	}
	if (quads)
		for (size_t i = 0; i < quads->size(); i++)
			fprintf(file, "f %d %d %d %d \n", 1+(*quads)[i].i1, 1+(*quads)[i].i2, 1+(*quads)[i].i3, 1+(*quads)[i].i4);
	if (segs)
		for (size_t i = 0; i < segs->size(); i++)
			fprintf(file, "f %d %d \n", 1+(*segs)[i].i1, 1+(*segs)[i].i2);
	fclose(file);
	return true;
}

// OLD shaders

const char *OLDmeshVertexShader = R"(
	#version 330
	layout (location = 0) in vec3 point;
	layout (location = 1) in vec3 normal;
	layout (location = 2) in vec2 uv;
	layout (location = 3) in mat4 instance; // for use with glDrawArrays/ElementsInstanced
											// uses locations 3,4,5,6 for 4 vec4s = mat4
	layout (location = 7) in vec3 color;	// for instanced color (vec4?)
	out vec3 vPoint;
	out vec3 vNormal;
	out vec2 vUv;
	out vec3 vColor;
	uniform bool useInstance = false;
	uniform mat4 modelview;
	uniform mat4 persp;
	void main() {
		mat4 m = useInstance? modelview*instance : modelview;
		vPoint = (m*vec4(point, 1)).xyz;
		vNormal = (m*vec4(normal, 0)).xyz;
		gl_Position = persp*vec4(vPoint, 1);
		vUv = uv;
		vColor = color;
	}
)";

const char *OLDmeshPixelShader = R"(
	#version 330
	in vec3 vPoint;
	in vec3 vNormal;
	in vec2 vUv;
	in vec3 vColor;
	out vec4 pColor;
	uniform int nLights = 1;
	uniform vec3 lights[20] = vec3[20](vec3(1, 1, 1));
	uniform bool useLight = true;
	uniform sampler2D textureUnit;
	uniform vec3 defaultColor = vec3(1);
	uniform bool useDefaultColor = true;
	uniform float opacity = 1;
	uniform bool useTexture = false;
	uniform bool useTint = false;
	uniform bool fwdFacing = false;
	uniform bool facetedShading = false;
	float Intensity(vec3 normalV, vec3 eyeV, vec3 point, vec3 light) {
		vec3 lightV = normalize(light-point);		// light vector
		vec3 reflectV = reflect(lightV, normalV);   // highlight vector
		float d = max(0, dot(normalV, lightV));     // one-sided diffuse
		float s = max(0, dot(reflectV, eyeV));      // one-sided specular
		return clamp(d+pow(s, 50), 0, 1);
	}
	void main() {
		vec3 N = normalize(facetedShading? cross(dFdx(vPoint), dFdy(vPoint)) : vNormal);
		if (fwdFacing && N.z < 0) discard;
		vec3 E = normalize(vPoint);					// eye vector
		float intensity = useLight? 0 : 1;
		if (useLight) {
			for (int i = 0; i < nLights; i++)
				intensity += Intensity(N, E, vPoint, lights[i]);
			intensity = clamp(intensity, 0, 1);
		}
		vec3 color = useTexture? texture(textureUnit, vUv).rgb : useDefaultColor? defaultColor : vColor;
		if (useTexture && useTint) {
			color.r *= defaultColor.r;
			color.g *= defaultColor.g;
			color.b *= defaultColor.b;
		}
		pColor = vec4(intensity*color, opacity);
	}
)";
