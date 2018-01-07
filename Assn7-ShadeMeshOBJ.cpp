// ShadeMeshOBJ.cpp: Phong shade .obj mesh

#include <stdio.h>
#include "glew.h"
#include "freeglut.h"
#include "GLSL.h"
#include "MeshIO.h"

// Application Data

char *objFilename = "C:\\Users\\Raj\\Desktop\\Graphics\\Assignment1\\Coffeecup.obj";
char *fileName = "C:\\Users\\Raj\\Desktop\\Graphics\\Assignment1\\parrots.tga";

vector<vec3> points;				// 3D mesh vertices
vector<vec3> normals;				// vertex normals
vector<int3> triangles;				// triplets of vertex indices
vector<vec2> uvs;

vec3  lightSource(1, 1, 0);		// for Phong shading
GLuint vBuffer = 0;				// GPU vertex buffer ID
GLuint program = 0;				// GLSL program ID
GLuint textureId = 0;

// Shaders

char *vertexShader = "\
	#version 130														\n\
	in vec3 point;														\n\
	in vec3 normal;														\n\
	in vec2 uv;															\n\
	out vec2 vUv;														\n\
	out vec3 vPoint;													\n\
	out vec3 vNormal;													\n\
    uniform mat4 view;													\n\
	uniform mat4 persp;													\n\
	uniform mat4 scale;													\n\
	void main() {														\n\
		vPoint = (view*vec4(point, 1)).xyz;								\n\
		vNormal = (view*vec4(normal, 0)).xyz;							\n\
		vUv = uv;														\n\
		//Bonus2														\n\
		//vUv = vec2(vec4(uv,0,1)*scale).xy;							\n\
		//Bonus1														\n\
		//vUv = 4*uv;													\n\
		gl_Position = persp*vec4(vPoint, 1);							\n\
	}";

char *pixelShader = "\
    #version 130														\n\
	in vec3 vPoint;														\n\
	in vec3 vNormal;													\n\
	in vec2 vUv;														\n\
	out vec4 pColor;													\n\
	uniform vec3 light = vec3(-.2, .1, -3);								\n\
	uniform sampler2D textureImage;										\n\
    void main() {														\n\
		vec3 N = normalize(vNormal);       // surface normal			\n\
        vec3 L = normalize(light-vPoint);  // light vector				\n\
        vec3 E = normalize(vPoint);        // eye vertex				\n\
        vec3 R = reflect(L, N);            // highlight vector			\n\
        float d = abs(dot(N, L));          // two-sided diffuse			\n\
        float s = abs(dot(R, E));          // two-sided specular		\n\
		float intensity = clamp(d+pow(s, 50), 0, 1);					\n\
		pColor = vec4(intensity*texture(textureImage, vUv).rgb, 1);							\n\
	}";

// Initialization

void InitVertexBuffer() {
    // create GPU buffer, make it the active buffer
    glGenBuffers(1, &vBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
    // allocate memory for vertex positions and normals
	int sizePts = points.size()*sizeof(vec3);
	int sizeNrms = normals.size()*sizeof(vec3);
	int sizeUvs = uvs.size() * sizeof(vec2);
	
    glBufferData(GL_ARRAY_BUFFER, sizePts+sizeNrms+sizeUvs, NULL, GL_STATIC_DRAW);
    // copy data
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizePts, &points[0]);
	if (sizeNrms)
		glBufferSubData(GL_ARRAY_BUFFER, sizePts, sizeNrms, &normals[0]);
	if (sizeUvs)
		glBufferSubData(GL_ARRAY_BUFFER, sizeNrms, sizeUvs, &uvs[0]);
}

// Interactive Rotation

vec2 mouseDown;				// for each mouse down, need start point
vec2 rotOld, rotNew;	    // previous, current rotations

void MouseButton(int butn, int state, int x, int y) {
	if (state == GLUT_DOWN)
		mouseDown = vec2((float) x, (float) y);
	if (state == GLUT_UP)
		rotOld = rotNew;
}


void InitTexture(const char *filename) {
	int width, height;
	char*pixels = ReadTexture(filename, width, height);

	if (pixels) {
		// allocate GPU texture buffer; copy, free pixels
		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);// in case width not multiple of 4
											  // transfer pixel data
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, pixels);
		delete[]pixels;
		glGenerateMipmap(GL_TEXTURE_2D);
		// refer sampler uniform to texture 0 (default texture)
		// for multiple textures, see glActiveTexture
		GLSL::SetUniform(program, "textureImage", 0);
	}
}


void MouseDrag(int x, int y) {
	vec2 mouse((float) x, (float) y);
	rotNew = rotOld+.3f*(mouse-mouseDown);
	glutPostRedisplay();
}

// Application

void Display() {
    glUseProgram(program);
	// update view matrix
	mat4 view = Translate(0, 0, -10)*RotateY(rotNew.x)*RotateX(rotNew.y);
	GLSL::SetUniform(program, "view", view);
	// update persp matrix
	static float fov = 15, nearPlane = -.001f, farPlane = -500;
	static float aspect = (float)glutGet(GLUT_WINDOW_WIDTH)/(float)glutGet(GLUT_WINDOW_HEIGHT);
	mat4 persp = Perspective(fov, aspect, nearPlane, farPlane);
	GLSL::SetUniform(program, "persp", persp);
	// transform light and send to fragment shader
	vec4 hLight = view*vec4(lightSource, 1);
	GLSL::SetUniform(program, "light", vec3(hLight.x, hLight.y, hLight.z));

	//Bonus2
	mat4 scale = Scale(3, 3, 3);
	GLSL::SetUniform(program, "scale", scale);


	// clear screen to grey, enable transparency, use z-buffer
    glClearColor(.5f, .5f, .5f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_BUFFER);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
    // setup vertex feeder
	int sizePts = points.size()*sizeof(vec3);
	int sizeNrms = normals.size() * sizeof(vec3);
	GLSL::VertexAttribPointer(program, "point", 3, GL_FLOAT, GL_FALSE, 0, (void *) 0);
	GLSL::VertexAttribPointer(program, "normal", 3, GL_FLOAT, GL_FALSE, 0, (void *) sizePts);
	GLSL::VertexAttribPointer(program, "uv", 2, GL_FLOAT, GL_FALSE, 0, (void *) sizeNrms);
	// draw triangles, finish
    glDrawElements(GL_TRIANGLES, 3*triangles.size(), GL_UNSIGNED_INT, &triangles[0]);
    glFlush();
}

void Close() {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBuffer);
}

void main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitWindowSize(400, 400);
    glutCreateWindow("Texture Example");
    glewInit();
	program = GLSL::LinkProgramViaCode(vertexShader, pixelShader);
	InitTexture(fileName);
	if (!ReadAsciiObj(objFilename, points, triangles, &normals)) {
		printf("failed to read obj file\n");
		getchar();
		return;
	}
	printf("%i vertices, %i triangles, %i normals\n", points.size(), triangles.size(), normals.size());
	Normalize(points, .8f); // scale/move model to uniform +/-1, approximate normals if none from file
    InitVertexBuffer();
    glutDisplayFunc(Display);
	glutMouseFunc(MouseButton);
	glutMotionFunc(MouseDrag);
    glutCloseFunc(Close);
    glutMainLoop();
}
