// Camera.cpp (c) 2019-2022 Jules Bloomenthal

#include <glad.h>
#include <stdio.h>
#include "Camera.h"

Camera::Camera(float aspectRatio, vec3 rot, vec3 tran, float fov, float nearDist, float farDist, bool invVrt) :
	aspectRatio(aspectRatio), rot(rot), tran(tran), fov(fov),
	nearDist(nearDist), farDist(farDist), invertVertical(invVrt) {
		persp = Perspective(fov, aspectRatio, nearDist, farDist);
		modelview = Translate(tran)*GetRotate();
		fullview = persp*modelview;
};

Camera::Camera(int scrnW, int scrnH, vec3 rot, vec3 tran, float fov, float nearDist, float farDist, bool invVrt) :
	aspectRatio((float) scrnW/scrnH), rot(rot), tran(tran), fov(fov),
	nearDist(nearDist), farDist(farDist), invertVertical(invVrt) {
		persp = Perspective(fov, aspectRatio, nearDist, farDist);
		modelview = Translate(tran)*GetRotate();
		fullview = persp*modelview;
};

Camera::Camera(int scrnX, int scrnY, int scrnW, int scrnH, vec3 rot, vec3 tran, float fov, float nearDist, float farDist, bool invVrt) :
	aspectRatio((float) scrnW/scrnH), rot(rot), tran(tran), fov(fov),
	nearDist(nearDist), farDist(farDist), invertVertical(invVrt) {
		persp = Perspective(fov, aspectRatio, nearDist, farDist);
		modelview = Translate(tran)*GetRotate();
		fullview = persp*modelview;
};

void Camera::SetFOV(float newFOV) {
	fov = newFOV;
	persp = Perspective(fov, aspectRatio, nearDist, farDist);
	fullview = persp*modelview;
}

void Camera::SetFOV(float newFOV, float newNearDist, float newFarDist) {
	fov = newFOV;
	nearDist = newNearDist;
	farDist = newFarDist;
	persp = Perspective(fov, aspectRatio, nearDist, farDist);
	fullview = persp*modelview;
}

float Camera::GetFOV() {
	return fov;
}

void Camera::Resize(int screenW, int screenH) {
	aspectRatio = (float)screenW/screenH;
	persp = Perspective(fov, aspectRatio, nearDist, farDist);
	fullview = persp*modelview;
}

void Camera::SetSpeed(float rotS, float tranS) {
	rotSpeed = rotS;
	tranSpeed = tranS;
}

mat4 Camera::GetRotate() {
	mat4 moveToCenter = Translate(-rotateCenter);
	mat4 moveBack = Translate(rotateCenter+rotateOffset);
	mat4 rx = RotateX(rot[0]);
	mat4 ry = RotateY(rot[1]);
	mat4 rz = RotateZ(rot[2]);
	return moveBack*rx*ry*rz*moveToCenter;
}

void Camera::SetRotateCenter(vec3 r) {
	// if center of rotation changed, orientation of modelview doesn't change, 
	// but orientation applied to scene with new center of rotation causes shift
	// to scene origin - this is fixed with translation offset computed here
	mat4 m = GetRotate();
	vec4 rXformedWithOldRotateCenter = m*r;
	rotateCenter = r;
	m = GetRotate(); // this is not redundant!
	vec4 rXformedWithNewRotateCenter = m*r;
	for (int i = 0; i < 3; i++)
		rotateOffset[i] += rXformedWithOldRotateCenter[i]-rXformedWithNewRotateCenter[i];
}

void Camera::MouseDrag(int x, int y) {
	MouseDrag((double) x, (double) y);
}

float VPHeight() {
	vec4 vp;
	glGetFloatv(GL_VIEWPORT, (float *) &vp);
	return vp[3];
}

void Camera::MouseDrag(double x, double y) {
	if (invertVertical) {
		vec4 vp;
		glGetFloatv(GL_VIEWPORT, (float *) &vp);
		y = VPHeight()-y;
	}
	vec2 mouse((float) x, (float) y), dif = mouse-mouseDown;
	if (shift)
		tran = tranOld+tranSpeed*vec3(dif.x, dif.y, 0);
	else
		rot = rotOld+rotSpeed*vec2(-dif.y, dif.x);	// rotate
	modelview = Translate(tran)*GetRotate();
	fullview = persp*modelview;
}

void Camera::MouseWheel(double spin, bool shift) {
	float f = 5*(float)spin;
	if (shift)
		tranOld.z = (tran.z += f*tranSpeed);		// dolly in/out
	else
		rotOld.z = (rot.z += f*rotSpeed);
	modelview = Translate(tran)*GetRotate();
	fullview = persp*modelview;
}

void Camera::MouseUp() {
	rotOld = rot;
	tranOld = tran;
}

void Camera::MouseDown(int x, int y, bool shift, bool control) {
	MouseDown((double) x, (double) y, shift, control);
}

void Camera::MouseDown(double x, double y, bool s, bool c) {
	if (invertVertical)
		y = VPHeight()-y;
	shift = s;
	control = c;
	mouseDown = vec2((float) x, (float) y);
	rotOld = rot;
	tranOld = tran;
}

vec3 Camera::GetRot() {
	return rot;
}

vec3 Camera::GetTran() {
	return tran;
}

const char *cameraUsage = R"(usage
	mouse-drag:\trotate x,y
	with shift:\ttranslate x,y
	mouse-wheel:\trotate z
	with shift:\ttranslate z
)";

char *Camera::Usage() {
	return (char *) cameraUsage;
}
