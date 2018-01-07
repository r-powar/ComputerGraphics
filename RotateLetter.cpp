// SimplestTriangle.cpp
// simplest program to draw colored triangle with GLSL and vertex buffering

#include "glew.h"
#include "freeglut.h"
#include "GLSL.h"
#include <vector>
#include <time.h>
// GPU identifiers
GLuint vBuffer = 0;
GLuint program = 0;

time_t startTime = clock();
static float degPerSec = 30;

char *vertexShader = "\
	#version 150								\n\
	in vec2 vPoint;							\n\
	in vec3 vColor;								\n\
	out vec4 color;								\n\
	uniform float radAng = 0; \n\
	vec2 Rotate2D (vec2 v){ \n\
		float xrotated = (cos(radAng)* v.x) - (sin(radAng)* v.y); \n\
		float yrotated = (sin(radAng)* v.x) + (cos(radAng)* v.y); \n\
		return vec2(xrotated, yrotated); \n\
	} \n\
	void main() {								\n\
		vec2 r = Rotate2D(vPoint); \n\
		gl_Position = vec4(r, 0, 1); \n\
	    color = vec4(vColor, 1);				\n\
	}";

char *pixelShader = "\
	#version 150								\n\
	in vec4 color;								\n\
	out vec4 fColor;							\n\
	void main() {								\n\
        fColor = color;							\n\
	}";

class Vertex {
public:
	vec2 point;
	vec3 color;
	Vertex() {}
	Vertex(float* p, float *c) {
		point = vec2(p[0], p[1]);
		color = vec3(c[0], c[1], c[2]);
	}

};

std::vector<Vertex> vertices;

void InitVertexBuffer() {

	vec2 f(200);
	//vertices
	float  points[][2] = { { 0, 0 },{ -.5f, -1 },{ -.5f,   1 },{ .5f, 1 },{ .75f, .75f },
	{ .75f, .25f },{ .5f, 0 },{ 0,  -1 },{ -.5f,  0 },{ 0, 1 } };

	float  colors[][3] = { { 1, 1, 1 },{ 1, 0, 0 },{ 0.5f, 0, 0 },{ 1, 1, 0 },{ 0.5f, 1, 0 },
	{ 0, 1, 0 },{ 0, 1, 1 },{ 0, 0, 1 },{ 1, 0, 1 },{ 0.5f, 0, 0.5f } };

	//triangles
	int triangles[][3] = { { 0, 1, 8 },{ 0, 1, 7 },{ 0, 8, 2 },{ 0, 2, 9 },{ 0, 9, 3 },{ 0, 3, 4 },
	{ 0, 4, 5 },{ 0, 5, 6 } };

	int ntriangles = sizeof(triangles) / (3 * sizeof(int)), nverts = 3 * ntriangles;
	vertices.resize(nverts);

	for (int i = 0; i < ntriangles; i++) {
		for (int k = 0; k < 3; k++) {
			int vid = triangles[i][k];
			vertices[3 * i + k] = Vertex(points[vid], colors[vid]);
		}
	}
	// create a vertex buffer for the array, and make it the active vertex buffer
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);	// to be a vertex array buffer
											// allocate buffer memory to hold vertex locations and colors
	glBufferData(GL_ARRAY_BUFFER, nverts * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
	// load data to the GPU
	//glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
	// start at beginning of buffer, for length of points array
	//glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors);
	// start at end of points array, for length of colors array
}

void Display() {
	glClearColor(.5, .5, .5, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(program);
	// associate position input to shader with position array in vertex buffer 
	GLSL::VertexAttribPointer(program, "vPoint", 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
	// associate color input to shader with color array in vertex buffer
	GLSL::VertexAttribPointer(program, "vColor", 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) sizeof(vec2));

	float dt = (float)(clock() - startTime) / CLOCKS_PER_SEC;
	GLSL::SetUniform(program,"radAng", (3.1415f/180.f)*dt*degPerSec);
	// finally, render three vertices as a triangle
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
	glFlush();
}

void Idle() {
	glutPostRedisplay();
}

void Close() {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBuffer);
}

void main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitWindowSize(400, 400);
	glutCreateWindow("Simplest Buffered GLSL Triangle");
	glewInit();
	InitVertexBuffer();
	program = GLSL::LinkProgramViaCode(vertexShader, pixelShader);
	glutIdleFunc(Idle);
	glutDisplayFunc(Display);
	glutCloseFunc(Close);
	glutMainLoop();
}


