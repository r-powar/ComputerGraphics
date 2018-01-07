// WidgetX.cpp - Buttons, Sliders, Movers, Typescripts - from 2012
// copyright (c) Jules Bloomenthal, 2012-2016, all rights reserved

#include "Draw.h"
#include "Widget.h"
#include "freeglut.h"

static float blk[] = {0, 0, 0}, wht[] = {1, 1, 1};
float offWht[] = {240.f/255.f, 240.f/255.f, 240.f/255.f};
float ltGry = 227.f/255.f, mdGry = 160.f/255.f, dkGry = 105.f/255.f;
float ltGryV[] = {ltGry, ltGry, ltGry}, mdGryV[] = {mdGry, mdGry, mdGry}, dkGryV[] = {dkGry, dkGry, dkGry};

// Keyboard

static int nShortBits = 8*sizeof(SHORT);
static SHORT shortMSB = 1 << (nShortBits-1);

bool KeyUp(int button) {
  // is the key presently up?
  return (GetKeyState(button) & shortMSB) == 0;
}

bool KeyDown(int c) {
	// is the key presently down?
	return (GetKeyState(c) & shortMSB) != 0;
}

// Titles

Header::Header() { Init(0, 0, 0, 0, NULL, NULL); }

Header::Header(int x1, int y1, int x2, int y2, char *title, float *textColor) {
	Init(x1, y1, x2, y2, title, textColor);
}

bool Header::Within(int x, int y) { return x >= x1 && x <= x2 && y >= y1 && y <= y2; }

void Header::Draw() {
	int strwid = 9*(1+strlen(title));
	int w = x2-x1+1, h = y2-y1+1;
	if (title && *title) {
		int margin = (w-strwid)/2;
		vec3 c (textColor[0], textColor[1], textColor[2]);
		Text(x1+margin+4, y2-4, c, title);
		Rectangle(x1, y2, margin, 1, mdGryV);         // left-top
		Rectangle(x2-margin, y2, margin, 1, mdGryV);  // right-top
		Rectangle(x1+1, y2-1, margin-1, 1, wht);      // left-top
		Rectangle(x2-margin, y2-1, margin-1, 1, wht); // right-top
	}
	else {
		Rectangle(x1, y2, w, 1, mdGryV);
		Rectangle(x1+1, y2-1, w-2, 1, wht);
	}
	Rectangle(x1, y1, 1, h, mdGryV);                  // left
	Rectangle(x2-1, y1+1, 1, h-1, mdGryV);            // right
	Rectangle(x1, y1+1, w, 1, mdGryV);                // bottom
	Rectangle(x1+1, y1+2, 1, h-3, wht);               // left
	Rectangle(x2, y1, 1, h, wht);                     // right
	Rectangle(x1, y1, w, 1, wht);                     // bottom
}

void Header::Init(int _x1, int _y1, int _x2, int _y2, char *_title, float *_textColor) {
	int slen = _title? strlen(_title) : 0;
	if (slen)
		memcpy(title, _title, slen+1);
	title[slen] = 0;
	x1 = _x1;
	y1 = _y1;
	x2 = _x2;
	y2 = _y2;
	if (_textColor)
		memcpy(textColor, _textColor, 3*sizeof(float));
	else
		textColor[0] = textColor[1] = textColor[2] = 0;
}

// Buttons

Button::Button() { InitRectangle(0, 0, 0, 0, NULL); }

Button::Button(int x, int y, int w, int h, char *name, float *bgrndCol, float *txtCol) {
	InitRectangle(x, y, w, h, name, bgrndCol, txtCol);
}

Button::Button(int x, int y, int size, char *name, bool *value, float *txtCol) {
	InitCheckbox(x, y, size, name, value, txtCol);
}

void InitButton(Button *b, char *name, float *bgrndCol, float *txtCol) {
	for (int i = 0; i < 3; i++) {
		b->backgroundColor[i] = bgrndCol? bgrndCol[i] : 1;
		b->textColor[i] = txtCol? txtCol[i] : 0;
	}
	b->textWidth = -1;
	b->font = GLUT_BITMAP_9_BY_15;
	name? b->name = string(name) : void();
}

void Button::InitCheckbox(int x, int y, int size, char *name, bool *valueA, float *txtCol) {
	this->x = x;
	this->y = y;
	w = h = size;
	winW = -1;
	type = B_Checkbox;
	value = valueA;
	InitButton(this, name, NULL, txtCol);
}

void Button::InitRectangle(int ax, int ay, int aw, int ah, char *name, float *bgrndCol, float *txtCol) {
	textWidth = -1;
	font = GLUT_BITMAP_9_BY_15;
	x = ax;
	y = ay;
	w = aw;
	h = ah;
	type = B_Rectangle;
	winW = -1;
	InitButton(this, name, bgrndCol, txtCol);
}

void Button::CenterText(int x, int y, char *text, int buttonWidth, float *color) {
	int size = FontSize(GLUT_BITMAP_9_BY_15);
	int xoff = (buttonWidth-size*strlen(text))/2;
	PutString(x+xoff, y, text, vec3(color[0], color[1], color[2]), font);
}

void Button::SetTextColor(float *col) { memcpy(textColor, col, 3*sizeof(float)); }

void Button::ShowName(char *name, float *color) {
	float yDotOffset = font == GLUT_BITMAP_TIMES_ROMAN_24? 6.5f : 4.f;
	float yRectOffset = font == GLUT_BITMAP_TIMES_ROMAN_24? 4.f : 6.f;
	int xpos = type == B_Rectangle? x+5 : x+w;
	int ypos = type == B_Checkbox? y-(int)yDotOffset : y+(int)yRectOffset;
	int nchars = strlen(name);
	int size = FontSize(font);
	bool singleLine = size*nchars <= w;
	vec3 col(color[0], color[1], color[2]);
	if (singleLine && h > 30)
		ypos += (h-20)/2;
	if (type == B_Checkbox) {
		char *cr = strchr(name, '\n'), buf[100];
		int len = cr? cr-name : 0;
		if (len) {
			strncpy(buf, name, len);
			buf[len] = 0;
			PutString(xpos, ypos+9, buf, col);
			PutString(xpos, ypos-5, cr+1, col);
			if (textWidth < 0) {
				int line1 = len, line2 = nchars-len-1;
				textWidth = 9*(line1 > line2? line1 : line2);
			}
		}
		else
			PutString(xpos, ypos, name, col);
	}
	else if (type == B_Rectangle && singleLine)
	    CenterText(x, ypos, name, w, color);
	else {
		char *sp = strchr(name, ' '), buf[100];
		int len = sp? sp-name : w/size;
		strncpy(buf, name, len);
		buf[len] = 0;
		if (sp != NULL) {
			// multi-line
			CenterText(x, ypos+15, buf, w, color);
			if (this->h >= 30) {
				char *s = name+len;
				while (*s == ' ')
					s++;
				CenterText(x, ypos, s, w, color);
			}
		}
		else
			CenterText(x, ypos+45, name, w, color);
	}
	if (textWidth < 0)
		textWidth = size*strlen(name);
}

void DrawDepressedBox(int x, int y, int w, int h, float *col) {
	if (col)
		Rectangle(x, y, w, h, col);
	else
		for (int i = 0; i < w; i++)
			for (int j = 0; j < h; j++)
				if ((i+j)%2)
					Rectangle(x+i, y+j, 1, 1, wht);
	Rectangle(x, y, w, 1, wht);
	Rectangle(x+w-1, y, 1, h, wht);
	Rectangle(x+1, y+1, w-2, 1, ltGryV);
	Rectangle(x+w-2, y+1, 1, h-2, ltGryV);
	Rectangle(x, y+1, 1, h-1, dkGryV);
	Rectangle(x, y+h-1, w-1, 1, dkGryV);
	Rectangle(x+1, y+2, 1, h-3, mdGryV);
	Rectangle(x+1, y+h-2, w-3, 1, mdGryV);
}

void DrawUnpressedBox(int x, int y, int w, int h, float *col) {
	if (col)
		Rectangle(x, y, w, h, col);
	Rectangle(x, y, w, 1, dkGryV);
	Rectangle(x+w-1, y+1, 1, h-1, dkGryV);
	Rectangle(x+1, y+1, w-2, 1, mdGryV);
	Rectangle(x+w-2, y+1, 1, h-2, mdGryV);
	Rectangle(x, y+1, 1, h-1, wht);
	Rectangle(x, y+h-1, w-1, 1, wht);
	Rectangle(x+1, y+2, 1, h-3, ltGryV);
	Rectangle(x+1, y+h-2, w-3, 1, ltGryV);
}

void DrawColoredBox(int x, int y, int w, int h, int id, vec3 &c) {
	float col[] = {(float)c.x, (float)c.y, (float)c.z};
	DrawDepressedBox(x, y, w, h, col);
	char buf[100];
	sprintf(buf, "%i", id);
	int textW = 9*strlen(buf);
	double luminance = 0.2126*c.x+0.7152*c.y+0.0722*c.z;
	Text(x+(w-textW)/2, y+8, luminance < .5f? vec3(1) : vec3(0), buf);
}

void DrawBlankBox(int x, int y, int w, int h) {
	Line(x+2, y+2, x+w-2, y+h-2, blk, blk, .5);
	Line(x+2, y+h-2, x+w-2, y+2, blk, blk, .5);
	DrawDepressedBox(x, y, w, h);
}

void Button::Draw(float *statusColor) { Draw(NULL, statusColor); }

void Button::Draw(char *nameOverride, float *statusColor) {
	// draw in clip (+/-1) space
	if (winW < 0) {
		winW = glutGet(GLUT_WINDOW_WIDTH);
		winH = glutGet(GLUT_WINDOW_HEIGHT);
	}
	glDisable(GL_BLEND);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_POINT_SMOOTH);
//	glDisable(GL_DEPTH_BUFFER);
	glDisable(GL_DEPTH_TEST);
	if (type == B_Rectangle) {
		float c[] = {1,0,0};
		Rectangle(x, y, w, h, offWht, true);
		statusColor? DrawDepressedBox(x, y, w, h) : DrawUnpressedBox(x, y, w, h);
	}
	if (type == B_Checkbox) {
		float ltGry[] = {.9f, .9f, .9f};
		float mdGry[] = {.6f, .6f, .6f};
		float dkGry[] = {.4f, .4f, .4f};
		int xx = x-5, yy = y-7, xEnd = xx+w, yEnd = yy+h;
		Rectangle(xx, yy, w+1, h+1, wht);
		Rectangle(xx, yy+1, 1, h-1, mdGry);
		Rectangle(xx, yEnd, w, 1, mdGry);
		Rectangle(xx+1, yy+2, 1, h-3, dkGry);
		Rectangle(xx+1, yEnd-1, w-2, 1, dkGry);
		Rectangle(xx+1, yy+1, w-2, 1, ltGry);
		Rectangle(xEnd-1, yy+1, 1, h-1, ltGry);
		if (statusColor) {
			int checkMark[7][7] = {0,0,1,0,0,0,0,
								   0,1,1,1,0,0,0,
								   1,1,1,1,1,0,0,
								   1,1,0,1,1,1,0,
								   1,0,0,0,1,1,1,
								   0,0,0,0,0,1,1,
								   0,0,0,0,0,0,1};
			for (int row = 0; row < 7; row++)
				for (int col = 0; col < 7; col++)
					if (checkMark[row][col] == 1) {
						int ix = xx+4+col, iy = yy+5+row;
						Rectangle(ix, iy, 1, 1, blk); // textColor); // blk);
					}
		}
	}
	ShowName(nameOverride? nameOverride : (char *) name.c_str(), textColor);
}

void Button::Highlight() {
	Rectangle(x, y, w, h, wht, true, .5f);
	glLineWidth(1.f);
	float x1 = (float)x, x2 = x1+(float)w, y1 = (float)y, y2 = y1+(float)h;
	Line(x1+1, y1+1.5f, x2-1, y1+1.5f, blk, blk, 1);
	Line(x2-1.5f, y2-1, x2-1.5f, y1+1, blk, blk, 1);
}

bool Button::Hit(int ax, int ay) {
	if (type == B_Rectangle)
	    return ax >= x && ax <= x+w && ay >= y && ay <= y+h;
	if (type == B_Checkbox)
		return  ax >= x-5 && ax <= (textWidth < 0? x-5+w : x-5+w+textWidth+7) && ay >= y-7 && ay <= y-7+w;
	return false;
}

bool Button::UpHit(int ax, int ay) {
	bool hit = Hit(ax, ay);
	if (hit && type == B_Checkbox && value != NULL)
		*value = !*value;
	return hit;
}

// Sliders

Slider::Slider() {
	x = y = size = 0;
	color[0] = color[1] = color[2] = 0;
	winW = -1;
	Init(0, 0, 80, 0, 1, .5f, Hor, NULL, color);
}

Slider::Slider(int x, int y, int size, float min, float max, float init, Orientation o, char *nameA, float *col) {
	Init(x, y, size, min, max, init, o, nameA, col);
}

int Round(float f)  { return (int)(f < 0.? ceil(f-.5f) : floor(f+.5f)); }

void Slider::Init(int x, int y, int size, float min, float max, float init, Orientation o, char *nameA, float *col) {
		this->x = x;
		this->y = y;
		this->size = size;
		if (nameA)
			name = string(nameA);
		orientation = o;
		SetRange(min, max, init);
		color[0] = color[1] = color[2] = 1;
		if (col)
			memcpy(color, col, 3*sizeof(float));
		else
			color[0] = color[1] = color[2] = 0;
	    winW = -1;
		int off = orientation == Hor? x : y;
		loc = Round(off+(float)(init-min)/(max-min)*size);
}

void Slider::SetValue(float val) {
	int off = orientation == Hor? x : y;
    loc = Round(off+(float)(val-min)/(max-min)*size);
}		

void Slider::SetRange(float min, float max, float init) {
	this->min = min;
	this->max = max;
	int off = orientation == Hor? x : y;
    loc = Round(off+(float)((init-min)/(max-min))*size);
}	

void Slider::Draw(char *nameOverride, float *sliderColor) {
	float *sCol = sliderColor? sliderColor : color;
	glLineWidth(2);
	int iloc = (int) loc;
	float grays[] = {160, 105, 227, 255};
	if (orientation == Hor) {
		for (int i = 0; i < 4; i++) {
			float g = grays[i]/255.f, col[] = {g, g, g};
			Rectangle(x, y-i+1, size, 1, col);
		}
		Rectangle(x+size-1, y-1, 1, 3, wht);
		Rectangle(x+size-2, y, 1, 1, ltGryV);
		Rectangle(x, y-1, 1, 3, mdGryV);
		// slider
		Rectangle(iloc-3, y-10, 7, 20, offWht);
		Rectangle(iloc+3, y-9, 1, 20, dkGryV);
		Rectangle(iloc-3, y-9, 1, 19, wht);
		Rectangle(iloc-3, y+10, 6, 1, wht);
		Rectangle(iloc-3, y-10, 7, 1, dkGryV);
		Rectangle(iloc-2, y-9, 1, 18, ltGryV);
		Rectangle(iloc+2, y-9, 1, 19, mdGryV);
		Rectangle(iloc-2, y+9, 4, 1, ltGryV);
		Rectangle(iloc-2, y-9, 5, 1, mdGryV);
	}
	else { // vertical
		for (int i = 0; i < 4; i++) {
			float g = grays[i]/255.f, col[] = {g, g, g};
			Rectangle(x-1+i, y, 1, size, col);
		}
		Rectangle(x-1, y, 4, 1, wht);
		Rectangle(x, y+1, 1, 1, ltGryV);
		Rectangle(x-1, y+size-1, 3, 1, mdGryV);
		// slider
		Rectangle(x-10, iloc-3, 20, 7, offWht); // whole knob
		Rectangle(x-10, iloc-3, 20, 1, dkGryV); // bottom
		Rectangle(x+10, iloc-3, 1, 7, dkGryV);  // right
		Rectangle(x-10, iloc-2, 1, 6, wht);     // left
		Rectangle(x-10, iloc+3, 20, 1, wht);    // top
		Rectangle(x-9, iloc-1, 1, 4, ltGryV);   // 1 pixel right of left
		Rectangle(x-9, iloc+2, 18, 1, ltGryV);  // 1 pixel below top
		Rectangle(x-9, iloc-2, 18, 1, mdGryV);  // 1 pixel above bottom
		Rectangle(x+9, iloc-2, 1, 5, mdGryV);   // 1 pixel left of right
	}
	if (nameOverride || !name.empty()) {
		const char *s = nameOverride? nameOverride : name.c_str();
		if (winW == -1) {
			winW = glutGet(GLUT_WINDOW_WIDTH);
			winH = glutGet(GLUT_WINDOW_HEIGHT);
		}
		char buf[100], num[100];
		float val = GetValue();
		vec3 col(color[0], color[1], color[2]);
		char *start = val < 1? buf+1 : buf; // skip leading zero
		if (orientation == Hor) {
			sprintf(num, val >= 1? "%3.2f" : val < .001? "%4.4f" : "%3.3f", val);
			sprintf(buf, "%s: %s", s, val < 1? num+1 : num); // +1: skip zero
			PutString(x+size+8, y-2, buf, col, GLUT_BITMAP_HELVETICA_12);
		}
		else {
			sprintf(buf, val >= 1? "%3.2f" : "%3.3f", val);
			int wName = 1+glutBitmapLength(GLUT_BITMAP_HELVETICA_12, (unsigned char *) s);
			int wBuf = glutBitmapLength(GLUT_BITMAP_HELVETICA_12, (unsigned char *) start);
			PutString(x-wName/2, y+size+6, s, col, GLUT_BITMAP_HELVETICA_12);
			PutString(x+1-wBuf/2, y-15, start, col, GLUT_BITMAP_HELVETICA_12);
		}
	}
}

float Slider::GetValue() {
	int ref = orientation == Hor? x : y;
	return min+((float)(loc-ref)/size)*(max-min);
}

bool Slider::Hit(int xx, int yy) {
    return orientation == Hor?
		xx >= x && xx <= x+size && yy >= y-10 && yy <= y+10 :
		xx >= x-16 && xx <= x+16 && yy >= y-32 && yy <= y+size+27;
}

bool Slider::Mouse(int ax, int ay) {
	// snap to mouse location
	int old = loc;
	int mouse = orientation == Hor? ax : ay;
	int min = orientation == Hor? x : y;
	loc = mouse < min? min : mouse > min+size? min+size : mouse;
	return old != loc;
}

// Mover

float DotProduct(float a[], float b[]) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }

void SetPlane(vec3 &p, int x, int y, mat4 &modelview, mat4 &persp, float *plane) {
	vec3 p1, p2;									// two points that transform to pixel x,y
	ScreenLine((float) x, (float) y, modelview, persp, (float *) &p1, (float *) &p2);
	for (int i = 0; i < 3; i++)
		plane[i] = p2[i]-p1[i];						// set plane normal to that of line p1p2
	plane[3] = -plane[0]*p.x-plane[1]*p.y-plane[2]*p.z;
		// pass plane through point
}

void Mover::SetPlane(int x, int y, mat4 &modelview, mat4 &persp) { ::SetPlane(*point, x, y, modelview, persp, plane); }

void Mover::Set(vec3 *p) { point = p; }

void Mover::Down(int x, int y, mat4 &modelview, mat4 &persp) { SetPlane(x, y, modelview, persp); }

void Mover::Drag(int x, int y, mat4 &modelview, mat4 &persp) {
    float p1[3], p2[3], axis[3];
    ScreenLine(static_cast<float>(x), static_cast<float>(y), modelview, persp, p1, p2);
        // get two points that transform to pixel x,y
    for (int i = 0; i < 3; i++)
        axis[i] = p2[i]-p1[i];
        // direction of line through p1
    float pdDot = DotProduct(axis, plane);
        // project onto plane normal
    float a = (-plane[3]-DotProduct(p1, plane))/pdDot;
    for (int j = 0; j < 3; j++)
        (*point)[j] = p1[j]+a*axis[j];
        // intersection of line with plane
}

bool Mover::Hit(int x, int y, mat4 &view) {
	return point? ScreenDistSq(x, y, *point, view) < 100 : false;
}

// Aimer

void Aimer::Set(vec3 b, vec3 v, mat4 mv, mat4 p, float s) {
	base = b;
	vector = v;
	modelview = mv;
	persp = p;
	size = s;
	tip = b+s*v;
	mode = A_None;
}

bool Aimer::Hit(int x, int y) { return ScreenDistSq(x, y, base, modelview) < 100 || ScreenDistSq(x, y, tip, modelview) < 100; }

void Aimer::Down(int x, int y) {
	fwdFace = FrontFacing(base, vector, modelview);
	mode = ScreenDistSq(x, y, base, modelview) < 100? A_Base : ScreenDistSq(x, y, tip, modelview) < 100? A_Tip : A_None;
	if (mode == A_Base)
		SetPlane(base, x, y, modelview, persp, plane);
}

void Aimer::Drag(int x, int y) {
	vec3 p1, p2;										// p1p2 is world-space line that xforms to line perp to screen at (x, y)
	ScreenLine((float) x, (float) y, modelview, persp, (float *) &p1, (float *) &p2);
	if (mode == A_Base) {
		// set base
		vec3 axis(p2-p1);								// direction of line through p1
		vec3 normal(plane[0], plane[1], plane[2]);
		float pdDot = dot(axis, normal);
		float a = (-plane[3]-dot(p1, normal))/pdDot;	// project onto plane normal
		base = p1+a*axis;								// intersection of line with plane
		tip = base+size*vector;
	}
	if (mode == A_Tip) {
		// set vector
		vec3 n = ProjectToLine(base, p1, p2);
		vec3 axis(p2-p1);								// should point towards the eye
		normalize(axis);
		vec3 dif(n-base);								// need diagram
		float d = length(dif);
		if (d < size) {									// mouse is within range of tip
			float lineDelta = sqrt(size*size-d*d);		// intersect with sphere
			tip = n-lineDelta*axis;
			if (FrontFacing(base, tip-base, modelview) != fwdFace)
				tip = n+lineDelta*axis;					// ought'n there a better way?
		}
		else
			tip = base+dif*size/d;						// mouse is beyond tip range
		vector = normalize(tip-base);
	}
}

void Aimer::Draw(vec3 &color, float baseQuadScale) {
	bool frontFacing = FrontFacing(base, vector, modelview);
	UseDrawShader(modelview);
	frontFacing? void() : DashOn();
	ArrowV(base, size*vector, modelview, color, NULL, 10);
	if (baseQuadScale > FLT_EPSILON) {
		vec3 n = Ortho(vector);								// |n| = |vector| = 1
		vec3 b = cross(vector, n);							// |b| = |vector||vector| = 1
		n *= baseQuadScale;
		b *= baseQuadScale;
		vec3 p[4] = {base-n-b, base-n+b, base+n+b, base+n-b};
		for (int i = 0; i < 4; i++)
			Line(p[i], p[(i+1)%4], color, 1, 2);
	}
	DashOff();
	Disk(base, 7, vec3(1,0,0));
	Disk(tip, 7, vec3(1,0,0));
}

// Typescripts

Typescript::Typescript() { Clear(); }

Typescript::Typescript(int x, int y, int w, int h, char *text) : x(x), y(y), w(w), h(h) {
    Clear();
	if (text)
		SetText(text);
}

void Typescript::Clear() {
	strcpy(save, text);
	text[0] = 0;
}

void Typescript::Restore() { strcpy(text, save); }

void Typescript::AddChar(char c) {
	int len = strlen(text);
	if (c == 8) { // backspace
		if (len > 0)
			text[len-1] = 0;
	}
	else
		if (len < maxTypescript-1) {
			text[len] = c;
			text[len+1] = 0;
		}
	// printf("typescript = %s\n", text);
}

void Typescript::SetText(const char *c) { strcpy(text, c); }

bool Typescript::Hit(int _x, int _y) {
    return _x >= x && _x < x+w && _y >= y && _y < y+h;
}

void Typescript::Draw(float *color, float *bgrnd) {
	DrawDepressedBox(x, y, w, h, bgrnd);
    Text(x+5, y+4, color? vec3(color[0], color[1], color[2]) : vec3(0), text);
}

// Miscellany

string GetDirectory() {
	char dir[201];
	DWORD len = GetCurrentDirectoryA(199, dir);
	return string(dir);
}

bool Newer(SYSTEMTIME &t1, SYSTEMTIME &t2) {
	// return true if t1 is newer than t2
	return t2.wYear         < t1.wYear?         true : t2.wYear         > t1.wYear?         false :
		   t2.wMonth        < t1.wMonth?        true : t2.wMonth        > t1.wMonth?        false :
		   t2.wDay          < t1.wDay?          true : t2.wDay          > t1.wDay?          false :
		   t2.wHour         < t1.wHour?         true : t2.wHour         > t1.wHour?         false :
		   t2.wMinute       < t1.wMinute?       true : t2.wMinute       > t1.wMinute?       false :
		   t2.wSecond       < t1.wSecond?       true : t2.wSecond       > t1.wSecond?       false :
		   t2.wMilliseconds < t1.wMilliseconds? true : t2.wMilliseconds > t1.wMilliseconds? false :
		   true;
}

void NewestFilename(char *search, string &name) {
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA(search, &fd);
	if (h != INVALID_HANDLE_VALUE) {
		SYSTEMTIME earliest, time;
		bool once = false;
		do {
			FileTimeToSystemTime(&fd.ftLastWriteTime, &time);
			if (!once || Newer(time, earliest)) {
				earliest = time;
				name = string(fd.cFileName);
			}
			once = true;
		} while (FindNextFileA(h, &fd));
		FindClose(h);
	}
}

void GetFilenames(char *search, vector<string> &names) {
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA(search, &fd);
	names.resize(0);
	if (h != INVALID_HANDLE_VALUE) {
		do
			names.push_back(string(fd.cFileName));
		while (FindNextFileA(h, &fd));
		FindClose(h);
	}
}

// Unicode

//string GetDirectory() {
//	char dir[201];
//	TCHAR path[201] = L"";
//	DWORD len = GetCurrentDirectory(199, path);
//	for (int i = 0; i < (int) len; i++)
//		dir[i] = (char) path[i];
//	dir[len] = '\\';
//	dir[len+1] = 0;
//	return string(dir);
//}

//void NewestFilename(char *search, string &name) {
//	wchar_t wbuf[1000];
//	mbstowcs(wbuf, search, 999);
//	WIN32_FIND_DATA fd;
//	HANDLE h = FindFirstFile(wbuf, &fd);
//	if (h != INVALID_HANDLE_VALUE) {
//		SYSTEMTIME earliest, time;
//		bool once = false;
//		do {
//			char buf[1000];
//			wcstombs(buf, fd.cFileName, 999);
//			FileTimeToSystemTime(&fd.ftLastWriteTime, &time);
//			if (!once || Newer(time, earliest)) {
//				earliest = time;
//				name = string(buf);
//			}
//			once = true;
//		} while (FindNextFile(h, &fd));
//		FindClose(h);
//	}
//}

//void GetFilenames(char *search, vector<string> &names) {
//	wchar_t wbuf[1000];
//	mbstowcs(wbuf, search, 999);
//	WIN32_FIND_DATA fd;
//	HANDLE h = FindFirstFile(wbuf, &fd);
//	names.resize(0);
//	if (h != INVALID_HANDLE_VALUE) {
//		do {
//			char buf[1000];
//			wcstombs(buf, fd.cFileName, 999);
//			names.push_back(string(buf));
//		} while (FindNextFile(h, &fd));
//		FindClose(h);
//	}
//}

#include <CommDlg.h>

//bool ChooseFile(bool read, string startDir, wchar_t *wbuf, string &filename, string title) {
//	const int MAX = 200;
//	wchar_t wname[MAX], wdir[MAX], wtitle[MAX];
//    OPENFILENAME ofn;
//	memset(&ofn, 0, sizeof(ofn));
//	ofn.lStructSize = sizeof(ofn);
//	ofn.hwndOwner = NULL; // should open dialog box if program fullscreen, without changing mode; alas, no
//	ofn.lpstrFile = wname;
//	ofn.lpstrFile[0] = '\0';
//	if (!read)
//		mbstowcs(wname, filename.c_str(), MAX);
//	ofn.nMaxFile = MAX;
//	ofn.lpstrFilter = wbuf;
//	ofn.nFilterIndex = 1; // 2;
//	mbstowcs(wdir, startDir.c_str(), MAX);
//	ofn.lpstrInitialDir = wdir;
//	mbstowcs(wtitle, title.c_str(), MAX);
//	ofn.lpstrFileTitle = wtitle;
//	ofn.nMaxFileTitle = 200;
//	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
//    BOOL ok = read? GetOpenFileNameW(&ofn) : GetSaveFileNameW(&ofn);
//    if (ok) {
//		char name[200];
//		wcstombs(name, ofn.lpstrFile, MAX);
//		filename = string(name);
//	}
//    return ok == TRUE;
//}

//bool ChooseFile(bool read, string startDir, string &filename, string title) {
//	const int MAX = 200;
//	wchar_t wname[MAX], wdir[MAX], wtitle[MAX];
//    OPENFILENAME ofn;
//	memset(&ofn, 0, sizeof(ofn));
//	ofn.lStructSize = sizeof(ofn);
//	ofn.hwndOwner = NULL; // FindWindow(NULL, (LPCWSTR) L"App Title");
//		                  // should open dialog box if program fullscreen, without changing mode; alas, no
//	ofn.lpstrFile = wname;
//	ofn.lpstrFile[0] = '\0';
//	if (!read)
//		mbstowcs(wname, filename.c_str(), MAX);
//	ofn.nMaxFile = MAX;
//	ofn.lpstrFilter = read? L"All\0*.bob;*.stl;*.obj\0\0" : L"All\0*.stl\0\0";
//	ofn.nFilterIndex = 1; // 2;
//	mbstowcs(wdir, startDir.c_str(), MAX);
//	ofn.lpstrInitialDir = wdir;
//	mbstowcs(wtitle, title.c_str(), MAX);
//	ofn.lpstrFileTitle = wtitle;
//	ofn.nMaxFileTitle = 200;
//	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
//    BOOL ok = read? GetOpenFileNameW(&ofn) : GetSaveFileNameW(&ofn);
//    if (ok) {
//		char name[200];
//		wcstombs(name, ofn.lpstrFile, MAX);
//		filename = string(name);
//	}
//    return ok == TRUE;
//}

bool ChooseFile(bool read, string startDir, char *wbuf, string &filename, string title) {
	const int MAX = 200;
	char wname[MAX], wdir[MAX], wtitle[MAX];
	strcpy(wname, filename.c_str());
	strcpy(wdir, startDir.c_str());
	strcpy(wtitle, title.c_str());
    OPENFILENAMEA ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL; // should open dialog box if program fullscreen, without changing mode; alas, no
	ofn.lpstrFile = wname; // optional suggested filename
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = MAX;
	ofn.lpstrFilter = wbuf;
	ofn.nFilterIndex = 1; // 2;
	ofn.lpstrInitialDir = wdir;
	ofn.lpstrFileTitle = wtitle;
	ofn.nMaxFileTitle = 200;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    BOOL ok = read? GetOpenFileNameA(&ofn) : GetSaveFileNameA(&ofn);
    if (ok)
		filename = string(ofn.lpstrFile);
    return ok == TRUE;
}

bool ChooseFile(bool read, string startDir, string &filename, string title) {
	const int MAX = 200;
	char wname[MAX], wdir[MAX], wtitle[MAX];
	strcpy(wname, filename.c_str());
	strcpy(wdir, startDir.c_str());
	strcpy(wtitle, title.c_str());
    OPENFILENAMEA ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL; // FindWindow(NULL, (LPCWSTR) L"App Title");
		                  // should open dialog box if program fullscreen, without changing mode; alas, no
	ofn.lpstrFile = wname;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = MAX;
	ofn.lpstrFilter = read? "All\0*.bob;*.stl;*.obj\0\0" : "All\0*.stl\0\0";
	ofn.nFilterIndex = 1; // 2;
	ofn.lpstrInitialDir = wdir;
	ofn.lpstrFileTitle = wtitle;
	ofn.nMaxFileTitle = 200;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    BOOL ok = read? GetOpenFileNameA(&ofn) : GetSaveFileNameA(&ofn);
    if (ok)
		filename = string(ofn.lpstrFile);
    return ok == TRUE;
}
