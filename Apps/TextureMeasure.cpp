// TextureMeasure: match vertices/uvs

#include <glad.h>
#include <glfw3.h>
#include <string.h>
#include <time.h>
#include <CameraArcball.h>
#include <Draw.h>
#include <GLXtras.h>
#include <Mesh.h>
#include <Misc.h>
#include <Sprite.h>
#include <Text.h>
#include <VecMat.h>
#include <Widgets.h>

// files
string dir = "C:/Users/Jules/Code/Book/";
string meshName = "Face.obj", textureName = "Face.tga";

// mesh, texture
Mesh mesh;
Sprite texture;
vector<int> vertexGroups;

// lighting
vec3 lights[] = { {.5, 0, 1}, {1, 1, 0}, {-1, 0, -2} };
const int nLights = sizeof(lights)/sizeof(vec3);

// window, camera
int winWidth = 1600, winHeight = 800;
int camX = 0, camW = 800, camH = winHeight;
int textureX = camW, textureW = winWidth-camW, textureH = winHeight;
CameraAB camera(0, 0, camW, camH, vec3(0, 0, 0), vec3(0, 0, -4.5f), 30, 0.001f, 500);

// colors
vec3 blk(0,0,0), wht(1,1,1), grn(0,1,0), blu(0,0,1), yel(1,1,0), orgRed(1,.27f,0), dkTurq(0,.81f,.82f), brick(.7f,.13f,.13f),
	 gold(1,.84f,0), dkOrchid(.6f,.2f,.8f), medGreen(0,1,.6f), dpPink(1,.08f,.58f), medViolet(0.78f,.08f,.52f), dkGreen(0,.39f,0);

// user selection
Mover mover;
void *picked = NULL;
int texturePick = -1, groupPick = -1;
vec2 cropMin(.01f, .01f), cropMax(.99f, .99f);
vec2 groupCenter; // to scale (via wheel) picked group (vector<vec2> groupCenters better?

// buttons
bool showTexture = true, showFaceted = false, showLines = true, showNormals = false;
bool showVids = false, showTids = false, showUvLines = true, groupMove = true;
Toggler togGroupMove(&groupMove, "group", 20, 380, 14);
Toggler togTexture(&showTexture, "texture", 20, 350, 14);
Toggler togLines(&showLines, "lines", 20, 320, 14);
Toggler togFaceted(&showFaceted, "faceted", 20, 290, 14);
Toggler togNormals(&showNormals, "normals", 20, 260, 14);
Toggler togVids(&showVids, "vrt-#s", 20, 230, 14);
Toggler togTids(&showTids, "tri-#s", 20, 200, 14);
Toggler togUvLines(&showUvLines, "uvLines", 20, 170, 14);
Toggler togReadMesh(NULL, "new mesh", 20, 140, 14);		// read mesh
Toggler togReadTexture(NULL, "new image", 20, 110, 14);	// read texture image
Toggler togSave(NULL, "save", 20, 80, 14);				// save mesh
Toggler togReset(NULL, "reset", 20, 50, 14);			// reset UVs from mesh vertices
Toggler togCrop(NULL, "crop", 20, 20, 14);				// crop texture image
Toggler *togs[] =  {&togCrop,  &togReset, &togSave, &togReadTexture, &togReadMesh, &togUvLines, &togTids,
					&togVids, &togNormals, &togFaceted, &togLines, &togTexture, &togGroupMove };
int nTogs = sizeof(togs)/sizeof(Toggler *);

// Display

vector<vec3> vertexColors;
time_t mouseMoved = clock()-CLOCKS_PER_SEC, mouseDragged = mouseMoved;

vec2 TfromUV(vec2 uv) {
	// return screen-space texture coords from uvs[i]
	return vec2(textureX+textureW*uv.x, textureH*uv.y);
	return vec2(textureX+textureW*uv.x, textureH*uv.y);
}

vec2 TfromUV(int i) { return TfromUV(mesh.uvs[i]); }

void DrawUVs() {
	UseDrawShader(ScreenMode());
	for (int i = 0; i < (int) mesh.triangles.size(); i++) {
		int3 t = mesh.triangles[i];
		for (int k = 0; k < 3; k++)
			Line(TfromUV(t[k]), TfromUV(t[(k+1)%3]), 2, blu);
	}
	if (texturePick >= 0)
		Disk(TfromUV(texturePick), 7, vec3(1, 0, 0));
	for (int i = 0; i < (int) mesh.uvs.size(); i++)
		Disk(TfromUV(i), 9, vertexColors[i]);
	vec2 s1 = TfromUV(cropMin), s3 = TfromUV(cropMax), s2(s1.x, s3.y), s4(s3.x, s1.y);
	Quad(s1, s2, s3, s4, false, blk, .3f, 4);
	Disk(s1, 12, vec3(0, 0, .2f));
	Disk(s3, 12, vec3(0, 0, .2f));
}

void TitleBox(int x1, int y1, int width, int size, const char *title) {
	int x2 = x1+width, y2 = y1+(int)((float)size*1.5f);
	int wtext = TextWidth(size, title), xtext = x1+(width-wtext)/2;
	Quad(x1, y1, x1, y2, x2, y2, x2, y1, true, wht, .4f);
	Text(xtext, y1+size/2-1, blk, (float) size, title);
}

void Display() {
	// clear to gray, clear z-buffer
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// transform lights
	vec3 xLights[nLights];
	for (int i = 0; i < nLights; i++) {
		vec4 xLight = camera.modelview*vec4(lights[i]);
		xLights[i] = vec3(xLight.x, xLight.y, xLight.z);
	}
	// left side: mesh
	glViewport(0, 0, textureX, winHeight);
	glEnable(GL_DEPTH_TEST);
	int meshShader = UseMeshShader(showLines);
	SetUniform(meshShader, "nLights", nLights);
	SetUniform3v(meshShader, "lights", nLights, (float *) xLights);
	SetUniform(meshShader, "useTexture", showTexture);
	SetUniform(meshShader, "facetedShading", showFaceted);
	SetUniform(meshShader, "color", wht);
	if (showLines) {
		SetUniform(meshShader, "outlineColor", vec4(blu, 1));
		SetUniform(meshShader, "outlineWidth", .75f);
		SetUniform(meshShader, "outlineTransition", .75f);
	}
	mesh.Display(camera, 0, showLines, groupMove);
	glDisable(GL_DEPTH_TEST);
	// lights, arcball
	float deltaDrag = (float) (clock()-mouseDragged)/CLOCKS_PER_SEC;
	float deltaMove = (float) (clock()-mouseMoved)/CLOCKS_PER_SEC;
	if ((deltaMove < .3f && Control()) ||
		(deltaDrag < .3f && (!picked || (picked == &camera && !camera.shift))))
			camera.arcball.Draw(Control());
	UseDrawShader(camera.fullview);
	if (deltaMove < .3f)
		for (int i = 0; i < nLights; i++)
			Star(lights[i], 6, IsVisible(lights[i], camera.fullview)? gold : blu, camera.fullview);
	// vertex/triangle annotation
	mat4 mview = camera.modelview*mesh.transform, mfullview = camera.fullview*mesh.transform;
	UseDrawShader(mfullview);
	int nPoints = (int) mesh.points.size(), nTriangles = (int) mesh.triangles.size();
	if (texturePick >= 0)
		Disk(mesh.points[texturePick], 9, vertexColors[texturePick]);
	if (showVids)
		for (int i = 0; i < nPoints; i++)
			Text(mesh.points[i], mfullview, brick, 12, "%i", i);
	if (showTids)
		for (int i = 0; i < nTriangles; i++) {
			int3 t = mesh.triangles[i];
			vec3 center = (mesh.points[t.i1]+mesh.points[t.i2]+mesh.points[t.i3])/3.f;
			Text(center, mfullview, yel, 12, "%i", i);
		}
	if (showNormals)
		for (int i = 0; i < (int) mesh.normals.size(); i++)
			ArrowV(mesh.points[i], .02f*mesh.normals[i], mview, camera.persp, yel);
	// right side: texture image
	glViewport(textureX, 0, textureW, textureH);
	texture.Display();
	if (showUvLines)
		DrawUVs();
	// buttons
	glViewport(0, 0, winWidth, winHeight);
	UseDrawShader(ScreenMode());
	Quad(0, 0, 0, 400, 130, 400, 130, 0, true, wht, .4f);
	for (int i = 0; i < nTogs; i++)
		togs[i]->Draw(NULL, 18);
	// titles
	const char *mName = meshName.c_str(), *tName = textureName.c_str();
	int size = 24, mWidth = TextWidth(size, mName), tWidth = TextWidth(size, tName);
	TitleBox(textureX-mWidth-20, 0, mWidth+20, size, mName);
	TitleBox(winWidth-tWidth-20, 0, tWidth+20, size, tName);
	glFlush();
}

// Vertices

void BufferUVs() {
	int nPts = (int) mesh.points.size(), nNrms = (int) mesh.normals.size(), nUvs = (int) mesh.uvs.size();
	int sPts = nPts*sizeof(vec3), sNrms = nNrms*sizeof(vec3), sUvs = nUvs*sizeof(vec2);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vBufferId);
	glBufferSubData(GL_ARRAY_BUFFER, sPts+sNrms, sUvs, &mesh.uvs[0]);
}

void DefaultUVs(bool buffer, bool normalize) {
	// initialize uvs to correspond with points.xy
	mat4 mTex = Translate(.5f, .54f, 0)*Scale(2, 1.7f, 1);	// custom scale/offset for Face.obj/Face.tga
	// mat4 mTex = Translate(.5f, .5f, 0)*Scale(2, 2, 1);	// default scale/offset (transforms +1/-1 to 0/1)
	int nPoints = (int) mesh.points.size();
	mesh.uvs.resize(nPoints);
	for (int i = 0; i < nPoints; i++) {
		vec4 t(mesh.points[i].x, mesh.points[i].y, 0, 1), T = mTex*t;
		mesh.uvs[i] = vec2(T.x, T.y);
	}
	cropMin = vec2(.01f, .01f);
	cropMax = vec2(.99f, .99f);
	if (normalize) {
		vec2 uvmn, uvmx;
		MinMax(mesh.uvs.data(), nPoints, uvmn, uvmx);
		for (int i = 0; i < nPoints; i++)
			mesh.uvs[i] = (mesh.uvs[i]-uvmn)/(uvmx-uvmn);
	}
	if (buffer)
		BufferUVs();
}

void CropUVs(vec2 uvMin, vec2 uvMax) {
	// set uvs by mapping points.xy to uvMin/uvMax
	vec3 pointsMin, pointsMax;
	MinMax(mesh.points.data(), mesh.points.size(), pointsMin, pointsMax);
	vec2 ptMin(pointsMin.x, pointsMin.y), ptMax(pointsMax.x, pointsMax.y);
	vec2 ptDif(ptMax-ptMin), uvDif(uvMax-uvMin), scale(uvDif.x/ptDif.x, uvDif.y/ptDif.y);
	int nPoints = (int) mesh.points.size();
	mesh.uvs.resize(nPoints);
	float xmn = 10, xmx = -10, ymn = 10, ymx = -10;
	for (int i = 0; i < nPoints; i++) {
		vec2 pt(mesh.points[i].x, mesh.points[i].y);
		mesh.uvs[i] = uvMin+scale*(pt-ptMin);
	}
	BufferUVs();
}

void SetVertexGroupsColors() {
	vec3 colors[] = { yel, grn, orgRed, dkTurq, brick, dkOrchid, medGreen, dpPink, medViolet, dkGreen };
	int npoints = (int) mesh.points.size(), nColors = sizeof(colors)/sizeof(vec3);
	vertexColors.assign(npoints, vec3(0, 0, 1));
	vertexGroups.assign(npoints, -1);
	for (int i = 0; i < (int) mesh.triangleGroups.size(); i++) {
		Group &g = mesh.triangleGroups[i];
		g.color = colors[i % nColors];
		for (int t = g.startTriangle; t < g.startTriangle+g.nTriangles; t++) {
			int3 tri = mesh.triangles[t];
			vertexGroups[tri.i1] = vertexGroups[tri.i2] = vertexGroups[tri.i3] = i;
			vertexColors[tri.i1] = vertexColors[tri.i2] = vertexColors[tri.i3] = g.color;
		}
	}
}

int NUngrouped() {
	int ngroups = mesh.triangleGroups.size();
	return ngroups? mesh.triangleGroups[0].startTriangle : mesh.triangles.size();
}

void PrintMinMaxUVs(const char *s) {
	vec2 mn, mx;
	MinMax(mesh.uvs.data(), mesh.uvs.size(), mn, mx);
	printf("%s: uvmn=(%3.2f,%3.2f), uvmx=(%3.2f,%3.2f)\n", s, mn.x, mn.y, mx.x, mx.y);
}

void PrintGroups() {
	int ngroups = 0, nUngrouped = NUngrouped();
	printf("Ungrouped: start = 0, nTriangles = %i\n", nUngrouped);
	for (int i = 0; i < (int) mesh.triangleGroups.size(); i++)
		if (mesh.triangleGroups[i].nTriangles)
			ngroups++;
	printf("%i group%s:\n", ngroups, ngroups==1? "" : "s");
	for (int i = 0; i < (int) mesh.triangleGroups.size(); i++) {
		Group g = mesh.triangleGroups[i];
		if (g.nTriangles)
			printf("\t%s (group=%i) startTri=%i, nTris=%i, col=(%2.1f,%2.1f,%2.1f)\n",
				g.name.c_str(), i, g.startTriangle, g.nTriangles, g.color.x, g.color.y, g.color.z);
	}
}

// I/O

void Read() {
	string mName = dir+meshName, tName = dir+textureName;
	mesh.Read(dir+meshName, dir+textureName, NULL, false, false);
	if (!mesh.uvs.size())
		DefaultUVs(false, false);	// base UVs on OBJ vertex locations
	mesh.transform = NormalizeMat(mesh.points.data(), mesh.points.size());
	if (mesh.points.size() != mesh.normals.size())
		SetVertexNormals(mesh.points, mesh.triangles, mesh.normals);
	mesh.Buffer();
	SetVertexGroupsColors();
}

string GetName(const char *prompt) {
	char buf[500];
	printf("%s: ", prompt);
	fgets(buf, 500, stdin);
	if (strlen(buf)) buf[strlen(buf)-1] = 0;
	return string(buf);
}

void ReadMesh() {
	meshName = GetName("mesh name");
	Read();
}

void ReadTexture() {
	textureName = GetName("texure name");
	texture.Initialize(dir+textureName);
	Read();
}

void Save() {
	string name = dir+meshName;
	bool ok = WriteAsciiObj(name.c_str(), mesh.points, mesh.normals, mesh.uvs, &mesh.triangles, NULL, NULL, &mesh.triangleGroups);
	printf("%s%s written\n", name.c_str(), ok? "" : " not");
}

// Mouse

float MouseDist(vec2 mouse, vec2 uv) { return length(mouse-TfromUV(uv)); }

int GetMeshUV(vec2 mouse) {
	int r = -1;
	float minLength = 100;
	for (int i = 0; i < (int) mesh.uvs.size(); i++) {
		float l = length(mouse-TfromUV(i));
		if (l < minLength) {
			minLength = l;
			if (l < 10) r = i;
		}
	}
	return r;
}

vec2 UVfromT(vec2 uv) {
	// return uv-coords from screen-space coords
	return vec2((uv.x-textureX)/textureW, uv.y/textureH);
}

void MouseButton(GLFWwindow *w, int butn, int action, int mods) {
	// left screen and left-mouse: pick light or control arcball
	// left screen and right-mouse: pick mesh vertex
	// right screen and left-mouse: pick uv point
	picked = NULL;
	if (action == GLFW_PRESS) {
		double x, y; // , yInv;
		glViewport(0, 0, winWidth, winHeight);
		glfwGetCursorPos(w, &x, &y);
	//	yInv = WindowHeight(w)-y;
		// button test
		for (int i = 0; i < nTogs; i++)
			if (togs[i]->DownHit(x, y)) { // yInv)) {
				picked = togs;
				if (i == 0) CropUVs(cropMin, cropMax);
				if (i == 1) DefaultUVs(true, true);
				if (i == 2) Save();
				if (i == 3) ReadTexture();
				if (i == 4) ReadMesh();
			}
		// test for texture selection
		if (!picked && x >= textureX) {
			// ** right screen
			if (butn == GLFW_MOUSE_BUTTON_LEFT) {
				// ** left-mouse: pick uv point
				double yInv = VPh()-y;
				vec2 mouse(x, yInv);
				texturePick = GetMeshUV(mouse);
				if (texturePick >= 0) {
					picked = &mesh.uvs;
					groupPick = vertexGroups[texturePick];
					if (groupPick >= 0) {
						int n = 0;
						groupCenter = vec2(0, 0);
						for (int i = 0; i < (int) mesh.uvs.size(); i++)
							if (vertexGroups[i] == groupPick) {
								groupCenter += mesh.uvs[i];
								n++;
							}
						groupCenter /= (float) n;
					}
				}
				if (!picked) {
					if (MouseDist(mouse, cropMin) < 10) picked = &cropMin;
					if (MouseDist(mouse, cropMax) < 10) picked = &cropMax;
				}
			}
		} // end right-screen
		// test for point selection
		if (!picked && x < textureX) {
			// ** left screen
			glViewport(0, 0, textureX, winHeight);	// needed by MouseOver, ScreenDist
			if (butn == GLFW_MOUSE_BUTTON_LEFT) {
				// ** left-mouse: pick light or control arcball
				float minDSq = 100;					// seems ok for large mouse cursor
				for (int i = 0; i < nLights; i++) {
					float dsq = ScreenDSq(x, y, lights[i], camera.fullview);
					if (dsq < minDSq) {
						minDSq = dsq;
						picked = &mover;
						mover.Down(&lights[i], (int) x, (int) y, camera.modelview, camera.persp);
					}
				}
				if (picked == NULL) {
					picked = &camera;
					camera.MouseDown((int) x, (int) y, Shift(), Control());
				}
			}
			// if (butn == GLFW_MOUSE_BUTTON_RIGHT)
			//	for (int i = 0; i < (int) mesh.points.size(); i++)
			//		if (MouseOver(x, yInv, mesh.points[i], camera.fullview, 20)) {
			//			picked = &mesh.points;
			//			pointPick = i;
			//		}
		} // end left-screen
	}
	if (action == GLFW_RELEASE)
		camera.MouseUp();
}

void MouseMove(GLFWwindow *w, double x, double y) {
	if (x < textureX)
		mouseMoved = clock();
	if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		if  (x > textureX) {
			double yInv = VPh()-y;
			vec2 mouse(x, yInv);
			if (picked == &mesh.uvs && texturePick >= 0) {
				if (groupMove && groupPick >= 0) {
					vec2 uvPicked = mesh.uvs[texturePick];
					vec2 uvMouse = UVfromT(mouse);
					vec2 dif = uvMouse-uvPicked;
					for (int i = 0; i < (int) mesh.uvs.size(); i++)
						if (vertexGroups[i] == groupPick)
							mesh.uvs[i] += dif;
				}
				else
					mesh.uvs[texturePick] = UVfromT(mouse);
				// buffer uvs
				int nPts = (int) mesh.points.size(), nNrms = (int) mesh.normals.size(), nUvs = (int) mesh.uvs.size();
				int sPts = nPts*sizeof(vec3), sNrms = nNrms*sizeof(vec3), sUvs = nUvs*sizeof(vec2);
				glBindBuffer(GL_ARRAY_BUFFER, mesh.vBufferId);
				glBufferSubData(GL_ARRAY_BUFFER, sPts+sNrms, sUvs, &mesh.uvs[0]);
			}
			if (picked == &cropMin) cropMin = UVfromT(mouse);
			if (picked == &cropMax) cropMax = UVfromT(mouse);
		}
		if (x < textureX) {
			glViewport(0, 0, textureX, winHeight);
			if (picked == &mover)
				mover.Drag((int) x, (int) y, camera.modelview, camera.persp);
			if (picked == &camera) {
				camera.MouseDrag((int) x, (int) y);
				mouseDragged = mouseMoved;
			}
		}
	}
}

void MouseWheel(GLFWwindow *w, double ignore, double spin) {
	double x, y;
	glViewport(0, 0, winWidth, winHeight);
	glfwGetCursorPos(w, &x, &y);
	if (x < textureX)
		camera.MouseWheel(spin, Shift());
	if (x > textureX && groupMove && groupPick >= 0) {
		// scale along X-axis
		float s = spin > 0? 1.1f : .9f;
		vec2 scale(1, 1);
		if (Control())
			scale.x = scale.y = s;
		else if (Shift())
			scale.y = s;
		else
			scale.x = s;
		for (int i = 0; i < (int) mesh.uvs.size(); i++)
			if (vertexGroups[i] == groupPick)
				mesh.uvs[i] = groupCenter+scale*(mesh.uvs[i]-groupCenter);
		BufferUVs();
	}
}

// App

void Resize(GLFWwindow *w, int width, int height) {
	winWidth = width;
	winHeight = height;
	float f = (float) texture.imgWidth/texture.imgHeight;
	textureW = (int) (f*height);
	camW = winWidth-textureW;
	camH = winHeight;
	textureX = camW;
	textureH = winHeight;
	camera.Resize(camW, camH);
}

const char *usage = R"(Usage
  Left Screen
	left-mouse:  pick light / rotate XY, shift: translate XY
	right-mouse: pick mesh vertex
	mouse wheel: rotate Z, shift: translate Z
  Right Screen
	left-mouse:  select/move UV
	mouse wheel: group scale (<ctrl> uniform, <shft> vert, <none> horiz)
)";

int main() {
	if (glfwInit()) {
		GLFWwindow *w = glfwCreateWindow(winWidth, winHeight, "Texture Measure", NULL, NULL);
		if (w) {
			glfwSetWindowPos(w, 100, 50);
			glfwMakeContextCurrent(w);
			gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
			SetFont("C:/Fonts/Tahoma.ttf", 16, 16);
			// read texture
			texture.Initialize(dir+textureName);
			Resize(w, winWidth, winHeight); // recompute based on texture size
			// read OBJ, normalize
			Read();
			PrintGroups();
			glfwSetScrollCallback(w, MouseWheel);
			glfwSetMouseButtonCallback(w, MouseButton);
			glfwSetCursorPosCallback(w, MouseMove);
			glfwSetWindowSizeCallback(w, Resize);
			printf(usage);
			glfwSwapInterval(1);
			while (!glfwWindowShouldClose(w)) {
				Display();
				glfwSwapBuffers(w);
				glfwPollEvents();
			}
			glfwDestroyWindow(w);
		}
		glfwTerminate();
	}
}
