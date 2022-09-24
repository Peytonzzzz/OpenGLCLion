// Rolling Ball: using rolling motion, compare quaternions vs Euler angles

// HTML, OpenGL, shaders
var canvas;
var gl;
var program;

// table
var quadVertexBuf;
var quadSize = 5, quadHeight = .5;

// ball location
var ballVertexBuf;
var ballNumTriangles;
var ballRadius = 1;
var ballPosition = vec3(0, 0, -quadHeight);

// ball orientation
var ballOrientation = vec4(0, 0, 0, 1);     // for quaternion method
var ballRotate = mat4();                    // for matrix method
var ballRotX = 0, ballRotY = 0;             // for rotation angles method
var ballCurrentMatrix = mat4();
var method = 0;                             // 0: ballOrientation accumulates rotations each arrow press; mRotate set in DrawBall
                                            // 1: matrix: mRotate accumulates rotations each arrow press,
                                            // 2: rot angles
// texture
var quadTexture;
var ballTexture;

// lighting
var light = vec3(1, -4, -3);

// colors
var useColor = 0;
var blue = vec3(0, 0, 1);
var yellow = vec3(.5, .5, 0);

// camera
var camDolly = -5;
var camOrientation = vec4(0, 1, 0, 0);      // 90 deg about y-axis

// mouse
var mouseStart;
var mouseDown = false;

function InitTexture(image) {
//  image.crossOrigin = "anonymous";
    if (!image)
        alert("no texture image!");
    console.log("InitTexture, image = "+image.src);
    gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGB, gl.RGB, gl.UNSIGNED_BYTE, image);
    gl.generateMipmap(gl.TEXTURE_2D);
}

function InitSphere(res) {
    // each pole is represented as a circle with u varying (0,1)
    // res: # lat circles north to south, excl. poles, incl equator if res odd
    var tmp_uvs = [];
    var nCircles = res, nCirclePoints = 2*res+2;
    ballNumTriangles = 2*(nCircles+1)*nCirclePoints;
    // fill ballVertices and send to GPU
    var nVertices = ballNumTriangles*3, nFloats = 8*nVertices, count = 0;
    var ballVertices = new Float32Array(nFloats);
    function AddTriangle(i1, i2, i3) {
        for (var k = 0; k < 3; k++) {
            var uv = tmp_uvs[k>1? i3 : k>0? i2 : i1];
            var pt = PtFromUV(uv[0], uv[1]);
            var data = [pt[0], pt[1], pt[2], pt[0], pt[1], pt[2], uv[0], uv[1]];
            for (var i = 0; i < 8; i++)
                ballVertices[count++] = data[i];
        }
    }
    function PtFromUV(u, v) {
        var elevation = Math.PI/2-(1-v)*Math.PI; // PI/2 = N. pole, 0 = equator, -PI/2 = S. pole
        var eFactor = Math.cos(elevation);
        var y = Math.sin(elevation);
        var angle = 2*Math.PI*(1-u);
        var x = eFactor * Math.cos(angle);
        var z = eFactor * Math.sin(angle);
        return vec3(x, y, z);
    }
    // set circle uvs: ea lat circle, 1st pt (u=0) repeated at end (u=1)
    for (var i = 0; i <= nCircles+1; i++) {
        var v = i / (nCircles+1);
        for (var j = 0; j <= nCirclePoints; j++) {
            var u = j / nCirclePoints; // 0 through 1
            tmp_uvs.push(vec2(u, v));
        }
    }
    // triangles between circles (1st circle is n pole, last is s pole)
    for (var k = 0; k <= nCircles; k++) {
        var vidCircle1 = k*(nCirclePoints+1);
        var vidCircle2 = vidCircle1+nCirclePoints+1;
        for (var j = 0; j < nCirclePoints; j++) {
            var vid1A = vidCircle1+j, vid1B = vid1A+1;
            var vid2A = vidCircle2+j, vid2B = vid2A+1;
            AddTriangle(vid1A, vid1B, vid2B);
            AddTriangle(vid1A, vid2B, vid2A);
        }
    }
    // buffer vertices
    ballVertexBuf = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, ballVertexBuf);
    gl.bufferData(gl.ARRAY_BUFFER, ballVertices, gl.STATIC_DRAW);
    // texture
    gl.activeTexture(gl.TEXTURE1+0);
    ballTexture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, ballTexture);
    InitTexture(document.getElementById("ballImage"));
}  // end InitSphere

function InitQuad() {
    var s = quadSize, h = quadHeight;
    // 6 points for 2 triangles, each with x,y,z,nx,ny,nz,u,v
    var quadVertices = new Float32Array([-s, -s, h, 0, 0, 1, 0, 0, -s, s, h, 0, 0, 1, 0, 1, s, s, h, 0, 0, 1, 1, 1,
                                         -s, -s, h, 0, 0, 1, 0, 0, s, s, h, 0, 0, 1, 1, 1, s, -s, h, 0, 0, 1, 1, 0]);
    quadVertexBuf = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, quadVertexBuf);
    gl.bufferData(gl.ARRAY_BUFFER, quadVertices, gl.STATIC_DRAW);
    // texture
    gl.activeTexture(gl.TEXTURE1+1);
    quadTexture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, quadTexture);
    InitTexture(document.getElementById("quadImage"));
}

function EnableAttribute(attribute) {
    var id = gl.getAttribLocation(program, attribute);
    gl.enableVertexAttribArray(id);
    return id;
}

function DrawQuad(camview) {
    gl.bindBuffer(gl.ARRAY_BUFFER, quadVertexBuf);
    // enable vertex attributes
    var pointId = EnableAttribute("point");
    var nrmId = EnableAttribute("normal");
    var uvId = EnableAttribute("uv");
    // connect vertex shader inputs
    gl.vertexAttribPointer(pointId, 3, gl.FLOAT, false, 8*4, 0);
    gl.vertexAttribPointer(nrmId, 3, gl.FLOAT, false, 8*4, 3*4);
    gl.vertexAttribPointer(uvId, 2, gl.FLOAT, false, 8*4, 6*4);
    // set texture, draw
	gl.uniform3fv(gl.getUniformLocation(program, "color"), flatten(yellow));
	gl.activeTexture(gl.TEXTURE1);
	gl.bindTexture(gl.TEXTURE_2D, quadTexture);
	gl.uniform1i(gl.getUniformLocation(program, "texmap"), 1);
    var modelviewId = gl.getUniformLocation(program, "modelview");
    gl.uniformMatrix4fv(modelviewId, false, flatten(camview));
    gl.uniform1i(gl.getUniformLocation(program, "doShadow"), 1);
    // load transformed ball position
	var hBall = mult(camview, vec4(ballPosition, 1));
	var xBall = vec3(hBall[0], hBall[1], hBall[2]);
	gl.uniform3fv(gl.getUniformLocation(program, "ballPosition"), flatten(xBall));
    gl.drawArrays(gl.TRIANGLES, 0, 6);
}

function DrawBall(camview) {
    // load modelview
    var mTranslate = translate(ballPosition);
    ballCurrentMatrix = method == 0? MatrixFromQuaternion(ballOrientation) :
                        method == 1? ballRotate :
                                     mult(rotateX(ballRotX), rotateY(ballRotY));
    var modelview = mult(camview, mult(mTranslate, ballCurrentMatrix));
    var modelviewId = gl.getUniformLocation(program, "modelview");
    gl.uniformMatrix4fv(modelviewId, false, flatten(modelview));
    // enable vertex attributes
    var pointId = EnableAttribute("point");
    var nrmId = EnableAttribute("normal");
    var uvId = EnableAttribute("uv");
    // connect vertex shader inputs
    gl.bindBuffer(gl.ARRAY_BUFFER, ballVertexBuf);
    gl.vertexAttribPointer(pointId, 3, gl.FLOAT, false, 8*4, 0);
    gl.vertexAttribPointer(nrmId, 3, gl.FLOAT, false, 8*4, 3*4);
    gl.vertexAttribPointer(uvId, 2, gl.FLOAT, false, 8*4, 6*4);
    gl.uniform1i(gl.getUniformLocation(program, "doShadow"), 0);
    // set texture, draw
	gl.uniform3fv(gl.getUniformLocation(program, "color"), flatten(blue));
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, ballTexture);
    gl.uniform1i(gl.getUniformLocation(program, "texmap"), 0);
    gl.drawArrays(gl.TRIANGLES, 0, 3*ballNumTriangles);
}

function Display() {
    // clear, zbuffer, shader program
    gl.clearColor(0, .5, .5, 1);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
    gl.enable(gl.DEPTH_TEST);
    gl.useProgram(program);
    // load perspective matrix
    var persp = mat4();
    persp = mult(persp, perspective(45, canvas.width/canvas.height, .1, 100));
    var perspId = gl.getUniformLocation(program, "persp");
    gl.uniformMatrix4fv(perspId, false, flatten(persp));
    // compute camera view matrix
 	var camRot = MatrixFromQuaternion(camOrientation);
    var camview = translate(0, 0, camDolly);
    camview = mult(camview, camRot);
    // color
    gl.uniform1i(gl.getUniformLocation(program, "useColor"), useColor);
    // load light
    var hLight = mult(camview, vec4(light, 1));
    var xLight = vec3(hLight[0], hLight[1], hLight[2]);
	gl.uniform3fv(gl.getUniformLocation(program, "light"), flatten(xLight));
    // draw objects
    DrawQuad(camview);
    DrawBall(camview);
}

function MouseUp(x, y) {
    mouseDown = false;
    Display();
}

function MouseDown(x, y) {
    mouseDown = true;
    mouseStart = vec2(x, y);
	Display();
}

function MouseMove(x, y) {
    if (mouseDown) {
        var xdif = x-mouseStart[0], ydif = y-mouseStart[1];
        var xrad = radians(-.1*xdif), yrad = radians(-.1*ydif);
        // compute quaternions (xyz ~ axis, w: acos(ang/2)
        var q1 = vec4(Math.sin(yrad), 0, 0, Math.cos(yrad)); // rotation about x-axis
        var q2 = vec4(0, Math.sin(xrad), 0, Math.cos(xrad)); // rotation about y-axis
        camOrientation = QuaternionMultiply(camOrientation, q1);
        camOrientation = QuaternionMultiply(camOrientation, q2);
        mouseStart = vec2(x, y);
        Display();
    }
}

function MouseWheel(e) {
	camDolly += .1*e.deltaY;
	Display();
}

function CheckArrowKey(e) {
    var keyDist = .1;
    var degRotate = 5, radRotate = radians(degRotate);
	var c = Math.cos(radRotate/2), s = Math.sin(radRotate/2);
    e = e || window.event;
    if (e.keyCode == '38' && (ballPosition[1]+ballRadius+keyDist) < quadSize) { // up arrow
        // up arrow: positive x-rotation
        ballRotX -= degRotate;
        ballOrientation = QuaternionMultiply(ballOrientation, vec4(s, 0, 0, -c));
        ballRotate = mult(rotateX(-degRotate), ballRotate);
		ballPosition[1] += keyDist;
    }
    if (e.keyCode == '40' && (ballPosition[1]-ballRadius-keyDist) > -quadSize) { // down
        // down arrow: negative x-rotation
        ballRotX += degRotate;
        ballOrientation = QuaternionMultiply(ballOrientation, vec4(s, 0, 0, c));
        ballRotate = mult(rotateX(degRotate), ballRotate);
		ballPosition[1] -= keyDist;
    }
    if (e.keyCode == '37' && (ballPosition[0]+ballRadius+keyDist) < quadSize) { // left
        // left arrow: positive y-rotation
        ballRotY += degRotate;
        ballOrientation = QuaternionMultiply(ballOrientation, vec4(0, s, 0, c));
        ballRotate = mult(rotateY(degRotate), ballRotate);
		ballPosition[0] += keyDist;
    }
    if (e.keyCode == '39' && (ballPosition[0]-ballRadius-keyDist) > -quadSize) { // right
        // right arrow: negative y-rotation
        ballRotY -= degRotate;
        ballOrientation = QuaternionMultiply(ballOrientation, vec4(0, s, 0, -c));
        ballRotate = mult(rotateY(-degRotate), ballRotate);
		ballPosition[0] -= keyDist;
    }
    Display();
}

function RotXRotYFromMatrix(m) {
    // doesn't deal properly with multiple solutions, should support rotZ too
    if (abs(m[2][0]) != 1) {
        rotX = -Math.asin(m[2][0]);
        var c = Math.cos(rotX);
        rotY = -Math.atan2(m[2][1]/c, m[2][2]/c);
    }
    else {
        if (m[2][0] == -1) {
            rotX = Math.pi/2;
            rotY = Math.atan2(m[0][1], m[0][2]);
        }
        else {
            rotX = -Math.pi/2;
            rotY = Math.atan2(-m[0][1], -m[0][2]);
        }
    }
    return vec2(rotX, rotY);
}

function ChangeColor() {
    function degrees(r) { return r*180.0/Math.PI; }
    var names = ["Using Texture", "Using Color"];
    useColor = 1-useColor;
    var button = document.getElementById("changeColor").firstChild;
    button.data = names[useColor];
    Display();
}

function ChangeMode() {
    function degrees(r) { return r*180.0/Math.PI; }
    var names = ["Using Quaternion", "Using Matrices", "Using Rot Angles"];
    method = (method+1)%3;
    var button = document.getElementById("changeMode").firstChild;
    button.data = names[method];
    if (method == 0) {
        ballOrientation = QuaternionFromMatrix(ballCurrentMatrix);
        ballRotate = ballCurrentMatrix;
    }
    if (method == 2) {
        var rots = RotXRotYFromMatrix(ballCurrentMatrix);
        ballRotY = degrees(rots[0]);
        ballRotX = degrees(rots[1]);
    }
    Display();
}

window.onload = function webGLStart() {
    canvas = document.getElementById("gl-canvas");
    if (!(gl = WebGLUtils.setupWebGL(canvas)))
        alert("no WebGL!");
    gl.viewport(0, 0, canvas.width, canvas.height);
    program = initShaders(gl, "vertex-shader", "fragment-shader");
    InitSphere(25);
    InitQuad();
    canvas.addEventListener("mousedown", function (event) { MouseDown(event.clientX, event.clientY); });
    canvas.addEventListener("mouseup", function (event) { MouseUp(event.clientX, event.clientY); });
    canvas.addEventListener("mousemove", function (event) { MouseMove(event.clientX, event.clientY); });
    canvas.addEventListener("wheel", function (event) { MouseWheel(event); });
    document.onkeydown = CheckArrowKey;
    document.getElementById("changeMode").onclick = function () { ChangeMode(); };
    document.getElementById("changeColor").onclick = function () { ChangeColor(); };
    Display();
}
