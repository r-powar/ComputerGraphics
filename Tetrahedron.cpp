// Assn3-RotateLetter.cpp: rotate letter over time

#include <stdio.h>
#include "glew.h"
#include "freeglut.h"
#include "GLSL.h"
#include <vector>
#include <time.h>
#include "mat.h"

// Application Data

GLuint vBuffer = 0;		// GPU vertex buffer ID 
GLuint program = 0;		// GLSL program ID

float s = .8f, f = s / sqrt(2.f);
// 10 2D vertex locations for 'B'
float points[][3] = { { -s,0,-f },{ s,0,-f },{ 0,-s,f },{ 0,s,f } };

// 10 colors
float colors[][3] = { { 1, 1, 1 },{ 1, 0, 0 },{ 0.5f, 0, 0 },{ 1, 1, 0 } };

// 4 triangles
int triangles[][3] = { { 0, 1, 2 },{ 0, 1, 3 },{ 1, 3, 2 },{ 0, 3, 2 } };

//Mouse movement and rotation about x and y axis
vec2 mouseDown;
vec2 rotOld, rotNew;

//control rotational speeds
float speed = .3f;

// Shaders

char *vertexShader = "\
	#version 130								\n\
	in vec3 vPoint;								\n\
	in vec3 vColor;								\n\
	out vec4 color;								\n\
	uniform mat4 view;							\n\
	void main() {								\n\
		gl_Position = view*vec4(vPoint, 1);		\n\
	    color = vec4(vColor, 1);				\n\
	}\n";

char *pixelShader = "\
	#version 130								\n\
	in vec4 color;								\n\
	out vec4 pColor;							\n\
	void main() {								\n\
        pColor = color;							\n\
	}\n";

// Vertex Buffering

struct Vertex {
	vec3 point;
	vec3 color;
	Vertex() { }
	Vertex(float *p, float *c) : point(vec3(p[0], p[1])), color(vec3(c[0], c[1], c[2])) { }
};

std::vector<Vertex> vertices;

void InitVertexBuffer() {
	// create vertex array
	int ntriangles = sizeof(triangles) / (3 * sizeof(int));
	vertices.resize(3 * ntriangles);
	for (int i = 0; i < ntriangles; i++)
		for (int k = 0; k < 3; k++) {
			int vid = triangles[i][k];
			vertices[3 * i + k] = Vertex(points[vid], colors[vid]);
		}
	// create and bind GPU vertex buffer, copy vertex data
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
}

// Animation

time_t startTime = clock();
static float degPerSec = 30;

void Idle() {
	glutPostRedisplay();
}

void MouseButton(int button, int state, int x, int y) {
	if (state == GLUT_DOWN) {
		mouseDown = vec2((float)x, (float)y);
	}
	if (state == GLUT_UP) {
		rotOld = rotNew;
	}
}

void MouseDrag(int x, int y) {
	vec2 mouse((float)x, (float)y);
	rotNew = rotOld + speed * (mouse - mouseDown);
	glutPostRedisplay();
}

// Application

void Display() {
	glClearColor(.5, .5, .5, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(program);

	// update angle
	//float dt = (float)(clock()-startTime)/CLOCKS_PER_SEC; // duration since start

	//angle in degress
	mat4 view = RotateY(rotNew.x)*RotateX(rotNew.y);
	GLSL::SetUniform(program, "view", view);
	// establish vertex fetch for point and for color
	GLSL::VertexAttribPointer(program, "vPoint", 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
	GLSL::VertexAttribPointer(program, "vColor", 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) sizeof(vec2));

	//enable z-buffer
	glEnable(GL_DEPTH_BUFFER);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// draw triangles
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
	glFlush();
}

void Close() {
	// unbind vertex buffer and free GPU memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBuffer);
}

void main(int argc, char **argv) {
	// init window
	glutInit(&argc, argv);
	glutInitWindowSize(400, 400);
	glutCreateWindow("Rotate Letter");
	glewInit();
	// build and use shader program
	program = GLSL::LinkProgramViaCode(vertexShader, pixelShader);
	// allocate vertex memory in the GPU and link it to the vertex shader
	InitVertexBuffer();
	// GLUT callbacks and event loop
	glutDisplayFunc(Display);
	glutMouseFunc(MouseButton);
	glutMotionFunc(MouseDrag);
	glutCloseFunc(Close);
	glutMainLoop();
}
