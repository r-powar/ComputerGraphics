// BezierCurve.cpp - interactive curve design

#include "glew.h"
#include "freeglut.h"
#include "mat.h"
#include <math.h>
#include <time.h>
#include "UI.h"

// Bezier class

class Bezier {
public:
	int res;				// display resolution
	vec3 p1, p2, p3, p4;	// control points
	Bezier(vec3 &p1, vec3 &p2, vec3 &p3, vec3 &p4, int res = 50) : p1(p1), p2(p2), p3(p3), p4(p4), res(res) { }
	vec3 Point(float t) {
		// return a point on the Bezier curve given parameter t, in (0,1)
		vec3 pint;
		pint = (-pow(t, 3) + 3 * (pow(t, 2)) - 3 * t + 1)*p1
			+ (3 * pow(t, 3) - 6 * pow(t, 2) + 3 * t)*p2
			+ (-3 * pow(t, 3) + 3 * pow(t, 2))*p3
			+ pow(t, 3)*p4;
		return pint;
	}

	void Draw(vec3 color) {
		// break the curve into res number of straight pieces, render each with Line()
		for (int i = 0; i < res; i++) {
			float t1 = i / (float) res;
			float t2 = (i + 1) / (float) res;
			Line(Point(t1), Point(t2), color);
		}	
	}

	void DrawControlMesh(vec3 pointColor, vec3 meshColor, float opacity, float width) {
		// draw the four control points and the mesh that connects them
		Line(p1, p2, pointColor, opacity, width, false);
		Line(p2, p3, pointColor, opacity, width, false);
		Line(p3, p4, pointColor, opacity, width, false);
	}

	vec3 *PickPoint(int x, int y, mat4 view) {
		// return pointer to nearest control point, if within 10 pixels of mouse (x,y), else NULL
		
		float dist1 = sqrt(ScreenDistSq(x, y, p1, view));
		float dist2 = sqrt(ScreenDistSq(x, y, p2, view));
		float dist3 = sqrt(ScreenDistSq(x, y, p3, view));
		float dist4 = sqrt(ScreenDistSq(x, y, p4, view));

		if (dist1 <= 10 && dist1 <= dist2 && dist1 <= dist3 && dist1 <= dist4) {
			return &p1;
		}
		else if(dist2 <= 10 && dist2 <= dist3 && dist3 <= dist4){
			return &p2;
		}
		else if (dist3 <= 10 && dist3 <= dist4) {
			return &p3;
		}
		else if (dist4 <= 10) {
			return &p4;
		}
		else {
			return NULL;
		}
		
	}
};

Bezier curve(vec3(-0.75, 0, 0), vec3(-0.5, 0.75, 0), vec3(0.5, 0.75, 0), vec3(0.75, 0, 0));

// Display

mat4	view;
vec2	rotOld, rotNew;	// rot.x is about Y-axis, rot.y is about X-axis

void Display() {
    // background, blending, zbuffer
    glClearColor(.6f, .6f, .6f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
	// update transformations, enable UI draw shader
	mat4 ortho = Ortho(-1, 1, -1, 1, -.01f, -10);
	view = ortho*Translate(0, 0, 1)*RotateY(rotNew.x)*RotateX(rotNew.y);
	UseDrawShader(view); // no shading, so single matrix
	// curve and mesh
	curve.Draw(vec3(.7f, .2f, .5f));
	curve.DrawControlMesh(vec3(0, .4f, 0), vec3(1, 1, 0), 1, 1.5f);
    glFlush();
}

// Mouse

int		winWidth = 630, winHeight = 500;
int		xMouseDown, yMouseDown;				// for each mouse down, need start point
bool 	cameraDown = false;
Mover	ptMover;

void MouseButton(int butn, int state, int x, int y) {
    y = glutGet(GLUT_WINDOW_HEIGHT)-y;						// invert y for upward-increasing screen space
    if (state == GLUT_UP) {
		if (cameraDown)
			rotOld = rotNew;				// reset rotation accumulation
		else if (ptMover.point)
			ptMover.Set(NULL);				// deselect control point
	}
	cameraDown = false;
	if (state == GLUT_DOWN) {
		vec3 *pp = curve.PickPoint(x, y, view);
		if (pp) {
			if (butn == GLUT_LEFT_BUTTON) { // pick control point
				ptMover.Set(pp);
				ptMover.Down(x, y, view);
			}
		}
		else {
			cameraDown = true;				// set mouse reference
			xMouseDown = x;
			yMouseDown = y;
		}
	}
    glutPostRedisplay();
}

void MouseDrag(int x, int y) {
    y = glutGet(GLUT_WINDOW_HEIGHT)-y;
    if (ptMover.point)
        ptMover.Drag(x, y, view);
	else if (cameraDown)
		rotNew = rotOld+.3f*(vec2((float)(x-xMouseDown), (float)(y-yMouseDown)));
    glutPostRedisplay();
}

// Application

int main(int ac, char **av) {
    glutInit(&ac, av);
    glutInitWindowSize(winWidth, winHeight);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Bezier Curve");
	glewInit();
    glutDisplayFunc(Display);
    glutMouseFunc(MouseButton);
    glutMotionFunc(MouseDrag);
    glutMainLoop();
}
