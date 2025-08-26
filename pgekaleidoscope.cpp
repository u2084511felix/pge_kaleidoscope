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
	olc::vf2d size;
};

struct Triangle {
	ShapeBase shapeb;
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


struct ShapeSet {
	std::vector<Circle> vCircle;
	std::vector<Square> vSquare;
	std::vector<Triangle> vTriangle;
};



class SeedGen
{
public:
	SeedGen(uint32_t x, uint32_t y)
	{
		nProcGen = (x & 0xFFFF) << 16 | (y & 0xFFFF);
		
		clusterSize = rndInt(1, 20);

		double distanceFromCluster = rndDouble(60.0, 200.0);
		for (int i = 0; i < clusterSize; i++) {
			distanceFromCluster += rndDouble(3.0, 100.0);
			Triangle t;
			Square s;
			Circle c;

			t.pos = {float(rndDouble(9.0, 10.0)), float(rndDouble(4.0, 5.0))};
			t.pos2 = {float(rndDouble(9.0, 10.0)), float(rndDouble(4.0, 5.0))};
			t.pos3 = {float(rndDouble(9.0, 10.0)), float(rndDouble(4.0, 5.0))};
			t.shapeb.distance = distanceFromCluster;
			t.shapeb.colour = shapeColours[rndInt(0, 8)];

			distanceFromCluster += rndDouble(1.0, 4.0);
			s.pos = {float(rndDouble(3.0, 10.0)), float(rndDouble(2.0, 8.0))};
			s.size = {float(rndDouble(3.0, 10.0)), float(rndDouble(2.0, 8.0))};
			s.shapeb.distance = distanceFromCluster;
			s.shapeb.colour = shapeColours[rndInt(0, 8)];

			distanceFromCluster += rndDouble(1.0, 4.0);
			c.pos = {float(rndDouble(3.0, 10.0)), float(rndDouble(2.0, 8.0))};
			c.diameter = rndDouble(0.2, 0.5);
			c.mask = nProcGen * rndDouble(0.1, 0.7);
			c.radius = float(rndDouble(1.0, 3.0));
			c.shapeb.distance = distanceFromCluster;
			c.shapeb.colour = shapeColours[rndInt(0, 8)];

			cluster.vCircle.push_back(c);
			cluster.vTriangle.push_back(t);
			cluster.vSquare.push_back(s);
		} 
	}

	~SeedGen()
	{

	}

public:
	ShapeSet cluster;
	int clusterSize;

private:
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
};



class ScreenBackground : public olc::PixelGameEngine
{
public:
	ScreenBackground()
	{
		sAppName = "ScreenBackground";
	}

public:
	bool OnUserCreate() override
	{
		return true;
	}

	olc::vf2d vScreenBackgroundOffset = { 0,0 };
	bool expand = 1;
	float pulse_freq = 0.0f;


	bool OnUserUpdate(float fElapsedTime) override
	{
		if (fElapsedTime <= 0.0001f) return true;
		Clear(olc::BLACK);

		//NOTE: MODULARS:
		if (expand) {
			pulse_freq += 0.00002f;
		} else {
			pulse_freq -= 0.00002f;
		}

		if (pulse_freq >= 0.2f) {
			expand = false;
		}
		if (pulse_freq <= 0.18f) {
			expand = true;
		}

		//NOTE: Human Input:

		if (GetKey(olc::Key::W).bHeld) vScreenBackgroundOffset.y -= 50.0f * fElapsedTime;
		if (GetKey(olc::Key::S).bHeld) vScreenBackgroundOffset.y += 50.0f * fElapsedTime;
		if (GetKey(olc::Key::A).bHeld) vScreenBackgroundOffset.x -= 50.0f * fElapsedTime;
		if (GetKey(olc::Key::D).bHeld) vScreenBackgroundOffset.x += 50.0f * fElapsedTime;

		olc::vi2d mouse = { GetMouseX() / 16, GetMouseY() / 16 };
		olc::vi2d galaxy_mouse = mouse + vScreenBackgroundOffset;



		//NOTE: Draw Screen Space
		olc::vi2d screen_sector = { 0,0 };
		int nSectorsX = ScreenWidth() / 16;
		int nSectorsY = ScreenHeight() / 16;
		for (screen_sector.x = 0; screen_sector.x < nSectorsX; screen_sector.x++)
			for (screen_sector.y = 0; screen_sector.y < nSectorsY; screen_sector.y++)
			{
				uint32_t seed1 = (uint32_t)vScreenBackgroundOffset.x + (uint32_t)screen_sector.x;
				uint32_t seed2 = (uint32_t)vScreenBackgroundOffset.y + (uint32_t)screen_sector.y;
				
				olc::vi2d sectorPos = {(screen_sector.x *16 + 8), (screen_sector.y *16 +8)};

				SeedGen star(seed1, seed2);
				for (int i = 0; i < star.clusterSize; i++) {
					olc::vf2d sqpos = star.cluster.vSquare[i].pos;
					olc::vf2d cpos = star.cluster.vCircle[i].pos;
					olc::vf2d tpos = star.cluster.vTriangle[i].pos;
					olc::vf2d tpos2 = star.cluster.vTriangle[i].pos2;
					olc::vf2d tpos3 = star.cluster.vTriangle[i].pos3;

					float crad = star.cluster.vCircle[i].radius;
					float cdiam = star.cluster.vCircle[i].diameter;
					uint32_t ccol = star.cluster.vCircle[i].shapeb.colour;

					olc::vf2d sqsize = star.cluster.vSquare[i].size;
					uint32_t sqcol = star.cluster.vSquare[i].shapeb.colour;

					uint32_t tcol = star.cluster.vTriangle[i].shapeb.colour;

					FillRect(sectorPos, sqsize, sqcol);
					FillCircle(sectorPos, cdiam, ccol);
					FillTriangleDecal(sectorPos, tpos2, tpos3, tcol);
				}				
				

				// FillCircle(screen_sector.x * 16 + 8, screen_sector.y * 16 + 8, (int)star.shapeDiameter * pulse_freq, star.shapeColour);
				// FillRect({screen_sector.x * 16 + 8, screen_sector.y * 16 + 8}, {10, 10}, star.shapeColour);
				// FillTriangleDecal({float(screen_sector.x *16 +8), float(screen_sector.y *16 +8)}, {float(screen_sector.x *16 +9), float(screen_sector.y *16 +9)}, {float(screen_sector.x *16 +4), float(screen_sector.y *16 +4)}, star.shapeColour);
			}
		return true;
	}
};

int main()
{
	ScreenBackground demo;
	if (demo.Construct(512, 480, 2, 2, false, false))
		demo.Start();
	return 0;
}
