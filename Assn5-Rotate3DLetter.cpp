// Rotate3DLetter.cpp: rotate letter in response to user

#include <stdio.h>
#include "glew.h"
#include "freeglut.h"
#include "GLSL.h"
#include "Draw.h"
#include "Widget.h"
#include <vector>

// Application Data

GLuint vBuffer = 0;   // GPU vertex buffer ID
GLuint program = 0;   // GLSL program ID

#define EXTRA_CREDIT

#ifdef EXTRA_CREDIT
	// tetrahedron
	float s = .8f, f = s/sqrt(2.f);
	float points[][3] = {{-s, 0, -f}, {s, 0, -f}, {0, -s, f}, {0, s, f}};
	float colors[][3] = {{1, 1, 1}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
	int triangles[][3] = {{0, 1, 2}, {0, 1, 3}, {0, 2, 3}, {1, 2, 3}};
#else
	// letter 'B'
	float points[10][3] = {{-.15f, .125f, 0}, {-.5f,  -.75f, 0}, {-.5f,  .75f, 0}, {.17f,  .75f, 0}, {.38f, .575f, 0},
						   { .38f,  .35f, 0}, { .23f, .125f, 0}, {.5f, -.125f, 0}, { .5f, -.5f, 0},  {.25f, -.75f, 0}};
	float colors[10][3] = {{ 1, 1, 1}, { 1, 0, 0}, {.5, 0, 0}, {1, 1, 0},  {.5, 1, 0},
						   { 0, 1, 0}, { 0, 1, 1}, {0, 0, 1},  { 1, 0, 1}, {.5, 0, .5}};
	int triangles[9][3] =  {{0, 1, 2}, {0, 2, 3}, {0, 3, 4}, {0, 4, 5},
							{0, 5, 6}, {0, 6, 7}, {0, 7, 8}, {0, 8, 9}, {0, 9, 1}};
#endif

// Shaders: vertex shader with view transform, trivial pixel shader

	void *picked = NULL;
	float white[] = { 1, 1, 1 };
	Slider fov(30, 20, 70, 5, 45, 15, Ver, "fov", white);

char *vertexShader = "\
	#version 130								\n\
	in vec3 vPoint;								\n\
	in vec3 vColor;								\n\
	out vec4 color;								\n\
	uniform mat4 view;							\n\
	void main()	{								\n\
		gl_Position = view*vec4(vPoint, 1);		\n\
	    color = vec4(vColor, 1);				\n\
	}											\n";

char *pixelShader = "\
	#version 130								\n\
	in vec4 color;								\n\
	out vec4 pColor;							\n\
	void main() {								\n\
        pColor = color;							\n\
	}											\n";

// Vertex Buffering

struct Vertex {
	vec3 point;
	vec3 color;
	Vertex() { }
	Vertex(float *p, float *c) : point(vec3(p[0], p[1], p[2])), color(vec3(c[0], c[1], c[2])) { }
};

std::vector<Vertex> vertices;

void InitVertexBuffer() {
	// create vertex array
	int nvertices = sizeof(triangles)/sizeof(int), ntriangles = nvertices/3;
	vertices.resize(nvertices);
	for (int i = 0; i < ntriangles; i++)
		for (int k = 0; k < 3; k++) {
			int vid = triangles[i][k];
			vertices[3*i+k] = Vertex(points[vid], colors[vid]);
		}
    // create and bind GPU vertex buffer, copy vertex data
    glGenBuffers(1, &vBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
}

// Interaction

vec2  mouseDown;			// reference for mouse drag
vec2  rotOld, rotNew;	    // .x is rotation about Y-axis, .y about X-axis
vec2  tranOld, tranNew;
float rotSpeed = .3f, tranSpeed = .01f;

void MouseButton(int butn, int state, int x, int y) {
	y = glutGet(GLUT_WINDOW_HEIGHT) - y;
	// called when mouse button pressed or released
	if (state == GLUT_DOWN)
		if (fov.Hit(x, y))
			picked = &fov;
		else
		{
			mouseDown = vec2((float) x, (float) y);	// save reference for MouseDrag
		}
	if (state == GLUT_UP) {
		rotOld = rotNew;						// save reference for MouseDrag
		tranOld = tranNew;
		picked = NULL;
	}
}

void MouseDrag(int x, int y) {
	y = glutGet(GLUT_WINDOW_HEIGHT) - y;
	// find mouse drag difference
	vec2 mouse((float) x, (float) y), dif = mouse-mouseDown;
	if (picked == &fov) {
		fov.Mouse(x, y);
	}
	else {
		if (glutGetModifiers() & GLUT_ACTIVE_SHIFT)
			tranNew = tranOld + tranSpeed*vec2(dif.x, -dif.y);	// SHIFT key: translate
		else
			rotNew = rotOld + rotSpeed*dif;						// rotate
	}
	glutPostRedisplay();
}

// Application

void Display() {
	// clear screen to grey
    glClearColor(.5, .5, .5, 1);
    glClear(GL_COLOR_BUFFER_BIT);
	// enable z-buffer (needed for tetrahedron)
	glEnable(GL_DEPTH_BUFFER);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	// update view transformation
    glUseProgram(program);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	mat4 modelview = Translate(tranNew.x, tranNew.y, 0)*RotateY(rotNew.x)*RotateX(rotNew.y);
	float width = (float)glutGet(GLUT_WINDOW_WIDTH);
	float height = (float)glutGet(GLUT_WINDOW_HEIGHT), aspect = width / height;

	mat4 persp = Perspective(fov.GetValue(), aspect, -.01f, -500); // based on field-of-view
	mat4 dolly = Translate(0, 0, -3);// avoid near plane clip

	GLSL::SetUniform(program, "view", persp*dolly*modelview);// send concatenation of 3
    // establish vertex fetch for point and for color
	GLSL::VertexAttribPointer(program, "vPoint", 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	GLSL::VertexAttribPointer(program, "vColor", 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)sizeof(vec3));
 	// draw triangles
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
	// draw controls in 2D screen space
	glDisable(GL_DEPTH_TEST);
	mat4 screen = Translate(-1, -1, 0)*Scale(2 / width, 2 / height, 1);
	UseDrawShader(screen);	// Draw.h
	fov.Draw();			// Widget.h

	glFlush();
}

void Close() {
	// unbind vertex buffer, free GPU memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBuffer);
}

void main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitWindowSize(400, 400);
    glutCreateWindow("Rotate 3D Letter");
    glewInit();
	program = GLSL::LinkProgramViaCode(vertexShader, pixelShader);
    InitVertexBuffer();
    glutDisplayFunc(Display);
	glutMouseFunc(MouseButton);
	glutMotionFunc(MouseDrag);
    glutCloseFunc(Close);
    glutMainLoop();
}
