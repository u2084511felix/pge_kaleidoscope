#define OLC_PGE_APPLICATION

#include <pge/olcPixelGameEngine.h>
//#include <pge/extensions/olcPGEX_TransformedView.h>
#include <random>
//olc::Pixel	shapeColour = olc::WHITE;

constexpr uint32_t shapeColours[8] = 
{
	0xFFFFFFFF, 0xFFD9FFFF, 0xFFA3FFFF, 0xFFFFC8C8,
	0xFFFFCB9D, 0xFF9F9FFF, 0xFF415EFF, 0xFF28199D
};

enum ShapeEnum {
	CIRCLE,
	TRIANGLE,
	SQUARE,
	STAR,
	MOON,
	HEXAGON
};

constexpr ShapeEnum shapeArray[6] = {
	CIRCLE,
	TRIANGLE,
	SQUARE,
	STAR,
	MOON,
	HEXAGON
};

enum Layer {
	BG,
	MID,
	FG
};

constexpr Layer layerEnum[3] = {
	BG,
	MID,
	FG
};


struct ShapeBase {
	uint32_t colour = shapeColours[0];
	Layer layer = layerEnum[0];
	int fill = 1;
	float size_mod = 1.0f;
	float mass = 1.0f;
	int size;
	float rdegrees = 0.1f;
};

struct Triangle {
	ShapeBase shapeb;
	float sidelength;
	olc::vf2d center;
	olc::vf2d pos;
	olc::vf2d pos2;
	olc::vf2d pos3;
};

struct Square {
	ShapeBase shapeb;
	olc::vf2d pos;
	olc::vf2d size;
};


struct Circle {
	ShapeBase shapeb;
	float radius = 0.0f;
	double diameter = 0.0;
	uint8_t mask = 0xFF;
	olc::vf2d pos;
};

struct Star {
	ShapeBase shapeb;
	olc::Renderable star;
};



class Kaleidoscope : public olc::PixelGameEngine
{
public:
	Kaleidoscope()
	{
		sAppName = "Kaleidoscope";
	}
	std::vector<Triangle> triangles;
	std::vector<Triangle>* vptriangles;
	std::vector<Triangle*> ptriangles;
	std::vector<Circle> circles;
	std::vector<Square> squares;
	olc::vf2d vKaleidoscopeOffset = { 0,0 };
	bool expand = 1;
	float pulse_freq = 0.0f;

	int clusterSize;
	const double PIEE = 3.14159265358979323846;
	uint32_t nProcGen = 0;

public:


	//NOTE: ...........................
	//.................................
	//    ProcGen / Randomizer utils: 
	//.................................
	//.................................

	double rndDouble(double min, double max)
	{
		return ((double)rnd() / (double)(0x7FFFFFFF)) * (max - min) + min;
	}

	int rndInt(int min, int max)
	{
		return (rnd() % (max - min)) + min;
	}
	
	// Modified from this for 64-bit systems:
	// https://lemire.me/blog/2019/03/19/the-fastest-conventional-random-number-generator-that-can-pass-big-crush/
	// Now I found the link again - Also, check out his blog, it's a fantastic resource!
	uint32_t rnd()
	{
		nProcGen += 0xe120fc15;
		uint64_t tmp;
		tmp = (uint64_t)nProcGen * 0x4a39b70d;
		uint32_t m1 = (tmp >> 32) ^ tmp;
		tmp = (uint64_t)m1 * 0x12fad5c9;
		uint32_t m2 = (tmp >> 32) ^ tmp;
		return m2;
	}


	//NOTE: ...........................
	//.................................
	//    Transformation Functions 
	//.................................
	//.................................


	void TriangleCoords(Triangle &triangle){
		//Make Triangle Equilateral
		int height = triangle.sidelength * sqrt(3) / 2;
		triangle.pos2 = {triangle.pos.x+triangle.sidelength, triangle.pos.y};
		triangle.pos3 = {triangle.pos.x+(triangle.sidelength/2), triangle.pos.y-height};
	};

	void TriangleCenter(Triangle &triangle){
		//((x1 + x2 + x3) / 3, (y1 + y2 + y3) / 3)
		triangle.center = {((triangle.pos.x + triangle.pos2.x + triangle.pos3.x)/3), ((triangle.pos.y + triangle.pos2.y + triangle.pos3.y)/3)};
	};

	void MoveTriangle(Triangle &triangle, olc::vf2d newcentre){
		triangle.pos = (triangle.pos - triangle.center) + newcentre;
		triangle.pos2 = (triangle.pos2 - triangle.center) + newcentre;
		triangle.pos3 = (triangle.pos3 - triangle.center) + newcentre;
		triangle.center = newcentre;
	};

	void RotateTriangle(Triangle &triangle){
		olc::vd2d a;
		olc::vd2d b;
		olc::vd2d c; 

		double angleRad = triangle.shapeb.rdegrees * (PIEE / 180);
		float cosAngle = cos(angleRad);
		float sinAngle = sin(angleRad);

		triangle.pos -= triangle.center;
		triangle.pos2 -= triangle.center;
		triangle.pos3 -= triangle.center;

		a.x = (triangle.pos.x * cosAngle) - (triangle.pos.y * sinAngle);
		a.y = (triangle.pos.x * sinAngle) + (triangle.pos.y * cosAngle);
		b.x = (triangle.pos2.x * cosAngle) - (triangle.pos2.y * sinAngle);
		b.y = (triangle.pos2.x * sinAngle) + (triangle.pos2.y * cosAngle);
		c.x = (triangle.pos3.x * cosAngle) - (triangle.pos3.y * sinAngle);
		c.y = (triangle.pos3.x * sinAngle) + (triangle.pos3.y * cosAngle);

		a += triangle.center;
		b += triangle.center;
		c += triangle.center;

		triangle.pos = a;
		triangle.pos2 = b;
		triangle.pos3 = c;
	};






	void SpawnShapes() {
		for (int x = 0; x < ScreenWidth(); x += 16) {
			for (int y = 0; y < ScreenHeight(); y += 16) {

				int spawn = 1;

				Triangle t;


				t.shapeb.layer = layerEnum[rndInt(0,3)];
				switch(t.shapeb.layer) {
					case BG: 
						t.sidelength = rndInt(10,20);
						break;
					case MID: 
						t.sidelength = rndInt(20,30);
						break;
					case FG: 
						t.sidelength = rndInt(50,60);
						spawn = rndInt(0, 3);
						break;
				}

				t.sidelength = rndInt(10,30);
				t.shapeb.colour = shapeColours[rndInt(0,8)];
				t.pos = {float(x), float(y)};
				//Make Equilateral: 
				TriangleCoords(t);
				TriangleCenter(t);
				t.shapeb.fill = rndInt(0,3);
				t.shapeb.rdegrees = rndDouble(0.1f, 3.0f);

				if (spawn == 1) {
					triangles.push_back(t);
				}
				spawn=1;

				//Squares
				Square sq;

				sq.pos = {float(x), float(y)};
				sq.shapeb.layer = layerEnum[rndInt(0,3)];

				float sqsize;
				switch(sq.shapeb.layer) {
					case BG: 
						sqsize = rndInt(1,4);
						break;
					case MID: 
						sqsize = rndInt(5,10);
						break;
					case FG: 
						sqsize = rndInt(20,40);
						spawn = rndInt(0, 3);
						break;
				}

				sq.size = {sqsize, sqsize};;
				sq.shapeb.fill = rndInt(0,3);
				sq.shapeb.colour = shapeColours[rndInt(0,8)];

				if (spawn == 1) {
					squares.push_back(sq);
				}
				spawn=1;


				//Circle
				Circle c;
				c.pos = {float(x), float(y)};
				c.shapeb.colour = shapeColours[rndInt(0,8)];

				c.shapeb.layer = layerEnum[rndInt(0,3)];

				switch(c.shapeb.layer) {
					case BG: 
						c.diameter = rndInt(1, 5);
						break;
					case MID: 
						c.diameter = rndInt(5, 10);
						break;
					case FG: 
						c.diameter = rndInt(20, 40);
						spawn = rndInt(0, 3);
						break;
				}
	
				c.radius = c.diameter / 2;
				c.shapeb.fill = rndInt(0,3);

				if (spawn == 1) {
					circles.push_back(c);
				}
				spawn=1;



			}
		}

	};


	void ReSpawnShape(Triangle &triangle) {
		olc::vf2d newc = {0,0};
		float min = std::min({triangle.pos.y, triangle.pos2.y, triangle.pos3.y});
		min = triangle.center.y - min;

		if (( triangle.center.y - min) >= ScreenHeight())  {
			newc = {triangle.center.x, ((0 - min))};
			MoveTriangle(triangle, newc);
		}
	}

	void ReSpawnCircle(Circle &circle) {
		olc::vf2d newc = {0,0};
		if (( circle.pos.y - circle.radius) >= ScreenHeight())  {
			newc = {circle.pos.x, ((0 - circle.radius))};
			circle.pos = newc;
		}
	}


	void ReSpawnRect(Square &square) {
		olc::vf2d newc = {0,0};
		if (( square.pos.y - square.size.y) >= ScreenHeight())  {
			newc = {square.pos.x, ((0 - square.size.y))};
			square.pos = newc;
		}
	}



	bool OnUserCreate() override
	{
		SetPixelBlend(1.0);
		SpawnShapes();
		return true;
	}




	bool OnUserUpdate(float fElapsedTime) override
	{
		//if (fElapsedTime <= 0.0001f) return true;
		Clear(olc::BLACK);


// 		//NOTE: MODULARS:
// 		if (expand) {
// 			pulse_freq += 0.00002f;
// 		} else {
// 			pulse_freq -= 0.00002f;
// 		}

// 		if (pulse_freq >= 0.2f) {
// 			expand = false;
// 		}
// 		if (pulse_freq <= 0.1f) {
// 			expand = true;
// 		}
// 		//NOTE: Human Input:

// 		if (GetKey(olc::Key::W).bHeld) vKaleidoscopeOffset.y -= 50.0f * fElapsedTime;
// 		if (GetKey(olc::Key::S).bHeld) vKaleidoscopeOffset.y += 50.0f * fElapsedTime;
// 		if (GetKey(olc::Key::A).bHeld) vKaleidoscopeOffset.x -= 50.0f * fElapsedTime;
// 		if (GetKey(olc::Key::D).bHeld) vKaleidoscopeOffset.x += 50.0f * fElapsedTime;


// 		olc::vi2d mouse = { GetMouseX() / 16, GetMouseY() / 16 };
// 		olc::vi2d galaxy_mouse = mouse + vKaleidoscopeOffset;

	
		for (Triangle &tri: triangles) {	
			olc::vf2d newc = {0, 1.0};
			MoveTriangle(tri, tri.center + newc);
			RotateTriangle(tri);

			if (tri.shapeb.fill == 1) {
				FillTriangle(tri.pos, tri.pos2, tri.pos3, tri.shapeb.colour);
			} else {
				DrawTriangle(tri.pos, tri.pos2, tri.pos3, tri.shapeb.colour);
			}

			ReSpawnShape(tri);
		}

		for (Circle &c: circles) {
			olc::vf2d newc = {0, 0.5};
			c.pos += newc;

			if (c.shapeb.fill == 1) {
				FillCircle(c.pos, c.radius, c.shapeb.colour);
			} else {
				DrawCircle(c.pos, c.radius, c.shapeb.colour, c.mask);
			}
			ReSpawnCircle(c);
		}

		for (Square &sq: squares) {
			olc::vf2d newc = {0, 1.3};
			sq.pos += newc;
			if (sq.shapeb.fill == 1) {
				FillRect(sq.pos, sq.size, sq.shapeb.colour);
			} else {
				DrawRect(sq.pos, sq.size, sq.shapeb.colour);
			}
			ReSpawnRect(sq);
		}
 		return true;
	}
};
int main()
{
	Kaleidoscope demo;
	if (demo.Construct(1024, 960, 1, 1, false, true, false, true))
		demo.Start();
	return 0;
}
