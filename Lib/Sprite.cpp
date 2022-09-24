// Sprite.cpp

#include "Draw.h"
#include "GLXtras.h"
#include "Misc.h"
#include "Sprite.h"
#include <algorithm>
#include <iostream>

// Shader Storage Buffers for Collision Tests
GLuint occupyBinding = 11, collideBinding = 12;
GLuint occupyBuffer = 0, collideBuffer = 0;

// Shaders
GLuint spriteShader = 0, spriteCollisionShader;

namespace SpriteSpace {

int BuildShader(bool collisionTest = false) {
	const char *vShaderQ = R"(
		#version 330
		uniform mat4 view;
		uniform float z = 0;
		out vec2 uv;
		void main() {
			vec2 pts[] = vec2[4](vec2(-1,-1), vec2(-1,1), vec2(1,1), vec2(1,-1));
			uv = (vec2(1,1)+pts[gl_VertexID])/2;
			gl_Position = view*vec4(pts[gl_VertexID], z, 1);
		}
	)";
	const char *vShaderT = R"(
		#version 330
		uniform mat4 view;
		uniform float z = 0;
		out vec2 uv;
		void main() {
			vec2 pts[] = vec2[6](vec2(-1,-1), vec2(-1,1), vec2(1,1), vec2(-1,-1), vec2(1,1), vec2(1,-1));
			uv = (vec2(1,1)+pts[gl_VertexID])/2;
			gl_Position = view*vec4(pts[gl_VertexID], z, 1);
		}
	)";
	#ifdef GL_QUADS
		const char *vShader = vShaderQ;
	#else
		const char *vShader = vShaderT;
	#endif
	const char *pShader = R"(
		#version 330
		in vec2 uv;
		out vec4 pColor;
		uniform mat4 uvTransform;
		uniform sampler2D textureImage;
		uniform sampler2D textureMat;
		uniform bool useMat;
		uniform int nTexChannels = 3;
		void main() {
			vec2 st = (uvTransform*vec4(uv, 0, 1)).xy;
			if (nTexChannels == 4)
				pColor = texture(textureImage, st);
			else {
				pColor.rgb = texture(textureImage, st).rgb;
				pColor.a = useMat? texture(textureMat, st).r : 1;
			}
			if (pColor.a < .02) // if nearly full matte,
				discard;		// don't tag z-buffer
		}
	)";
	const char *pCollisionShader = R"(
		#version 430
		layout(binding = 11, std430) buffer Occupy  { int occupy[]; };		// set occupy[x][y] to sprite id
		layout(binding = 12, std430) buffer Collide { int collide[]; };		// does spriteId collide with spriteN?
		layout(binding = 0, r32ui) uniform uimage1D atomicCollide;			// does spirteId collide with spriteN?
		layout(binding = 0, offset = 0) uniform atomic_uint counter;
		in vec2 uv;
		out vec4 pColor;
		uniform vec4 vp;
		uniform bool showOccupy = false;
		uniform int spriteId = 0;
		uniform mat4 uvTransform;
		uniform sampler2D textureImage;
		uniform sampler2D textureMat;
		uniform bool useMat = false;
		uniform int nTexChannels = 3;
		void main() {
			vec2 st = (uvTransform*vec4(uv, 0, 1)).xy;
			if (nTexChannels == 4)
				pColor = texture(textureImage, st);
			else {
				pColor.rgb = texture(textureImage, st).rgb;
				pColor.a = useMat? texture(textureMat, st).r : 1;
			}
			if (pColor.a < .02) // if nearly full matte, don't tag z-buffer
				discard;
			if (pColor.a >= .02) {
				vec3 cols[] = vec3[](vec3(1,0,0),vec3(1,1,0),vec3(0,1,0),vec3(0,0,1));
				int id = int((gl_FragCoord.y-vp[1])*vp[2]+gl_FragCoord.x-vp[0]);
				int o = occupy[id];
				if (o > -1) {
					collide[o] = 1;
					atomicCounterIncrement(counter);
					if (showOccupy)
						pColor = vec4(cols[o], 1);
				}
				occupy[id] = spriteId;
			}
		}
	)";
	return LinkProgramViaCode(&vShader, collisionTest? &pCollisionShader : &pShader);
}

GLuint GetShader() {
	if (!spriteShader)
		spriteShader = BuildShader();
	return spriteShader;
}

GLuint GetCollisionShader() {
	if (!spriteCollisionShader)
		spriteCollisionShader = BuildShader(true);
	return spriteCollisionShader;
}

bool CrossPositive(vec2 a, vec2 b, vec2 c) { return cross(vec2(b-a), vec2(c-b)) > 0; }

} // end namespace

// Collision

vector<int> clearOccupy, clearCollide;
int nCollisionSprites = 0;
GLuint countersBuf = 0, atomicCollideBuffer = 0;

void ResetCounter() {
	GLuint count = 0;
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, countersBuf);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, countersBuf);
	glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &count);
}

int ReadCounter() {
	GLuint count = 0;
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, countersBuf);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, countersBuf);
	glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &count);
	return count;
}

void InitCollisionShaderStorage(int nsprites) {
	vec4 vp = VP();
	int w = (int) vp[2], h = (int) vp[3];
	// occupancy buffer
	clearOccupy.assign(w*h, -1);
	glGenBuffers(1, &occupyBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, occupyBinding, occupyBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, w*h*sizeof(int), clearOccupy.data(), GL_DYNAMIC_DRAW);
	// collision buffer
	clearCollide.assign(nsprites, -1);
	glGenBuffers(1, &collideBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, collideBinding, collideBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, nsprites*sizeof(int), clearCollide.data(), GL_DYNAMIC_DRAW);
	// atomic counters
	glGenBuffers(1, &countersBuf);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, countersBuf);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, countersBuf);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, 0);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}

void ClearCollide() {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, collideBinding, collideBuffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, clearCollide.size()*sizeof(int), clearCollide.data());
}

void ClearOccupyAndCounter(int nsprites) {
	int w = VPw(), h = VPh();
	// occupancy buffer
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, occupyBinding, occupyBuffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, w*h*sizeof(int), clearOccupy.data());
	// collision buffer
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, collideBinding, collideBuffer);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, nsprites*sizeof(int), clearCollide.data());
	// atomic counter
	ResetCounter();
}

void GetCollided(int nsprites, Sprite *s) {
	if ((int) s->collided.size() < nsprites)
		s->collided.resize(nsprites);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, nsprites*sizeof(int), s->collided.data());
}

bool ZCompare(Sprite *s1, Sprite *s2) { return s1->z > s2->z; }

int TestCollisions(vector<Sprite *> &sprites) {
	int nsprites = sprites.size();
	if (nsprites != nCollisionSprites) {
		nCollisionSprites = nsprites;
		InitCollisionShaderStorage(nsprites);
	}
	else
		ClearOccupyAndCounter(nsprites);
	vector<Sprite *> tmp = sprites;
	for (int i = 0; i < nsprites; i++)
		tmp[i]->id = i;
	sort(tmp.begin(), tmp.end(), ZCompare);
	GLuint program = SpriteSpace::GetCollisionShader();
	glUseProgram(program);
	SetUniform(program, "vp", VP());
	SetUniform(program, "showOccupy", true);
	// display in descending z order
	for (int i = 0; i < nsprites; i++) {
		Sprite *s = tmp[i];
		SetUniform(program, "spriteId", s->id);
		ClearCollide();
		s->Display();
		GetCollided(nsprites, s);
	}
	SetUniform(program, "showOccupy", false);
	UseDrawShader(ScreenMode());
	glUseProgram(0);
	return ReadCounter();
}

bool Sprite::Intersect(Sprite &s) {
	vec2 pts[] = { {-1,-1}, {-1,1}, {1,1}, {1,-1} };
	float x1min = FLT_MAX, x1max = -FLT_MAX, y1min = FLT_MAX, y1max = -FLT_MAX;
	float x2min = FLT_MAX, x2max = -FLT_MAX, y2min = FLT_MAX, y2max = -FLT_MAX;
	for (int i = 0; i < 4; i++) {
		vec2 p1 = PtTransform(pts[i]);
		vec2 p2 = s.PtTransform(pts[i]);
		x1min = min(x1min, p1.x); x1max = max(x1max, p1.x);
		y1min = min(y1min, p1.y); y1max = max(y1max, p1.y);
		x2min = min(x2min, p2.x); x2max = max(x2max, p2.x);
		y2min = min(y2min, p2.y); y2max = max(y2max, p2.y);
	}
	bool xNoOverlap = x1min > x2max || x2min > x1max;
	bool yNoOverlap = y1min > y2max || y2min > y1max;
	return !xNoOverlap && !yNoOverlap;
}

void Sprite::Initialize(GLuint texName, float z) {
	this->z = z;
	textureName = texName;
}

void Sprite::Initialize(string imageFile, float z) {
	this->z = z;
	textureName = LoadTexture(imageFile.c_str(), true, &nTexChannels, &imgWidth, &imgHeight);
}

void Sprite::Initialize(string imageFile, string matFile, float z) {
	Initialize(imageFile, z);
	matName = LoadTexture(matFile.c_str());
}

void Sprite::Initialize(vector<string> &imageFiles, string matFile, float z) {
	this->z = z;
	nFrames = imageFiles.size();
	textureNames.resize(nFrames);
	for (size_t i = 0; i < nFrames; i++)
		textureNames[i] = LoadTexture(imageFiles[i].c_str());
	if (!matFile.empty())
		matName = LoadTexture(matFile.c_str());
	change = clock()+(time_t)(frameDuration*CLOCKS_PER_SEC);
}

bool Sprite::Hit(int x, int y) {
	// test against z-buffer
	float depth;
	if (DepthXY((int) x, (int) y, depth))
		return abs(depth-z) < .01;
	// test if inside quad
	GetViewportSize(winWidth, winHeight);
	vec2 pts[] = { {-1,-1}, {-1,1}, {1,1}, {1,-1} }, xPts[4];
	vec2 test((2.f*x)/winWidth-1, (2.f*y)/winHeight-1);
	for (int i = 0; i < 4; i++)
		xPts[i] = PtTransform(pts[i]);
	for (int i = 0; i < 4; i++)
		if (SpriteSpace::CrossPositive(test, xPts[i], xPts[(i+1)%4]))
			return false;
	return true;
}

void Sprite::SetPosition(vec2 p) { position = p; UpdateTransform(); }

vec2 Sprite::GetPosition() { return position; }

void Sprite::UpdateTransform() {
	ptTransform = Translate(position.x, position.y, 0)*Scale(scale.x, scale.y, 1)*RotateZ(rotation);
}

vec2 Sprite::PtTransform(vec2 p) {
	vec4 q = ptTransform*vec4(p, 0, 1);
	return vec2(q.x, q.y); // q.x/q.w, q.y/q.w
}

void Sprite::MouseDown(vec2 mouse) {
	oldMouse = position;
	GetViewportSize(winWidth, winHeight);
	mouseDown = mouse;
}

vec2 Sprite::MouseDrag(vec2 mouse) {
	vec2 dif(mouse-mouseDown), difScale(2*dif.x/winWidth, 2*dif.y/winHeight);
	SetPosition(oldMouse+difScale);
	return difScale;
}

void Sprite::MouseWheel(double spin) {
	scale += .1f*(float) spin;
	scale.x = scale.x < FLT_MIN? FLT_MIN : scale.x;
	scale.y = scale.y < FLT_MIN? FLT_MIN : scale.y;
	UpdateTransform();
}

vec2 Sprite::GetScale() { return scale; }

void Sprite::SetScale(vec2 s) {
	scale = s;
	UpdateTransform();
}

mat4 Sprite::GetPtTransform() { return ptTransform; }

void Sprite::SetPtTransform(mat4 m) { ptTransform = m; }

void Sprite::SetUvTransform(mat4 m) { uvTransform = m; }

int GetSpriteShader() {
	if (!spriteShader)
		SpriteSpace::BuildShader();
	return spriteShader;
}

void Sprite::Display(mat4 *fullview, int textureUnit) {
	int s = CurrentProgram();
	if (s <= 0 || (s != spriteShader && s != spriteCollisionShader))
		s = SpriteSpace::GetShader();
	glUseProgram(s);
	glActiveTexture(GL_TEXTURE0+textureUnit);
	if (nFrames) { // animation
		time_t now = clock();
		if (now > change) {
			frame = (frame+1)%nFrames;
			change = now+(time_t)(frameDuration*CLOCKS_PER_SEC);
		}
		glBindTexture(GL_TEXTURE_2D, textureNames[frame]);
	}
	else glBindTexture(GL_TEXTURE_2D, textureName);
	SetUniform(s, "textureImage", (int) textureUnit);
	SetUniform(s, "useMat", matName > 0);
	SetUniform(s, "nTexChannels", nTexChannels);
	SetUniform(s, "z", z);
	if (matName > 0) {
		glActiveTexture(GL_TEXTURE0+textureUnit+1);
		glBindTexture(GL_TEXTURE_2D, matName);
		SetUniform(s, "textureMat", (int) textureUnit+1);
	}
	SetUniform(s, "view", fullview? *fullview*ptTransform : ptTransform);
	SetUniform(s, "uvTransform", uvTransform);
#ifdef GL_QUADS
	glDrawArrays(GL_QUADS, 0, 4);
#else
	glDrawArrays(GL_TRIANGLES, 0, 6);
#endif
}

void Sprite::SetFrameDuration(float dt) { frameDuration = dt; }

void Sprite::Release() {
	glDeleteBuffers(1, &textureName);
	if (matName > 0)
		glDeleteBuffers(1, &matName);
}
