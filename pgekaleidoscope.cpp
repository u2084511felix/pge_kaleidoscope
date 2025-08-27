#define OLC_PGE_APPLICATION
#include <pge/olcPixelGameEngine.h>
//#include <pge/extensions/olcPGEX_TransformedView.h>


#include <random>

//olc::Pixel	shapeColour = olc::WHITE;

constexpr uint32_t g_shapeColours[8] = 
{
	0xFFFFFFFF, 0xFFD9FFFF, 0xFFA3FFFF, 0xFFFFC8C8,
	0xFFFFCB9D, 0xFF9F9FFF, 0xFF415EFF, 0xFF28199D
};

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


struct ShapeBase {
	uint32_t colour = shapeColours[0];
	double distance = 0.0;
	float size_mod = 1.0f;
	float mass = 1.0f;
	int size;
	float rdegrees = 0.1f;
};

struct Triangle {
	ShapeBase shapeb;
	olc::vf2d center;
	olc::vf2d pos;
	olc::vf2d pos2;
	olc::vf2d pos3;
	std::vector<olc::vf2d> tcoords;
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


struct ShapeSet {
	std::vector<Triangle> triangles;
	std::vector<Circle> circles;
	std::vector<Square> squares;
};



class SeedGen
{
public:
	SeedGen()
	{

	}

	~SeedGen()
	{

	}

public:
	int clusterSize;
	const double PIEE = 3.14159265358979323846;
	uint32_t nProcGen = 0;

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



	std::vector<olc::vf2d> TriangleCoords(int length, olc::vf2d startp){
		int height = length * sqrt(3) / 2;
		std::vector<olc::vf2d> triangle_coords;
		olc::vf2d a = startp;
		olc::vf2d b = {startp.x+length, startp.y};
		olc::vf2d c = {startp.x+(length/2), startp.y-height};
		triangle_coords.push_back(a);
		triangle_coords.push_back(b);
		triangle_coords.push_back(c);
		return triangle_coords;
	};

	olc::vi2d TriangleCenter(std::vector<olc::vf2d> trcoords){
		//((x1 + x2 + x3) / 3, (y1 + y2 + y3) / 3)
		olc::vf2d a = trcoords[0];
		olc::vf2d b = trcoords[1];
		olc::vf2d c = trcoords[2];
		olc::vf2d centre = {((a.x + b.x + c.x)/3), ((a.y + b.y + c.y)/3)};
		return centre;
	};

	std::vector<olc::vf2d> MoveTriangle(std::vector<olc::vf2d> trcoords, olc::vf2d newcentre){
		//((x1 + x2 + x3) / 3, (y1 + y2 + y3) / 3)
		olc::vf2d a = trcoords[0];
		olc::vf2d b = trcoords[1];
		olc::vf2d c = trcoords[2];
		olc::vf2d oldcentre = {((a.x + b.x + c.x)/3), ((a.y + b.y + c.y)/3)};

		float adiffx = a.x - oldcentre.x;
		float adiffy = a.y - oldcentre.y;

		float bdiffx = b.x - oldcentre.x;
		float bdiffy = b.y - oldcentre.y;

		float cdiffx = c.x - oldcentre.x;
		float cdiffy = c.y - oldcentre.y;

		olc::vf2d newa = {newcentre.x + adiffx, newcentre.y + adiffy};
		olc::vf2d newb = {newcentre.x + bdiffx, newcentre.y + bdiffy};
		olc::vf2d newc = {newcentre.x + cdiffx, newcentre.y + cdiffy};

		std::vector<olc::vf2d> newtpos = {newa, newb, newc};
		return newtpos;
	};

	std::vector<olc::vf2d> RotateTriangle(std::vector<olc::vf2d> trcoords, double degrees, olc::vf2d tric){
		olc::vd2d a;
		olc::vd2d b;
		olc::vd2d c; 

		double angleRad = degrees * (PIEE / 180);
		float cosAngle = cos(angleRad);
		float sinAngle = sin(angleRad);

		
		trcoords[0].x -= tric.x;
		trcoords[0].y -= tric.y;
		trcoords[1].x -= tric.x;
		trcoords[1].y -= tric.y;
		trcoords[2].x -= tric.x;
		trcoords[2].y -= tric.y;

		a.x = (trcoords[0].x * cosAngle) - (trcoords[0].y * sinAngle);
		a.y = (trcoords[0].x * sinAngle) + (trcoords[0].y * cosAngle);
		b.x = (trcoords[1].x * cosAngle) - (trcoords[1].y * sinAngle);
		b.y = (trcoords[1].x * sinAngle) + (trcoords[1].y * cosAngle);
		c.x = (trcoords[2].x * cosAngle) - (trcoords[2].y * sinAngle);
		c.y = (trcoords[2].x * sinAngle) + (trcoords[2].y * cosAngle);


		a.x += tric.x;
		a.y += tric.y;
		b.x += tric.x;
		b.y += tric.y;
		c.x += tric.x;
		c.y += tric.y;

		std::vector<olc::vf2d> newtpos;

		newtpos.push_back(a);
		newtpos.push_back(b);
		newtpos.push_back(c);
		return newtpos;
	};



};



class ScreenBackground : public olc::PixelGameEngine
{
public:
	ScreenBackground()
	{
		sAppName = "Kaleidoscope";
	}

public:
	SeedGen seed;
	ShapeSet shapes;

	bool OnUserCreate() override
	{
		SetPixelBlend(1.0);

		

		for (int x = 0; x < ScreenWidth(); x += 16) {
			for (int y = 0; y < ScreenHeight(); y += 16) {
				Triangle t;
				// Circle c;
				// Square sq;


				//Triangle
				int sidel = seed.rndInt(10,30);
				t.shapeb.colour = shapeColours[seed.rndInt(0,8)];
				t.tcoords = seed.TriangleCoords(sidel, {float(x), float(y)});
				t.tcoords = seed.MoveTriangle(t.tcoords, {float(x), float(y)});	
				t.pos = t.tcoords[0];
				t.pos2 = t.tcoords[1];
				t.pos3 = t.tcoords[2];
				t.center = seed.TriangleCenter(t.tcoords);	
				t.shapeb.rdegrees = 0.1f;

		// 		//Squares
		// 		Square sq;
		// 		sq = star.cluster.vSquare[i];
		// 		olc::vf2d sqpos = sq.pos;
		// 		olc::vf2d sqsize = sq.size;
		// 		uint32_t sqcol = sq.shapeb.colour;
		//
		// 		//Circle
		// 		Circle c = star.cluster.vCircle[i];
		// 		olc::vf2d cpos = c.pos;	
		// 		float crad = c.radius;
		// 		float cdiam = c.diameter;
		// 		uint32_t ccol = c.shapeb.colour;
				//



				shapes.triangles.push_back(t);
				// shapes.circles.push_back(c);
				// shapes.squares.push_back(sq);

				//Draw(x, y, olc::Pixel(rand() % 256, rand() % 256, rand() % 256));

			}
		}
		return true;
	}

	olc::vf2d vScreenBackgroundOffset = { 0,0 };
	bool expand = 1;
	float pulse_freq = 0.0f;


	bool OnUserUpdate(float fElapsedTime) override
	{
		if (fElapsedTime <= 0.0001f) return true;
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
//
// 		//NOTE: Human Input:
//
// 		if (GetKey(olc::Key::W).bHeld) vScreenBackgroundOffset.y -= 50.0f * fElapsedTime;
// 		if (GetKey(olc::Key::S).bHeld) vScreenBackgroundOffset.y += 50.0f * fElapsedTime;
// 		if (GetKey(olc::Key::A).bHeld) vScreenBackgroundOffset.x -= 50.0f * fElapsedTime;
// 		if (GetKey(olc::Key::D).bHeld) vScreenBackgroundOffset.x += 50.0f * fElapsedTime;
//
// 		olc::vi2d mouse = { GetMouseX() / 16, GetMouseY() / 16 };
// 		olc::vi2d galaxy_mouse = mouse + vScreenBackgroundOffset;
		//
	


		for (Triangle &tri: shapes.triangles) {

			tri.center.y += 0.1f;
			//tri.tcoords = seed.MoveTriangle(tri.tcoords, tri.center);
			//
			tri.tcoords = seed.RotateTriangle(tri.tcoords, tri.shapeb.rdegrees, tri.center);
			FillTriangle(
				tri.tcoords[0], 
				tri.tcoords[1], 
				tri.tcoords[2], 
				tri.shapeb.colour
			);

		}




 		return true;
	}


};


int main()
{
	ScreenBackground demo;
	if (demo.Construct(1024, 960, 1, 1))
		demo.Start();
	return 0;
}
