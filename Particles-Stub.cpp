// Particles.cpp - demonstrate ballistic particles
//	ALTERNATIVES
//		A. flash/blink before a particle dies
//		B. a particle should bounce when it hits the ground
//		C. it should bounce when it hits some object
//		D. it should create new particles when it dies
//		E. when it hits the ground, it should slide across and then come to a stop
//		F. play with the emit rate and/or the lifetime
//		G. display the particle with transparency

#include "glew.h"
#include "freeglut.h"
#include <time.h>
#include "GLSL.h"
#include "UI.h"

#define PI 3.141592f

GLuint	shaderId = 0;
GLuint	cylBufferId = 0;
float	gravity = 1;
float	ground = -.5f;
Button	reset(20, 20, 100, 20, "Reset");
vec3	lightSource(2, 2, -6);
mat4	view, persp;
vec2	mouseDown;							// for each mouse down, need start point
vec2	rotOld(-190,-20), rotNew(rotOld);	// previous, current rotations, rot.x is about Y-axis, rot.y is about X-axis
float	dolly = -8;

struct Vertex {
	vec3 point, normal;
	Vertex() { }
	Vertex(vec3 &p, vec3 &n) : point(p), normal(n) { }
};

// Shaders

char *vertexShader = "\
	#version 130														\n\
	in vec3 point;														\n\
	in vec3 normal;														\n\
	out vec3 vPoint;													\n\
	out vec3 vNormal;													\n\
    uniform mat4 view;													\n\
	uniform mat4 persp;													\n\
	void main() {														\n\
		vPoint = (view*vec4(point, 1)).xyz;								\n\
		gl_Position = persp*vec4(vPoint, 1);							\n\
		vNormal = (view*vec4(normal, 0)).xyz;							\n\
	}";

char *pixelShader = "\
    #version 130														\n\
	in vec3 vPoint;														\n\
	in vec3 vNormal;													\n\
	out vec4 pColor;													\n\
	uniform vec4 color = vec4(1,1,1,1);									\n\
	uniform vec3 light;													\n\
    void main() {														\n\
		vec3 N = normalize(vNormal);          // surface normal			\n\
        vec3 L = normalize(light-vPoint);     // light vector			\n\
        vec3 E = normalize(vPoint);           // eye vector				\n\
        vec3 R = reflect(L, N);			      // highlight vector		\n\
        float d = abs(dot(N, L));             // two-sided diffuse		\n\
        float s = abs(dot(R, E));             // two-sided specular		\n\
		float intensity = clamp(d+pow(s, 50), 0, 1);					\n\
		pColor = vec4(intensity*color.rgb, color.a);					\n\
	}";

// Misc

float Lerp(float a, float b, float alpha) { return a+alpha*(b-a); }

float Blend(float x) {
    // cubic blend function f: f(0)=1, f(1)=0, f'(0)=f'(1)=0
    x = abs(x);
    float x2 = x*x, x4 = x2*x2;
    return x < DBL_EPSILON? 1 : x > 1? 0 : (-4.f/9.f)*x2*x4+(17.f/9.f)*x4+(-22.f/9.f)*x2+1;
};

float Random() { return (float) (rand()%1000)/1000.f; }
    // return a random number between 0 and 1
    
float Random(float a, float b) { return Lerp(a, b, Random()); }
    // return a random number between a and b
	
// Cylinder

void MakeCylinderVertexBuffer() {
	class Helper { public:
		int vCount;
		Vertex verts[288]; // 24 circumferential pts, 4 triangles each: 4X24X3 = 288
		void Triangle(vec3 p1, vec3 n1, vec3 p2, vec3 n2, vec3 p3, vec3 n3) {
			verts[vCount++] = Vertex(p1, n1);
			verts[vCount++] = Vertex(p2, n2);
			verts[vCount++] = Vertex(p3, n3);
		}
		Helper() { vCount = 0; }
	} h;
	vec3 pBot = vec3(0), pTop = vec3(0, 1, 0), nBot(0, -1, 0), nTop(0, 1, 0);
    for (int i1 = 0; i1 < 24; i1++) {
		int i2 = (i1+1)%24;
        float a1 = 2*PI*(float)i1/24, x1 = cos(a1), z1 = sin(a1);
        float a2 = 2*PI*(float)i2/24, x2 = cos(a2), z2 = sin(a2);
		vec3 n1(x1, 0, z1), p1Bot(pBot+n1), p1Top(pTop+n1);
		vec3 n2(x2, 0, z2), p2Bot(pBot+n2), p2Top(pTop+n2);
		h.Triangle(pBot, nBot, p1Bot, nBot, p2Bot, nBot); // bottom wedge
		h.Triangle(pTop, nTop, p1Top, nTop, p2Top, nTop); // top wedge
		h.Triangle(p1Bot, n1, p1Top, n1, p2Top, n2);		// side top
		h.Triangle(p2Top, n2, p2Bot, n2, p1Bot, n1);		// side bottom
    }
	glGenBuffers(1, &cylBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, cylBufferId);
	glBufferData(GL_ARRAY_BUFFER, sizeof(h.verts), h.verts, GL_STATIC_DRAW);
}

class Cylinder {
public:
    float height, radius;
	vec3 color, location;
    Cylinder() { height = radius = 0; }
	Cylinder (float h, float r, vec3 c, vec3 l) : height(h), radius(r), color(c), location(l) { }
    bool Inside(float p[3]) {
		if (p[1] > ground+height)
			return false;
		float dx = p[0]-location.x, dz = p[2]-location.z;
		return dx*dx+dz*dz < radius*radius;
	}
    void Draw() {
		glBindBuffer(GL_ARRAY_BUFFER, cylBufferId);
		GLSL::VertexAttribPointer(shaderId,  "point", 3,  GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) 0);
		GLSL::VertexAttribPointer(shaderId, "normal", 3,  GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) sizeof(vec3));
		GLSL::SetUniform(shaderId, "color", vec4(color.x, color.y, color.z, 1));
		mat4 m = view*Translate(location)*Scale(radius, height, radius);
		GLSL::SetUniform(shaderId, "view", m);
		glDrawArrays(GL_TRIANGLES, 0, 288); // suspect
    }
};

float height = 1, radius = 1;

Cylinder cylinders[] = {
	Cylinder(height/2, radius/4, vec3(1, .7f, 0), vec3(-.3f, ground, .6f)),
	Cylinder(height/3, radius/2, vec3(0, 0, .7f), vec3(.3f, ground, -.2f)),
	Cylinder(height/4, radius/3, vec3(0, .7f, 0), vec3(.2f, ground, -.7f))
};
int nCylinders = sizeof(cylinders)/sizeof(Cylinder);

// Particle

class Particle {
public:
    int		level;		// first generation is level 0
	bool	grounded;   // has particle returned to earth?
	clock_t	birth;      // when instantiated
	float	lifetime;   // duration of activity
	float	speed;      // speed of motion per unit time
	float	size;       // in pixels
	float	emitRate;   // sub-particle emission rate if grounded
	vec3	position;	// current location in 3D
	vec3	velocity;	// current linear 3D direction
	vec3	color;		// r, g, b, alpha
	clock_t prevEmit;   // when last spawned another particle
    void Init(int l, float lt, float s, float sz, float er) {
		grounded = false;
		birth = clock();
		level = l;
		lifetime = lt;
		speed = s;
		size = sz;
		emitRate = er;
	}
    void Move(float deltaTime) {
		// update vertical component of velocity
		velocity[1] -= gravity * deltaTime;
		normalize(velocity);
		/* this particle is moving in the direction defined by vec3 velocity
		according to Newton, the particle will continue in this direction unless some force
		is acting on it -- what force could that be, Isaac? (sound of apple falling)
		so, how does the force of gravity act upon a particle over the small period of deltaTime?
		well, gravity is acceleration, and if we integrate it over time it represents a change in velocity
		(and if we integrate velocity over time we get a change in position)
		so, what's an easy way to integrate acceleration over time?
			1) gravity acts downwards, so it should decrease the vertical ('y') component of velocity
			2) this acts over time, the longer is deltaTime, the greater should be the decrease */

		// update position

		/* that is, compute a new location for vec3 position based upon the current location, the velocity,
		and deltaTime */
		
		/* notice there are two global variables: gravity and speed. you might incorporate them here, so
		that if you wish to later speed-up/slow-down or increase/decrease the force of gravity, you can do
		so by changing the globals. */
		for (int i = 0; i < 3; i++) {
			position[i] = position[i] + (speed * deltaTime * velocity[i]);
		}
        // bounce against cylinders
        for (int c = 0; c < nCylinders; c++) {
            Cylinder &cyl = cylinders[c];
            if (cyl.Inside(position) && velocity[1] < 0) {
                position[1] = cyl.height;
                velocity[1] = -.5f*velocity[1];
                break;
            }
        }
    }
	void Update(float deltaTime) {
		// move ungrounded particle
        if (!grounded) {
            Move(deltaTime);
			// test for grounded
			if (position[1] <= ground) {
				position[1] = ground;
				grounded = true;
                prevEmit = clock();
			}
		}
	}
	void Draw() {
		Disk(position, size, color);
	}
};

// Emitter

#define MAX_PARTICLES 5000

class Emitter {
public:
	clock_t  prevTime;                 // needed to compute delta time
	clock_t  nextEmitTime;             // to control particle emissions
	Particle minParticle;              // minimum values for position, size, etc
    Particle maxParticle;              // maximum values
	Particle particles[MAX_PARTICLES]; // array of particles
	int      nparticles;               // # elements in array
	Emitter() {
		prevTime = clock();
		nparticles = 0;
		nextEmitTime = 0;
		srand((int) time(NULL));
	}
	void CreateParticle(int level = 0, float *pos = NULL, float *col = NULL) {
		// create new particle randomly between minParticle and maxParticle
        if (nparticles < MAX_PARTICLES) {
		    Particle &p = particles[nparticles++];
            float b = Blend(level/10.f);
            float lifetime = Lerp(minParticle.lifetime, maxParticle.lifetime, b*Random());
            float speed =    Lerp(minParticle.speed,    maxParticle.speed,    b*Random());
            float size =     Lerp(minParticle.size,     maxParticle.size,     b*Random());
            float emitRate = Lerp(minParticle.emitRate, maxParticle.emitRate, b*Random());
            p.Init(level, lifetime, speed, size, emitRate);
            // set position, using argument if given
            if (pos)
                memcpy(p.position, pos, 3*sizeof(float));
            else
                // use min/max particle position
                for (int i = 0; i < 3; i++)
                    p.position[i] = Random(minParticle.position[i], maxParticle.position[i]);
            // set color, using argument if given
            if (col)
                memcpy(p.color, col, 3*sizeof(float));
            else
                for (int k = 0; k < 3; k++)
                    p.color[k] = Random(minParticle.color[k], maxParticle.color[k]);
            // set velocity
            float azimuth = Random(0., 2.f*PI);
            float elevation = Random(0., PI/2.f);
            float cosElevation = cos(elevation);
            p.velocity[1] = sin(elevation);
            p.velocity[2] = cosElevation*sin(azimuth);
            p.velocity[0] = cosElevation*cos(azimuth);
        }
	}
	void Draw() {
		for (int i = 0; i < nparticles; i++)
			particles[i].Draw();
	}
    void Update() {
        // need delta time to regulate speed
		clock_t now = clock();
		float dt = (float) (now-prevTime)/CLOCKS_PER_SEC;
		prevTime = now;
		// delete expired particles
		for (int i = 0; i < nparticles;) {
			Particle &p = particles[i];
            if (now > p.birth+p.lifetime*CLOCKS_PER_SEC) {
                // delete particle
		        if (i < nparticles-1)
		            particles[i] = particles[nparticles-1];
	            nparticles--;
            }
			else
				i++;
		}
		// update ungrounded particles
		for (int k = 0; k < nparticles; k++) {
			Particle &p = particles[k];
            if (p.grounded) {
				float dt = (float) (now-p.prevEmit)/CLOCKS_PER_SEC;
				if (dt > 1./p.emitRate) {

				    // spawn new particle
                    CreateParticle(p.level+1, p.position, p.color);
					p.prevEmit = now;
				}
			}
			else
                p.Update(dt);
		}
		// possibly emit new particle
        if (now > nextEmitTime) { 
			CreateParticle();
			float randomBoundedEmitRate = Random(minParticle.emitRate, maxParticle.emitRate);
			nextEmitTime = now+(clock_t)((float)CLOCKS_PER_SEC/randomBoundedEmitRate);
	  	}
	}
} emitter;

// Display

void Display(void) {
	// background, zbuffer, blend
    glClearColor(.65f, .5f, .5f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// shader, send matrices, send light
	glUseProgram(shaderId);
	float aspect = (float) glutGet(GLUT_WINDOW_WIDTH)/glutGet(GLUT_WINDOW_HEIGHT);
	persp = Perspective(15, aspect, -.001f, -500);
	view = Translate(0, 0, dolly)*RotateY(rotNew.x)*RotateX(rotNew.y);
	GLSL::SetUniform(shaderId, "view", view);
	GLSL::SetUniform(shaderId, "persp", persp);
	//glUniformMatrix4fv(glGetUniformLocation(shaderId, "view"), 1, true, (float *) &view[0][0]);
	//glUniformMatrix4fv(glGetUniformLocation(shaderId, "persp"), 1, true, (float *) &persp[0][0]);
	vec4 hLight = view*vec4(lightSource, 1);
	vec3 xlight(hLight.x, hLight.y, hLight.z);
	glUniform3fv(glGetUniformLocation(shaderId, "light"), 1, (float *) &xlight);
    // draw ground, cylinders, particles
    float g = 12, h = ground;
    float groundfloor[][3] = {{-g, h, -g}, {-g, h, g}, {g, h, g}, {g, h, -g}};
    for (int c = 0; c < nCylinders; c++)
        cylinders[c].Draw();
	UseDrawShader(persp*view);
	emitter.Draw();
    // draw buttons last
    ScreenMode();
    glDisable(GL_DEPTH_TEST);
	reset.Draw(NULL, NULL);
    Text(glutGet(GLUT_WINDOW_WIDTH)-100, 50, vec3(0), "%i particles", emitter.nparticles);
    // finish
    glFlush();
}

// Interactive Rotation

void MouseButton(int butn, int state, int x, int y) {
    y = glutGet(GLUT_WINDOW_HEIGHT)-y;
	if (state == GLUT_DOWN)
		mouseDown = vec2((float) x, (float) y);
	if (state == GLUT_UP)
		rotOld = rotNew;
	glutPostRedisplay();
}

void MouseDrag(int x, int y) {
	y = glutGet(GLUT_WINDOW_HEIGHT)-y;
	vec2 mouse((float) x, (float) y);
	rotNew = rotOld+.3f*(mouse-mouseDown);
//	printf("rot = (%5.4f, %5.4f), dolly = %5.4f\n", rotNew.x, rotNew.y, dolly);
	glutPostRedisplay();
}

void MouseWheel(int wheel, int direction, int x, int y) {
	dolly += (direction > 0? -.1f : .1f);
//	printf("rot = (%5.4f, %5.4f), dolly = %5.4f\n", rotNew.x, rotNew.y, dolly);
	glutPostRedisplay();
}

// Objects

void BuildObjects() {
	// initialize emitter
	Particle &min = emitter.minParticle;
    Particle &max = emitter.maxParticle;
    min.Init(-1, .15f, .1f, 5, 15); // level, lifetime, speed, size, emitRate
    max.Init(-1, 7.f, .4f, 9, 50);
    min.position = max.position = vec3(0, height, 0);
    min.color = vec3(0, 0, 0);
    max.color = vec3(1, 1, 1);
	MakeCylinderVertexBuffer();
	//for (int i = 0; i < nCylinders; i++)
	//	cylinders[i].Init();
}

// Application

void Idle(void) {
	emitter.Update();
	glutPostRedisplay();
}

void Close() {
	// unbind vertex buffer, free GPU memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	for (int i = 0; i < nCylinders; i++)
		glDeleteBuffers(1, &cylBufferId);
}

void main(int ac, char **av) {
	// init window
    glutInit(&ac, av);
    glutInitWindowSize(900, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow(av[0]);
	glewInit();
	// build, use shaderId program
	shaderId = GLSL::LinkProgramViaCode(vertexShader, pixelShader);
	if (!shaderId) {
		printf("Can't link shaderId program\n");
		getchar();
		return;
	}
	BuildObjects();
	// set glut callbacks
    glutDisplayFunc(Display);
    glutMouseFunc(MouseButton);
    glutMotionFunc(MouseDrag);
	glutMouseWheelFunc(MouseWheel);
    glutCloseFunc(Close);
	glutIdleFunc(Idle);
	// begin event handling
    glutMainLoop();
}
