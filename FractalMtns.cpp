// FractalMtns.cpp - textured displacement surface using tessellation shader

#include "glew.h"
#include "freeglut.h"
#include "GLSL.h"
#include "UI.h"
#include "mat.h"

// colors
vec3	blk(0), wht(1), cyan(0,1,1);

// view
vec2	mouseDown, rotOld, rotNew;						// previous, current rotations
mat4	rotM;											// MouseDrag sets, Display uses
float	dolly = -10;
mat4	modelview, persp, fullview, screen;				// camera matrices

// UI
void   *picked = NULL, *hover = NULL;
vec3	light(-.2f, .4f, .8f);
Mover	lightMover(&light);
Slider	scale(30, 20, 70, 0, 1, .4f, true, "scl", &wht);
bool	facetedShading = 0, fieldOption = 0;
Button	faceted(80, 20, 15, "", &facetedShading, &wht);
Button	option(80, 50, 15, "", &fieldOption, &wht);

// shader
GLuint	shader = 0, textureId = 0;

char *vertexShaderCode = "void main() { gl_Position = vec4(0); }"; // no-op
		
char *tessEvalShaderCode = "\
	#version 400 core														\n\
	layout (quads, equal_spacing, ccw) in;									\n\
	out vec3 point;															\n\
	out vec3 normal;														\n\
	uniform int option = 2; // 0: waves, 1: fractals						\n\
	uniform float heightScale;												\n\
	uniform sampler2D heightField;											\n\
    uniform mat4 modelview;													\n\
	uniform mat4 persp;														\n\
	vec3 PtFromFractal(float s, float t) {									\n\
		float height = texture(heightField, vec2(s, t)).r;					\n\
		return vec3(2*s-1, 2*t-1, heightScale*height);						\n\
	}																		\n\
	vec3 PtFromWaves(float s, float t) {									\n\
		float h = sin(10*s)*sin(26*t)/max(.25, s);							\n\
		return vec3(2*s-1, 2*t-1, heightScale*h);							\n\
	}																		\n\
	vec3 PtFromField(float s, float t) {									\n\
		return option == 1? PtFromWaves(s, t) : PtFromFractal(s, t);		\n\
	}																		\n\
	void main() {															\n\
		float delta = .001;													\n\
		float s = gl_TessCoord.st.s, t = gl_TessCoord.st.t;					\n\
		vec3 p = PtFromField(s, t);											\n\
		float sBef = s-delta, sAft = s+delta;								\n\
		float tBef = t-delta, tAft = t+delta;								\n\
		vec3 a = sBef < 0? PtFromField(sAft, t)-p :							\n\
				 sAft > 1? p-PtFromField(sBef, t) :							\n\
						   PtFromField(sAft, t)-PtFromField(sBef, t);		\n\
		vec3 b = tBef < 0? PtFromField(s, tAft)-p :							\n\
				 tAft > 1? p-PtFromField(s, tBef) :							\n\
						   PtFromField(s, tAft)-PtFromField(s, tBef);		\n\
		vec3 n = normalize(cross(a, b));									\n\
		point = (modelview*vec4(p, 1)).xyz;									\n\
		normal = -(modelview*vec4(n, 0)).xyz;								\n\
		gl_Position = persp*vec4(point, 1);									\n\
	}";

char *pixelShaderCode = "\
    #version 400 core														\n\
	in vec3 point;															\n\
	in vec3 normal;															\n\
	out vec4 pColor;														\n\
	uniform vec3 light;														\n\
	uniform vec4 color = vec4(.5, .5, .5, 1);		// default grey			\n\
	uniform int faceted = 0;												\n\
    void main() {															\n\
		vec3 n = faceted == 1? cross(dFdy(point), dFdx(point)) : normal;	\n\
		vec3 N = normalize(n);												\n\
        vec3 L = normalize(light-point);			// light vector			\n\
        vec3 E = normalize(point);					// eye vertex			\n\
        vec3 R = reflect(L, N);						// highlight vector		\n\
		float dif = abs(dot(N, L));                 // one-sided diffuse	\n\
		float spec = pow(max(0, dot(E, R)), 50);							\n\
		float amb = .15, ad = clamp(amb+dif, 0, 1);							\n\
		pColor = vec4(ad*color.rgb+spec*color.rgb, 1);						\n\
	}";

// display

void Display() {
    // background, blending, zbuffer
    glClearColor(.6f, .6f, .6f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
	// compute transformation matrices
	modelview = Translate(0, 0, dolly)*rotM;
	float fov = 15, nearPlane = -.001f, farPlane = -500;
	float aspect = (float) glutGet(GLUT_WINDOW_WIDTH) / (float) glutGet(GLUT_WINDOW_HEIGHT);
	persp = Perspective(fov, aspect, nearPlane, farPlane);
	fullview = persp*modelview;
	screen = ScreenMode();
	// use tessellation shader
	glUseProgram(shader);
	// update uniforms
	GLSL::SetUniform(shader, "heightScale", scale.GetValue());
	GLSL::SetUniform(shader, "heightField", (int) textureId);
	GLSL::SetUniform(shader, "faceted", (int) facetedShading);
	GLSL::SetUniform(shader, "option", (int) fieldOption);
	GLSL::SetUniform(shader, "modelview", modelview);
	GLSL::SetUniform(shader, "persp", persp);
	vec4 hLight = modelview*vec4(light, 1);
	GLSL::SetUniform3v(shader, "light", 1, (float *) &hLight);
	// tessellate quad
	glPatchParameteri(GL_PATCH_VERTICES, 4);
	float r = 1000, outerLevels[] = {r, r, r, r}, innerLevels[] = {r, r};
	glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, outerLevels);
	glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, innerLevels);
	glDrawArrays(GL_PATCHES, 0, 4);
	// draw widgets in screen space
	UseDrawShader(screen);
	if (IsVisible(light, fullview))
		Sun(ScreenPoint(light, fullview), hover == &light? &cyan : NULL);
	glDisable(GL_DEPTH_TEST);
	scale.Draw();
	faceted.Draw(facetedShading? "faceted" : "smooth");
	option.Draw(fieldOption? "waves" : "fractals");
    glFlush();
}

void SetHeightfield() {
	class Helper { public:
		int width, height; // size of terrain grid (wPow2,hPow2 if for texture map)
		char *pixels;
		Helper(int wPow2 = 256, int hPow2 = 256) : width(wPow2+1), height(hPow2+1) { pixels = new char[wPow2*hPow2]; }
		~Helper() { delete [] pixels; }
		float Random(float min, float max) { return min+(((float) rand())/RAND_MAX)*(max-min); }
		void Set(int x, int y, float val) {
			if (x < width-1 && y < height-1) {
				// unsigned int v = val < 0? 0 : val > 255? 255 : (unsigned int) val;
				pixels[x+(width-1)*y] = (int) (val < 0? 0 : val > 255? 255 : val); // v;
			}
		}
		void RecurseMidpoint(int x1, int x2, int y1, int y2, float v11, float v12, float v22, float v21, float range) {
			// v11: lower left, v12: upper left, v22: upper right, v21: lower right
			int xm = (x1+x2)/2, ym = (y1+y2)/2;
			float vm1 = (v11+v21)/2+Random(-range, range);			// lo mid
			float vm2 = (v12+v22)/2+Random(-range, range);			// hi mid
			float v1m = (v11+v12)/2+Random(-range, range);			// left mid
			float v2m = (v21+v22)/2+Random(-range, range);			// right mid
			float vmm = (vm1+vm2+v1m+v2m)/4+Random(-range, range);	// mid mid
			Set(x1, ym, v1m);
			Set(x2, ym, v2m);
			Set(xm, y1, vm1);
			Set(xm, y2, vm2);
			Set(xm, ym, vmm);
			if (x2-x1 > 2 || y2-y1 > 2) {
				RecurseMidpoint(x1, xm, y1, ym, v11, v1m, vmm, vm1, range/2); // lower left quadrant
				RecurseMidpoint(x1, xm, ym, y2, v1m, v12, vm2, vmm, range/2); // upper left
				RecurseMidpoint(xm, x2, ym, y2, vm1, vmm, v2m, v21, range/2); // lower right
				RecurseMidpoint(xm, x2, y1, ym, vmm, vm2, v22, v2m, range/2); // upper right
			}
		}
	};
	int wPow2 = 256, hPow2 = 256;
	Helper h(wPow2, hPow2);
	float v11 = h.Random(0, 255), v12 = h.Random(0, 255), v21 = h.Random(0, 255), v22 = h.Random(0, 255);
	h.Set(0, 0, v11);
	h.RecurseMidpoint(0, wPow2, 0, hPow2, v11, v12, v22, v21, 0);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);			// in case width not multiple of 4
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, wPow2, hPow2, 0, GL_RED, GL_UNSIGNED_BYTE, h.pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
}

// mouse

void MouseOver(int x, int y) {
	int width = glutGet(GLUT_WINDOW_WIDTH), height = glutGet(GLUT_WINDOW_HEIGHT);
	void *wasHover = hover;
	hover = NULL;
	if (ScreenDistSq(x, height-y, light, fullview) < 100)
		hover = (void *) light;
	if (hover != wasHover)
		glutPostRedisplay();
}

void MouseButton(int butn, int state, int x, int y) {
    y = glutGet(GLUT_WINDOW_HEIGHT)-y;
    if (state == GLUT_UP) {
		if (picked == &rotM)
			rotOld = rotNew;
	}
	faceted.UpHit(x, y, state);
	option.UpHit(x, y, state);
	picked = NULL;
	if (state == GLUT_DOWN) {
		if (ScreenDistSq(x, y, light, fullview) < 100) {
			picked = &light;
			lightMover.Down(x, y, modelview, &persp);
		}
		else if (scale.Hit(x, y))
			picked = &scale;
		else if (faceted.Hit(x, y))
			picked = &faceted;
		else if (option.Hit(x, y))
			picked = &option;
		if (!picked) {
			picked = &rotM;
			mouseDown = vec2((float) x, (float) y);
		}
	}
    glutPostRedisplay();
}

void MouseDrag(int x, int y) {
    y = glutGet(GLUT_WINDOW_HEIGHT)-y;
	if (picked == &light)
		lightMover.Drag(x, y, modelview, &persp);
	if (picked == &scale)
		scale.Mouse(x, y);
	if (picked == &rotM) {
		rotNew = rotOld+.3f*(vec2((float) x, (float) y)-mouseDown);
			// new rotations depend on old plus mouse distance from mouseDown
		rotM = RotateY(rotNew.x)*RotateX(rotNew.y);
			// rot.x is about Y-axis, rot.y is about X-axis
	}
    glutPostRedisplay();
}

void MouseWheel(int wheel, int direction, int x, int y) {
	dolly += (direction > 0? -.1f : .1f);
	glutPostRedisplay();
}

// application

void Reshape(int w, int h) {
	glViewport(0, 0, w, h);
	glutPostRedisplay();
}

int MakeShaderProgram() {
	int vShader = GLSL::CompileShaderViaCode(vertexShaderCode, GL_VERTEX_SHADER);
	int teShader = GLSL::CompileShaderViaCode(tessEvalShaderCode, GL_TESS_EVALUATION_SHADER);
	int pShader = GLSL::CompileShaderViaCode(pixelShaderCode, GL_FRAGMENT_SHADER);
	int program = glCreateProgram();
    glAttachShader(program, vShader);
    glAttachShader(program, pShader);
	glAttachShader(program, teShader);
    glLinkProgram(program);
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
        GLSL::PrintProgramLog(program);
	return program;
}

void main(int ac, char **av) {
    // init app window
    glutInit(&ac, av);
    glutInitWindowSize(930, 800);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("height Field Texture Map Demo");
	GLenum err = glewInit();
	if (err != GLEW_OK)
        printf("Error initializing GLEW: %s\n", glewGetErrorString(err));
	shader = MakeShaderProgram();
	if (!shader) {
		printf("Can't link shader program\n");
		getchar();
		return;
	}
	SetHeightfield();
    // callbacks
    glutDisplayFunc(Display);
    glutMouseFunc(MouseButton);
    glutMotionFunc(MouseDrag);
	glutMouseWheelFunc(MouseWheel);
	glutPassiveMotionFunc(MouseOver);
    glutReshapeFunc(Reshape);
    glutMainLoop();
}
