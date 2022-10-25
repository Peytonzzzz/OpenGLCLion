// Widgets.cpp (c) 2019-2022 Jules Bloomenthal

#include <glad.h>
#include <float.h>
#include "Draw.h"
#include "GLXtras.h"
#include "Misc.h"
#include "Widgets-Mac.h"

// Cursor/Mouse/Keyboard

bool KeyDown(GLFWwindow *w, int button) { return glfwGetKey(w, button) == GLFW_PRESS; }

bool Shift(GLFWwindow *w) { return KeyDown(w, GLFW_KEY_LEFT_SHIFT) || KeyDown(w, GLFW_KEY_RIGHT_SHIFT); }

bool Control(GLFWwindow *w) { return KeyDown(w, GLFW_KEY_LEFT_CONTROL) || KeyDown(w, GLFW_KEY_RIGHT_CONTROL); }

vec3 *MouseOver(vector<vec3> &points, int x, int y, mat4 fullview, bool invertVertical) {
	for (size_t i = 0; i < points.size(); i++)
		if (::MouseOver(x, y, points[i], fullview, 12, 0, 0, invertVertical))
			return &points[i];
	return NULL;
}

bool MouseOver(double x, double y, vec2 p, int proximity, int xCursorOffset, int yCursorOffset, bool invertVertical) {
	if (invertVertical) y = VPh()-y;
	float f = length(vec2((float)(x+xCursorOffset), (float)(y+yCursorOffset))-p);
	return f < proximity;
}

bool MouseOver(int x, int y, vec2 p, int proximity, int xCursorOffset, int yCursorOffset, bool invertVertical) {
	if (invertVertical) y = VPh()-y;
	float xo = static_cast<float>(x+xCursorOffset), yo = static_cast<float>(y+yCursorOffset);
	return length(vec2(xo, yo)-p) < proximity;
}

bool MouseOver(double x, double y, vec3 p, mat4 &view, int proximity, int xCursorOffset, int yCursorOffset, bool invertVertical) {
	return ScreenDSq(x+xCursorOffset, y+yCursorOffset, p, view, NULL, invertVertical) < proximity*proximity;
}

// Matrix Support

vec3 Vec3(vec4 v) { return vec3(v.x, v.y, v.z); }

int TransformArray(vec3 *in, vec3 *out, int n, mat4 m) {
	for (int i = 0; i < n; i++)
		out[i] = Vec3(m*vec4(in[i]));
	return n;
}

int TransformArray(vector<vec3> &vecs, vector<vec3> &xvecs, mat4 m) {
	xvecs.resize(vecs.size());
	return TransformArray(vecs.data(), xvecs.data(), vecs.size(), m);
}

vec3 MatrixOrigin(mat4 f) { return vec3(f[0][3], f[1][3], f[2][3]); }

float MatrixScale(mat4 f) { return length(vec3(f[0][0], f[1][0], f[2][0])); }

void SetMatrixOrigin(mat4 &f, vec3 base) {
	for (int i = 0; i < 3; i++)
		f[i][3] = base[i];
}

void Scale3x3(mat4 &f, float scale) {
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			f[i][j] *= scale;
}

// Mover

void SetPlane(vec3 p, mat4 modelview, mat4 persp, float *plane) {
	for (int i = 0; i < 3; i++)
		plane[i] = modelview[2][i];  // zrow (which is product of modelview matrix with untransformed z-axis)
	plane[3] = -plane[0]*p.x-plane[1]*p.y-plane[2]*p.z; // pass plane through point
}

void Mover::Down(vec3 *p, int x, int y, mat4 modelview, mat4 persp) {
	mat4 inv = Invert(modelview); // probably could do this by separating into rot and tran matrices
	cameraPosition = vec3(inv[0][3], inv[1][3], inv[2][3]);
	vec2 s = ScreenPoint(*p, persp*modelview);
	y = VPh()-y; ///////////**********
	mouseOffset = vec2(s.x-x, s.y-y);
	point = p;
	pMousedown = *p;
	transform = NULL;
	SetPlane(*p, modelview, persp, plane);
}

void Mover::Down(mat4 *t, int x, int y, mat4 modelview, mat4 persp) {
	mat4 inv = Invert(modelview); // probably could do this by separating into rot and tran matrices
	cameraPosition = vec3(inv[0][3], inv[1][3], inv[2][3]);
	vec3 p((*t)[0][3], (*t)[1][3], (*t)[2][3]);
	vec2 s = ScreenPoint(p, persp*modelview);
	y = VPh()-y; ///////////**********
	mouseOffset = vec2(s.x-x, s.y-y);
	transform = t;
	point = NULL;
	pMousedown = MatrixOrigin(*t);
	SetPlane(p, modelview, persp, plane);
}

vec3 Mover::Drag(int xMouse, int yMouse, mat4 modelview, mat4 persp) {
	vec3 dPoint;
	if (point || transform) {
		vec3 p1, p2, axis;
		yMouse = VPh()-yMouse;
		float x = xMouse+mouseOffset.x, y = yMouse+mouseOffset.y;
		ScreenLine((float) x, (float) y, modelview, persp, p1, p2);
		// get two points that transform to pixel x,y
		axis = p2-p1;
		// direction of line through p1
		float pdDot = dot(axis, plane);
		// project onto plane normal
		float a = (-plane[3]-dot(p1, plane))/pdDot;
		// intersection of line with plane
		vec3 p = p1+a*axis;
		if (point) *point = p;
		if (transform) { (*transform)[0][3] = p.x; (*transform)[1][3] = p.y; (*transform)[2][3] = p.z; }
		dPoint = p-pMousedown;
	}
	return dPoint;
}

void  Mover::Wheel(double spin) {
	if (point || transform) {
		vec3 p = point? *point : vec3((*transform)[0][3], (*transform)[1][3], (*transform)[2][3]);
		vec3 dif = .02f*normalize(cameraPosition-p);
		p += .02f*(float)spin*normalize(cameraPosition-p);
		if (point) *point = p;
		if (transform) { (*transform)[0][3] = p.x; (*transform)[1][3] = p.y; (*transform)[2][3] = p.z; }
	}
}

bool Mover::Hit(int x, int y, mat4 &view, int proximity) {
	return MouseOver((float) x, (float) y, *point, view, proximity);
}

bool Mover::IsSet(void *p) { return p? point == p : point != NULL; }

void Mover::Unset() { point = NULL; }

Mover::Mover() { for (int i = 0; i < 4; i++) plane[i] = 0; }

// Framer

Framer::Framer() { Set(NULL, 0, mat4(1)); }

Framer::Framer(mat4 *m, float radius, mat4 fullview) { Set(m, radius, fullview); }

void Framer::Set(mat4 *m, float radius, mat4 fullview) {
	vec2 s(0, 0);
	if (m) {
		base = MatrixOrigin(*m);
		s = ScreenPoint(base, fullview);
	}
	arcball.SetBody(m, radius);
	arcball.SetCenter(s);
	moverPicked = false;
}

bool Framer::Hit(int x, int y) { return arcball.Hit(x, y); }

void Framer::Down(int x, int y, mat4 modelview, mat4 persp, bool control) {
	moverPicked = MouseOver(x, y, arcball.center);
	if (moverPicked)
		mover.Down(&base, x, y, modelview, persp);
	else
		arcball.Down(x, y, control);
}

void Framer::Drag(int x, int y, mat4 modelview, mat4 persp) {
	if (moverPicked) {
		mover.Drag(x, y, modelview, persp);
		SetMatrixOrigin(*arcball.m, base);
		arcball.SetCenter(ScreenPoint(base, persp*modelview));
	}
	else
		arcball.Drag(x, y);
}

void Framer::Up() { arcball.Up(); }

void Framer::Wheel(double spin, bool shift) { arcball.Wheel(spin, shift); }

void Framer::Draw(mat4 fullview, bool showConstrainAxes) {
	if (arcball.m) {
		UseDrawShader(ScreenMode());
		arcball.Draw(showConstrainAxes);
		// draw center
		UseDrawShader(fullview);
		Disk(base, 9, vec3(1, .2f, .8f)); // arcball pink
	}
}

mat4 *Framer::GetMatrix() { return arcball.m; }

// Arcball

void Arcball::SetBody(mat4 *mat, float r) {
	// set Body arcball, set scale from *mat, on Drag: update *m
	use = Use::Body;
	m = mat;
	if (m)
		scale = MatrixScale(*m);
	radius = r;
}

void Arcball::SetBody(mat4 mat, float r) {
	// set Body arcball, set scale from mat, Drag does not update m
	use = Use::Body;
	radius = r;
	scale = MatrixScale(mat);
}

void Arcball::SetCamera(mat4 *mat, vec2 c, float r) {
	// set Camera arcball with given center, radius
	use = Use::Camera;
	m = mat;
	scale = MatrixScale(*m);
	radius = r;
	center = c;
}

void Arcball::SetCenter(vec2 c) {
	center = c;
}

void Arcball::SetCenter(vec2 c, float r) {
	center = c;
	radius = r;
}

bool Arcball::MouseOver(int x, int y) {
	return ::MouseOver(x, y, center);
}

bool Arcball::Hit(int x, int y) {
	vec2 dif(x-center.x, y-center.y);
	return dot(dif, dif) < radius*radius;
}

vec3 Arcball::BallV(vec2 mouse) {
	// return point on sphere centered on origin with radius in pixels
	// constrain to circumference if mouse exceeds radius
	vec2 dif(mouse-center);
	float difLen = length(dif);
	if (difLen > .97f*radius)
		dif *= (.97f*radius/difLen);
	float sq = radius*radius-dot(dif, dif);
	vec3 v(dif.x, dif.y, sqrt(sq)); // in pixels
	return normalize(v);
}

bool Normalize(vec3 &v) {
	float l = length(v);
	if (l < FLT_MIN) return false;
	v /= l;
	return true;
}

vec3 Arcball::ConstrainToAxis(vec3 loose, vec3 axis) {
	// see Arcball.html by Ken Shoemake
	// force sphere point to be perpendicular to axis, on unit sphere
	float d = dot(axis, loose);
	vec3 p = loose-d*axis;						// on plane defined by axis
	if (Normalize(p)) return p.z > 0? p : -p;	// on unit sphere
	vec3 pp(-axis.y, axis.x, 0);
	if (Normalize(pp)) return pp;
	return vec3(1, 0, 0);
};

Arcball::Constraint Arcball::GetConstraint(int mousex, int mousey, mat4 *m) {
	Constraint c = { ConstraintIndex::None, vec3() };
	vec2 mouse((float) mousex, (float) mousey);
	if (!m) { // use == Camera
		vec2 dif(mouse-center);
		float dx = abs(dif.x), dy = abs(dif.y), dr = abs(length(dif)-radius);
		if (dx < dy && dx < dr) c = { ConstraintIndex::XAxis, vec3(1, 0, 0) }; // nearest vertical axis
		if (dy < dx && dy < dr) c = { ConstraintIndex::YAxis, vec3(0, 1, 0) }; // nearest horiz axis
		if (dr < dx && dr < dy) c = { ConstraintIndex::ZAxis, vec3(0, 0, 1) }; // nearest circle
	
	}
	if (m) { // use == Body
		vec3 p = BallV(mouse);
		float dotMax = -1;
		for (int i = 0; i < 3; i++) {
			vec3 axis = normalize(vec3((*m)[0][i], (*m)[1][i], (*m)[2][i]));
			vec3 onPlane = ConstrainToAxis(p, axis);
			float d = abs(dot(onPlane, p));
			if (d > dotMax) {
				dotMax = d;
				c = { (ConstraintIndex) i, axis };
			}
		}
	}
	return c;
};

void Arcball::SetNearestAxis(int mousex, int mousey, mat4 *downMat) {
	constraint = GetConstraint(mousex, mousey, downMat);
 };

void Arcball::Down(int x, int y, bool constrain, mat4 *mOverride) {
	constraint = { ConstraintIndex::None, vec3() };
	mouseDown = mouseMove = vec2((float) x, (float) y);
	mat4 *mat = mOverride? mOverride : m;
	if (mat) {
		qstart = Quaternion(*mat);
		scale = MatrixScale(*mat);
		if (constrain)
			constraint = GetConstraint(x, y, use == Use::Camera? NULL : mat);
	}
}

void Arcball::Up() {
	mouseDown = mouseMove = vec2(0, 0);
	constraint = { ConstraintIndex::None, vec3() };
	dragging = false;
}

Quaternion Arcball::Drag(int x, int y) {
	dragging = true;
	mouseMove = vec2((float) x, (float) y);
	vec3 v1 = BallV(mouseDown), v2 = BallV(mouseMove);
	if (constraint.id != ConstraintIndex::None) {
		v1 = ConstrainToAxis(v1, constraint.axis);
		v2 = ConstrainToAxis(v2, constraint.axis);
	}
	Quaternion qrot;
	vec3 axis = cross(v2, v1);
	if (dot(axis, axis) > .000001f) {
		qrot = Quaternion(axis, (float) acos((double) dot(v1, v2)));
		qq = use == Use::Camera? qstart*qrot : qrot*qstart;
		mat3 m3 = scale*qq.Get3x3();
		if (m != NULL)
			for (int i = 0; i < 3; i++)
				for (int j = 0; j < 3; j++)
					(*m)[i][j] = m3[i][j];
	}
	return qrot;
}

void Arcball::Wheel(double spin, bool shift) {
	scale *= (spin > 0? 1.01f : .99f);
	Scale3x3(*m, scale/MatrixScale(*m));
}

mat4 *Arcball::GetMatrix() { return m; }

Quaternion Arcball::GetQ() { return qq; }

void Arcball::Draw(bool showConstrainAxes, mat4 *matOverride) {
	mat4 *mm = matOverride? matOverride : m;
	if (mm) {
		UseDrawShader(ScreenMode());
		vec3 v1 = BallV(mouseDown), v2 = BallV(mouseMove);
		vec3 s1, p1, mauve(1, .2f, .8f), yellow(1, 1, 0), red(1, 0, 0), blue(0, 0, 1);
		bool showConstrain = showConstrainAxes && constraint.id != ConstraintIndex::None;
		if (length(mouseDown-mouseMove) > 2)
			// draw arc
			for (int i = 0; i < 24; i++) {
				vec3 q = v1+((float)i/23)*(v2-v1);
				vec3 v = radius*normalize(q);
				vec3 s2(center.x+v.x, center.y+v.y, 0);
				if (i > 0) Line(s1, s2, 2, mauve);
				s1 = s2;
			}
		// else should draw arc projected onto contraint.axis
		if (showConstrainAxes) {
			Constraint c = constraint.id == ConstraintIndex::None? GetConstraint((int) mouseMove.x, (int) mouseMove.y, matOverride) : constraint;
			if (use == Use::Camera) {
				if (!dragging || c.id == ConstraintIndex::YAxis)
					Line(vec2(center.x-radius, center.y), vec2(center.x+radius, center.y), 2, c.id == ConstraintIndex::YAxis? yellow : mauve);
				if (!dragging || c.id == ConstraintIndex::XAxis)
					Line(vec2(center.x, center.y-radius), vec2(center.x, center.y+radius), 2, c.id == ConstraintIndex::XAxis? yellow : mauve);
			}
			if (use == Use::Body) {
				for (int i = 0; i < 3; i++) {
						vec3 col = (ConstraintIndex) i == c.id? yellow : mauve;
						vec3 axis = normalize(vec3((*mm)[0][i], (*mm)[1][i], (*mm)[2][i]));
						vec3 v1(axis.y, -axis.x, 0);
						if (!Normalize(v1)) v1 = vec3(0, 1, 0);
						vec3 v2 = cross(v1, axis);
						vec2 p1, p2;
						for (int k = 0; k < 10; k++) {
							float t = (float) k / 19, a = t*2*3.1415f;
							vec3 v = cos(a)*v1+sin(a)*v2;
							p2 = center+radius*vec2(v.x, v.y);
							if (k > 0) Line(p1, p2, 2, col);
							p1 = p2;
						}
					}
			}
		}
		// outer circle
		vec3 col = use == Use::Camera && showConstrainAxes && constraint.id == ConstraintIndex::ZAxis? yellow : mauve;
		for (int i = 0; i < 36; i++) {
			float ang = 2*3.141592f*((float)i/35);
			vec3 p2(center.x+radius*cos(ang), center.y+radius*sin(ang), 0);
			if (i > 0) Line(p1, p2, 2, col);
			p1 = p2;
		}
	}
}

// Joystick

bool Nil(float f) { return f > -FLT_EPSILON && f < FLT_EPSILON; }

vec3 ProjectToLine(vec3 &p, vec3 &p1, vec3 &p2) {
	// project p to line p1p2
	vec3 delta(p2-p1);
	if (Nil(delta.x) && Nil(delta.y) && Nil(delta.z))
		return p1;
	vec3 dif(p-p1);
	float alpha = dot(delta, dif)/dot(delta, delta);
	return p1+alpha*delta;
}

Joystick::Joystick(vec3 *b, vec3 *v, float arrowScale, vec3 color) :
	base(b), vec(v), arrowScale(arrowScale), color(color) {
		for (int i = 0; i < 4; i++)
			plane[i] = 0;
}

bool Joystick::Hit(int x, int y, vec3 b,  vec3 v, mat4 fullview) {
	return ScreenDSq(x, y, b, fullview) < 100 ||
		   ScreenDSq(x, y, b+v, fullview) < 100;
}

bool Joystick::Hit(int x, int y, mat4 fullview) {
	return // ScreenDistSq(x, y, *base, fullview) < 100 ||
		   ScreenDSq(x, y, *base+*vec, fullview) < 100;
}

void Joystick::Down(int x, int y, vec3 *b, vec3 *v, mat4 modelview, mat4 persp) {
	mat4 fullview = persp*modelview;
	base = b;
	vec = v;
	fwdFace = FrontFacing(*base, *vec, fullview);
	mode = ScreenDSq(x, y, *base, fullview) < 100? JoyType::A_Base :
		   ScreenDSq(x, y, *base+*vec, fullview) < 100? JoyType::A_Tip : JoyType::A_None;
	if (mode == JoyType::A_Base)
		SetPlane(*base, modelview, persp, plane);
}

void Joystick::Drag(int x, int y, mat4 modelview, mat4 persp) {
	vec3 p1, p2;                                        // p1p2 is world-space line that xforms to line perp to screen at (x, y)
	ScreenLine((float) x, (float) y, modelview, persp, p1, p2);
	if (mode == JoyType::A_Base) {
		vec3 axis(p2-p1);                               // direction of line through p1
		vec3 normal(plane[0], plane[1], plane[2]);
		float pdDot = dot(axis, normal);
		float a = (-plane[3]-dot(p1, normal))/pdDot;    // project onto plane normal
		*base = p1+a*axis;                              // intersection of line with plane
	}
	if (mode == JoyType::A_Tip) {
		float len = length(*vec);
		vec3 hit1, hit2, v;
		int nhits = LineSphere(p1, p2, *base, len, hit1, hit2);
		if (nhits == 0) {
			vec3 n = ProjectToLine(*base, p1, p2);
			v = normalize(n-*base);
		}
		else {
			v = hit2-*base;
			if (FrontFacing(*base, v, modelview) != fwdFace)
				v = hit1-*base;
		}
		*vec = len*normalize(v);
	}
}

void Joystick::Draw(vec3 color, mat4 modelview, mat4 persp) {
//  bool frontFacing = FrontFacing(*base, *vec, modelview);
	UseDrawShader(persp*modelview);
#ifdef GL_LINE_STIPPLE
	if (!frontFacing) {
		glEnable(GL_LINE_STIPPLE);
		glLineStipple(6, 0xAAAA);
	}
#endif
//  ArrowV(base, len*v, modelview, color, NULL, 10);
	Line(*base, *base+*vec, 5, color);
#ifdef GL_LINE_STIPPLE
	glDisable(GL_LINE_STIPPLE);
#endif
	Disk(*base, 12, vec3(1,0,1));
	Disk(*base+*vec, 12, vec3(1,0,1));
}

void Joystick::SetVector(vec3 v) { *vec = v; }

void Joystick::SetBase(vec3 b) { *base = b; }

// Rectangular Push-Button

Button::Button(const char *cname, int x, int y, int w, int h, vec3 col)
	: x(x), y(y), w(w), h(h), color(col)
{
		if (cname) name = string(cname);
};

void Button::Draw(const char *nameOverride, float textSize, vec3 *colorOverride, vec3 textColor) {
	// assume ScreenMode and no depth-test
	const char *s = nameOverride? nameOverride : name.c_str();
	int nchars = strlen(s), npixels = int(textSize*(float)nchars);
	Quad(x, y, x, y+h, x+w, y+h, x+w, y, true, colorOverride? *colorOverride : color, 1, 2);
	int midX = x+w/2, midY = y-h/2;
//	Text((int) (midX-npixels/2), (int) (midY+(int)(1.5f*textSize)), textColor, textSize, nameOverride? nameOverride : name.c_str());
}

bool Button::Hit(int xMouse, int yMouse) {
	return xMouse > x && xMouse < x+w && yMouse > y && yMouse < y+h;
}

bool Button::Hit(double xMouse, double yMouse) {
	return xMouse > x && xMouse < x+w && yMouse > y && yMouse < y+h;
}

// Toggler

Toggler::Toggler(const char *cname, int x, int y, float dia, vec3 col, vec3 ringCol)
	: ptr(NULL), x(x), y(y), dia(dia), onCol(col), offCol(col), ringCol(ringCol) {
	if (cname) name = string(cname);
};

Toggler::Toggler(const char *cname, bool on, int x, int y, float dia, vec3 onCol, vec3 offCol, vec3 ringCol)
	: x(x), y(y), dia(dia), onCol(onCol), offCol(offCol), ringCol(ringCol) {
	ptr = &this->state;
	*ptr = on;
	if (cname) name = string(cname);
};

Toggler::Toggler(bool *on, const char *cname, int x, int y, float dia, vec3 onCol, vec3 offCol, vec3 ringCol)
	: ptr(on), x(x), y(y), dia(dia), onCol(onCol), offCol(offCol), ringCol(ringCol) {
	if (cname) name = string(cname);
};

void Toggler::Draw(const char *nameOverride, float textSize, vec3 *color) {
	// assume ScreenMode and disabled depth test
	vec3 p((float)x, (float)y, 0);
	const char *s = nameOverride? nameOverride : name.c_str();
	Disk(p, dia, ringCol);
	Disk(p, dia-4, color? *color : (On()? onCol : offCol));
//	Text((float)x+.7f*(float)textSize, (float)y-6, vec3(0, 0, 0), textSize, s);
}

bool Toggler::Hit(int xMouse, int yMouse, int proximity) {
	return Hit((float) xMouse, (float) yMouse);
}

bool Toggler::Hit(double xMouse, double yMouse, int proximity) {
	return Hit((float) xMouse, (float) yMouse);
}

bool Toggler::Hit(float xMouse, float yMouse, int proximity) {
	return MouseOver(xMouse, yMouse, vec2(x, y), proximity);
}

bool Toggler::DownHit(double xMouse, double yMouse, int proximity) {
	return DownHit((int) xMouse, (int) yMouse, proximity);
}

bool Toggler::DownHit(int xMouse, int yMouse, int proximity) {
	return DownHit((float) xMouse, (float) yMouse, proximity);
}

bool Toggler::DownHit(float xMouse, float yMouse, int proximity) {
	bool hit = Hit(xMouse, yMouse, proximity);
	if (hit) {
		if (ptr != NULL) *ptr = *ptr? false : true;
		else state = !state;
	}
	return hit;
}

bool Toggler::On() { return ptr == NULL? false : *ptr; }

void Toggler::Set(bool set) { if (ptr) *ptr = set; }

const char *Toggler::Name() { return name.c_str(); }

void Toggler::SetName(const char *s) { name = string(s); }

// Magnifier

Magnifier::Magnifier(int2 srcLoc, int2 dspSize, int blockSize) : srcLoc(srcLoc), blockSize(blockSize) {
	int nxBlocks = dspSize[0]/blockSize, nyBlocks = dspSize[1]/blockSize;
	displaySize = int2(nxBlocks*blockSize, nyBlocks*blockSize);
}

Magnifier::Magnifier(int srcX, int srcY, int sizeX, int sizeY, int blockSize) {
	*this = Magnifier(int2(srcX, srcY), int2(sizeX, sizeY), blockSize);
}

void Magnifier::Down(int x, int y) {
	y = VPh()-y;
	srcLocSave = srcLoc;
	mouseDown = int2(x, y);
}

void Magnifier::Drag(int x, int y) {
	y = VPh()-y;
	srcLoc[0] = srcLocSave[0]+x-mouseDown[0];
	srcLoc[1] = srcLocSave[1]+y-mouseDown[1];
}

bool Magnifier::Hit(int x, int y) {
	y = VPh()-y;
	int nxBlocks = displaySize[0]/blockSize, nyBlocks = displaySize[1]/blockSize;
	return x >= srcLoc[0] && y >= srcLoc[1] && x <= srcLoc[0]+nxBlocks-1 && y <= srcLoc[1]+nyBlocks-1;
}

void Magnifier::Display(int2 displayLoc, bool showSrcWindow) {
	class Helper { public:
		void Rect(int xi, int yi, int wi, int hi, bool solid, vec3 col) {
			float x = (float) xi, y = (float) yi, w = (float) wi, h = (float) hi;
			vec3 p0(x, y, 0), p1(x+w, y, 0), p2(x+w, y+h, 0), p3(x, y+h, 0);
			Quad(p0, p1, p2, p3, solid, col, 1, 2);
		}
	} h;
	int nxBlocks = displaySize[0]/blockSize, nyBlocks = displaySize[1]/blockSize;
	int dy = displaySize[1]-nyBlocks*blockSize;
	float *pixels = new float[nxBlocks*nyBlocks*3];
	glReadPixels(srcLoc[0], srcLoc[1], nxBlocks, nyBlocks, GL_RGB, GL_FLOAT, pixels);
	for (int i = 0; i < nxBlocks; i++)
		for (int j = 0; j < nyBlocks; j++) {
			float *pixel = pixels+3*(j*nxBlocks+i);
//			float *pixel = pixels+3*(i*nyBlocks+j);
			vec3 col(pixel[0], pixel[1], pixel[2]);
			h.Rect(displayLoc[0]+blockSize*i, displayLoc[1]+blockSize*j+dy, blockSize, blockSize, true, col);
		}
	glDisable(GL_BLEND);
	if (showSrcWindow)
		h.Rect(srcLoc[0], srcLoc[1], nxBlocks-1, nyBlocks-1, false, vec3(0, .7f, 0));
	h.Rect(displayLoc[0], displayLoc[1]+dy, nxBlocks*blockSize, nyBlocks*blockSize, false, vec3(0, .7f, 0));
	delete [] pixels;
}
