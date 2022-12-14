// Camera.h (c) 2019-2022 Jules Bloomenthal

#ifndef CAMERA_HDR
#define CAMERA_HDR

#include "VecMat.h"

// mousex, mousey in screen coordinates (xmouse increases rightwards, ymouse increases downwards)
// invertVertical should be true to accommodate downwards increasing mouse y
// simple camera parameters and methods for mouse
// no-shift
//   drag: rotate about X and Y axes
//   wheel: rotate about Z axis
// shift
//   drag: translate along X and Y axes
//   wheel: translate along Z axis

class Camera {
private:
	float   aspectRatio = 1;
	vec3    rot, tran;                  // Euler angles and position
	float   fov = 30;
	float   nearDist = .001f, farDist = 500;
	bool    invertVertical = true;      // OpenGL defines origin lower left; Windows defines it upper left
	vec2    mouseDown;                  // for each mouse down, need start point
	vec3    rotOld, tranOld;            // reference during drag
	vec3    rotateCenter;               // world rotation origin
	vec3    rotateOffset;               // for temp change in world rotation origin
	float   tranSpeed = .01f;
	float   rotSpeed = .3f;
	bool	shift = false, control = false;
public:
	mat4    modelview, persp, fullview; // read-only
	mat4    GetRotate();
	void    SetRotateCenter(vec3 r);
	void    MouseUp();
	void    MouseDown(int xmouse, int ymouse, bool shift = false, bool control = false);
	void    MouseDown(double xmouse, double ymouse, bool shift = false, bool control = false);
	void    MouseDrag(int xmouse, int ymouse);
	void    MouseDrag(double xmouse, double ymouse);
	void    MouseWheel(double spin, bool shift = false);
	void    Resize(int w, int h);
	float   GetFOV();
	void    SetFOV(float fov);
	void    SetFOV(float fov, float nearDist, float farDist);
	void    SetSpeed(float rotSpeed, float tranSpeed);
	vec3    GetRot();
	vec3    GetTran();
	char   *Usage();
	Camera() { };
	Camera(int scrnX, int scrnY, int scrnW, int scrnH, vec3 rot = vec3(0,0,0), vec3 tran = vec3(0,0,0),
		   float fov = 30, float nearDist = .001f, float farDist = 500, bool invVrt = true);
	Camera(int scrnW, int scrnH, vec3 rot = vec3(0,0,0), vec3 tran = vec3(0,0,0),
		   float fov = 30, float nearDist = .001f, float farDist = 500, bool invVrt = true);
	Camera(float aspectRatio, vec3 rot = vec3(0,0,0), vec3 tran = vec3(0,0,0),
		   float fov = 30, float nearDist = .001f, float farDist = 500, bool invVrt = true);
};

#endif
