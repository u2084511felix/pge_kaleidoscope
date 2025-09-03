/*

	License (OLC-3)
	~~~~~~~~~~~~~~~
	Redistribution and use in source and binary forms, with or without modification,
	are permitted provided that the following conditions are met:

	1. Redistributions or derivations of source code must retain the above copyright
	notice, this list of conditions and the following disclaimer.

	2. Redistributions or derivative works in binary form must reproduce the above
	copyright notice. This list of conditions and the following	disclaimer must be
	reproduced in the documentation and/or other materials provided with the distribution.

	3. Neither the name of the copyright holder nor the names of its contributors may
	be used to endorse or promote products derived from this software without specific
	prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS	"AS IS" AND ANY
	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
	SHALL THE COPYRIGHT	HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL,	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
	TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
	BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
	ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
	SUCH DAMAGE.


*/

#define OLC_PGE_APPLICATION

#include <pge/olcPixelGameEngine.h>
#include <vector>
#include <algorithm> 

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
	olc::Pixel colour;
	Layer layer = layerEnum[0];
	int fill = 1;
	float size_mod = 1.0f;
	float mass = 1.0f;
	int size;
	float rdegrees = 0.1f;
	olc::vf2d speed;
	int counterclockwiserotation;
};

struct Triangle {
	ShapeBase shapeb;
	float sidelength;
	float height;
	float width;
	olc::vf2d center;
	olc::vf2d x;
	olc::vf2d y;
	olc::vf2d z;
	std::vector<olc::vf2d> coords;
	std::vector<olc::vf2d> texture;
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
	std::vector<Triangle> bgtriangles;
	std::vector<Circle> circles;
	std::vector<Square> squares;

	bool expand = 1;
	float pulse_freq = 0.0f;

	olc::Sprite* Mask;
	olc::Sprite* newsp;
	olc::Decal* maskdecal;
	olc::vf2d cetrepos;


	olc::vf2d uvA = {0.0f, 1.0f};
	olc::vf2d uvB = {1.0f, 1.0f};
	olc::vf2d uvC = {0.5f, 0.0f};

	std::vector<std::vector<olc::vf2d>> uv_sets;
	const double PIEE = 3.14159265358979323846;
	uint32_t nProcGen = 0;

	Triangle baseTriangle;
	Triangle* bt = &baseTriangle;


	const int max_radius = 5; 

public:
	olc::Pixel randomColour(){
		int r = rndInt(0,255);
		int g = rndInt(0,255);
		int b = rndInt(0,255);
		return olc::Pixel(r,g,b);
	};

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
		float height = triangle.sidelength * sqrt(3) / 2;
		triangle.height = height;
		triangle.z = {triangle.x.x+(triangle.sidelength/2), triangle.x.y-height};
		triangle.y = {triangle.x.x+triangle.sidelength, triangle.x.y};
	};

	void TriangleCenter(Triangle &triangle){
		//((x1 + x2 + x3) / 3, (y1 + y2 + y3) / 3)
		triangle.center = {((triangle.x.x + triangle.y.x + triangle.z.x)/3), ((triangle.x.y + triangle.y.y + triangle.z.y)/3)};
	};

	void MoveTriangle(Triangle &triangle, olc::vf2d newcentre){
		triangle.x = (triangle.x - triangle.center) + newcentre;
		triangle.y = (triangle.y - triangle.center) + newcentre;
		triangle.z = (triangle.z - triangle.center) + newcentre;
		triangle.center = newcentre;
	};

	void RotateTriangle(Triangle &triangle){
		olc::vd2d a;
		olc::vd2d b;
		olc::vd2d c; 

		double angleRad = triangle.shapeb.rdegrees * (PIEE / 180);

		float cosAngle = cos(angleRad);
		float sinAngle = sin(angleRad);
		if (triangle.shapeb.counterclockwiserotation &1) {
			sinAngle *= -1;
		}

		triangle.x -= triangle.center;
		triangle.y -= triangle.center;
		triangle.z -= triangle.center;

		a.x = (triangle.x.x * cosAngle) - (triangle.x.y * sinAngle);
		a.y = (triangle.x.x * sinAngle) + (triangle.x.y * cosAngle);
		b.x = (triangle.y.x * cosAngle) - (triangle.y.y * sinAngle);
		b.y = (triangle.y.x * sinAngle) + (triangle.y.y * cosAngle);
		c.x = (triangle.z.x * cosAngle) - (triangle.z.y * sinAngle);
		c.y = (triangle.z.x * sinAngle) + (triangle.z.y * cosAngle);

		a += triangle.center;
		b += triangle.center;
		c += triangle.center;

		triangle.x = a;
		triangle.y = b;
		triangle.z = c;
	};

	int hex_distance(olc::vi2d h) {
	    return (std::abs(h.x) + std::abs(h.y) + std::abs(-h.x - h.y)) / 2;
	}

	void SpawnShapes(int random_seed) {
		for (int x = 0; x < ScreenWidth(); x += random_seed) {
			for (int y = 0; y < ScreenHeight(); y += random_seed) {
				int spawn = 1;

				Triangle t;
				t.shapeb.layer = layerEnum[rndInt(0,3)];
				switch(t.shapeb.layer) {
					case BG: 
						t.sidelength = rndInt(5,20);
						t.shapeb.fill = 1;
						spawn = rndInt(0, 6);
						break;
					case MID: 
						t.sidelength = rndInt(20,40);
						t.shapeb.fill = rndInt(0,3);
						spawn = rndInt(0, 18);
						break;
					case FG: 
						t.sidelength = rndInt(40, 60);
						t.shapeb.fill = rndInt(0,3);
						spawn = rndInt(0, 36);
						break;
				}
				t.shapeb.counterclockwiserotation = rndInt(0,12);
				t.shapeb.speed = {0, (float)rndDouble(0.1, 0.2)};
				t.shapeb.colour = randomColour(); 
				t.x = {float(x), float(y)};
				//Make Equilateral: 
				TriangleCoords(t);
				TriangleCenter(t);
				t.shapeb.rdegrees = rndDouble(0.1f, 0.2f);
				if (spawn == 1) {
					bgtriangles.push_back(t);
				}
				spawn=1;

				Square sq;
				sq.pos = {float(x), float(y)};
				sq.shapeb.layer = layerEnum[rndInt(0,3)];
				sq.shapeb.speed = {0, (float)rndDouble(0.1, 0.2)};
				float sqsize;
				switch(sq.shapeb.layer) {
					case BG: 
						sqsize = rndInt(1,4);
						sq.shapeb.fill = 1;
						spawn = rndInt(0, 6);
						break;
					case MID: 
						sqsize = rndInt(5,10);
						sq.shapeb.fill = rndInt(0,3);
						spawn = rndInt(0, 18);
						break;
					case FG: 
						sqsize = rndInt(30,50);
						sq.shapeb.fill = rndInt(0,2);
						spawn = rndInt(0, 36);
						break;
				}
				sq.size = {sqsize, sqsize};;
				sq.shapeb.colour = randomColour();
				if (spawn == 1) {
					squares.push_back(sq);
				}
				spawn=1;

				Circle c;
				c.pos = {float(x), float(y)};
				c.shapeb.colour = randomColour();
				c.shapeb.layer = layerEnum[rndInt(0,3)];
				c.shapeb.speed = {0, (float)rndDouble(0.1, 0.2)};
				switch(c.shapeb.layer) {
					case BG: 
						c.diameter = rndInt(1, 5);
						c.shapeb.fill = 1;
						spawn = rndInt(0, 6);
						break;
					case MID: 
						c.diameter = rndInt(5, 15);
						c.shapeb.fill = rndInt(0,3);
						spawn = rndInt(0, 18);
						break;
					case FG: 
						c.diameter = rndInt(20, 60);
						c.shapeb.fill = rndInt(0,2);
						spawn = rndInt(0, 36);
						break;
				}
				c.radius = c.diameter / 2;
				if (spawn == 1) {
					circles.push_back(c);
				}
				spawn=1;
			}
		}

	};

	void ReSpawnShape(Triangle &triangle) {
		olc::vf2d newc = {0,0};
		float min = std::min({triangle.x.y, triangle.y.y, triangle.z.y});
		min = triangle.center.y - min;
		float newx = rndDouble(0, float(ScreenWidth()));
		if (( triangle.center.y - min) >= ScreenHeight())  {
			triangle.shapeb.colour = randomColour();
			newc = {newx, ((0 - min))};
			MoveTriangle(triangle, newc);
		}
	}

	void ReSpawnCircle(Circle &circle) {
		olc::vf2d newc = {0,0};
		if (( circle.pos.y - circle.radius) >= ScreenHeight())  {
			float newx = rndDouble(0, float(ScreenWidth()));
			circle.shapeb.colour = randomColour();
			newc = {newx, ((0 - circle.radius))};
			circle.pos = newc;
		}
	}

	void ReSpawnRect(Square &square) {
		olc::vf2d newc = {0,0};
		if (( square.pos.y - square.size.y) >= ScreenHeight())  {
			float newx = rndDouble(0, float(ScreenWidth()));
			square.shapeb.colour = randomColour();
			newc = {newx, ((0 - square.size.y))};
			square.pos = newc;
		}
	}

	bool OnUserCreate() override
	{
		SetPixelBlend(1.0);

		uv_sets = {
		    {uvA, uvB, uvC},
		    {uvC, uvA, uvB},
		    {uvB, uvC, uvA},
		    {uvA, uvB, uvC},
		    {uvC, uvA, uvB},
		    {uvB, uvC, uvA},
		};

		bt->sidelength = 250.0f;
		bt->height = (sqrt(3.0f) / 2.0f) * bt->sidelength;
		bt->width = bt->height / 2.0f;
		bt->x = {(float)(ScreenWidth() / 2), (float)(ScreenHeight() / 2)};
		cetrepos = {(float)(ScreenWidth() / 2), (float)(ScreenHeight() / 2)};
		TriangleCoords(*bt);
		TriangleCenter(*bt);
		MoveTriangle(*bt, bt->x);

		for (int radius = 0; radius <= max_radius; ++radius) {
			for (int q = -radius; q <= radius; ++q) {
				for (int r = -radius; r <= radius; ++r) {
					olc::vi2d current_hex = {q, r};
					if (hex_distance(current_hex) == radius) {

						Triangle upward;
						Triangle downward;
						upward.shapeb.colour = randomColour();
						downward.shapeb.colour = randomColour();
	
						int typeff = ((q - r) % 3 + 3) % 3;
						int up_uv_set_index = typeff * 2; 
						int down_uv_set_index = up_uv_set_index + 1;

						float x = (float)bt->sidelength * current_hex.x + ((float)bt->sidelength/2) * current_hex.y;
						float y = bt->height * current_hex.y;
						olc::vf2d center_point = {x + cetrepos.x, y + cetrepos.y};

						float relcenter = bt->x.y - cetrepos.y;
						float offset = center_point.y+relcenter;
						olc::vf2d p_left = {center_point.x - (bt->sidelength/2), offset};
						olc::vf2d p_right = {center_point.x + (bt->sidelength/2), offset};
						olc::vf2d p_top_left = {center_point.x, offset - bt->height};  
						olc::vf2d p_top_right = {p_top_left.x+bt->sidelength, p_top_left.y}; 

						upward.x = p_left;
						upward.y = p_right;
						upward.z = p_top_left;
						upward.texture = {uv_sets[up_uv_set_index][0], uv_sets[up_uv_set_index][1], uv_sets[up_uv_set_index][2]};
						upward.coords = {upward.x, upward.y, upward.z};

						downward.x = p_top_left;
						downward.y = p_top_right; 
						downward.z = p_right;

						downward.texture = {uv_sets[down_uv_set_index][0], uv_sets[down_uv_set_index][1], uv_sets[down_uv_set_index][2]};	
						downward.coords = {downward.x, downward.y, downward.z};

						triangles.push_back(upward); 
						triangles.push_back(downward); 
					}
				}
			}
		}
		SpawnShapes(rndInt(8, 32));
		Mask = GetDrawTarget();
		newsp = new olc::Sprite(ScreenWidth()/4, ScreenHeight()/4);	
		maskdecal = new olc::Decal(newsp);
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		Clear(olc::BLACK);	
		SetDrawTarget(newsp);
		Clear(olc::BLACK);
	
		for (Triangle &tri: bgtriangles) {	
			MoveTriangle(tri, tri.center + tri.shapeb.speed);
			RotateTriangle(tri);

			if (tri.shapeb.fill == 1) {

				FillTriangle(tri.x, tri.y, tri.z, tri.shapeb.colour);
			} else {
				DrawTriangle(tri.x, tri.y, tri.z, tri.shapeb.colour);
			}

			ReSpawnShape(tri);
		}

		for (Circle &c: circles) {
			
			c.pos += c.shapeb.speed;

			if (c.shapeb.fill == 1) {
				FillCircle(c.pos, c.radius, c.shapeb.colour);
			} else {
				DrawCircle(c.pos, c.radius, c.shapeb.colour, c.mask);
			}
			ReSpawnCircle(c);
		}

		for (Square &sq: squares) {
			sq.pos += sq.shapeb.speed;
			if (sq.shapeb.fill == 1) {
				FillRect(sq.pos, sq.size, sq.shapeb.colour);
			} else {
				DrawRect(sq.pos, sq.size, sq.shapeb.colour);
			}
			ReSpawnRect(sq);
		}
		SetPixelMode(olc::Pixel::MASK); // Draw all pixels

		SetDrawTarget(nullptr);
		maskdecal->Update();

		for (Triangle &t: triangles) {
			DrawPolygonDecal(maskdecal, t.coords, t.texture);
		}
 		return true;
	}
};
int main()
{
	Kaleidoscope demo;
	if (demo.Construct(1024, 960, 1, 1))

		demo.Start();
	return 0;
}
