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

/*TODO:
 * 4. Experiement with different shapes and types of kaleidoscope (using sources foudn in bookmarked research).
 * 5. Add sound spectrum visual feedback to shapes and kaleidoscope effects.
 * 6. Add glasslike wireframe to kaleidoscope edges with a clamping texture sampling method.
 * 7. Figure out how to optimise shape animations.
*/


#define OLC_GFX_OPENGL33
#define OLC_PGE_APPLICATION
#include "pge/olcPixelGameEngine.h"

#define OLC_PGEX_SHADERS
#include <pge/extensions/olcPGEX_Shaders.h>

// Using a transformed view to handle pan and zoom

#define OLC_PGEX_TRANSFORMEDVIEW
#include "pge/extensions/olcPGEX_TransformedView.h"
#include <opencv2/videoio.hpp>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cmath> 

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

// struct DecalShader {
// 	std::unique_ptr<olc::Decal> source = nullptr;
// 	std::unique_ptr<olc::Decal> target = nullptr;
//
// 	olc::Shade shader = {};
// 	olc::Effect effect = {};
// 	olc::EffectConfig fxconfig = {
// 		DEFAULT_VS,
// 		DEFAULT_PS,
// 		1,
// 		1
// 	};
// };


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
	float diagonal;
	olc::vf2d center;
	olc::vf2d size;
	olc::Sprite* sprite;
	olc::Decal* decal;
};

struct QuadSquare {
	ShapeBase shapeb;
	float height;
	float width;
	olc::vf2d center;
	olc::vf2d pos1;
	olc::vf2d pos2;
	olc::vf2d pos3;
	olc::vf2d pos4;
	std::vector<olc::vf2d> coords;
	std::vector<olc::vf2d> texture;
};

struct Circle {
	ShapeBase shapeb;
	float radius = 0.0f;
	double diameter = 0.0;
	uint8_t mask = 0xFF;
	olc::vf2d pos;
};

struct Line {
	olc::vf2d a = {0.0, 0.0};
	olc::vf2d b = {0.0, 0.0};
};


struct ImageSprites {
	olc::Sprite* m_pImage = nullptr;
	olc::Sprite* m_pQuantised = nullptr;
	olc::Sprite* m_pDithered = nullptr;
};

//NOTE: From MaGetzUb discord message. Utility for saving sprite to a filepath on linux using libpng.
inline bool SaveSprite(olc::Sprite* sprite, const std::string& path) {

    std::vector<png_bytep> rows(sprite->height);
    for(size_t i = 0; i < rows.size(); i++) {
        rows[i] = reinterpret_cast<png_bytep>(sprite->GetData() + sprite->width*i) ;
    }

    auto onClose = [](FILE* fp) -> void { fclose(fp); };
    auto file = std::unique_ptr<FILE, decltype(onClose)>( 
        fopen(path.c_str(), "wb"), 
        onClose
    );

    if(file == nullptr) {
        std::cerr << "Error opening file\n";
        return false;
    }

    png_structp writeStruct = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!writeStruct) {
        std::cerr << "Error creating PNG write struct\n";
        return false;
    }

    png_infop infoStruct = png_create_info_struct(writeStruct);
    if (!infoStruct) {
        std::cerr << "Error creating PNG info struct\n";
        png_destroy_write_struct(&writeStruct, NULL);
        return false;
    }

    if (setjmp(png_jmpbuf(writeStruct))) {
        std::cerr << "Error during PNG creation\n";
        png_destroy_write_struct(&writeStruct, &infoStruct);
        return false;
    }

    png_init_io(writeStruct, file.get());

    png_set_IHDR(
        writeStruct, 
        infoStruct, 
        sprite->width, sprite->height,
        8, 
        PNG_COLOR_TYPE_RGBA, 
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT, 
        PNG_FILTER_TYPE_DEFAULT
    );

    png_write_info(writeStruct, infoStruct);
    png_write_image(writeStruct, rows.data());
    png_write_end(writeStruct, NULL);
    png_destroy_write_struct(&writeStruct, &infoStruct);

    return true;

}


struct Star {
	ShapeBase shapeb;
	olc::vf2d center;
	// Innter radius
	float radius = 0.0f;
	// Outer radius
	float oradius = 0.0f;
	std::vector<olc::vf2d> inner;
	std::vector<olc::vf2d> outer;
	std::vector<Line> lines;
	std::vector<olc::vf2d> texture;
	std::vector<olc::vf2d> coords;
	olc::Sprite* sprite;
	olc::Decal* decal;
};


class Kaleidoscope : public olc::PixelGameEngine
{
public:
	Kaleidoscope()
	{
		sAppName = "Kaleidoscope";
	}

	// OLC objects for rendering to a texture.

	std::vector<Triangle> triangles;
	std::vector<Triangle> bgtriangles;
	std::vector<Circle> circles;
	std::vector<Square> squares;

	std::vector<QuadSquare> quadsquares;
	std::vector<Star> stars;

	std::vector<ImageSprites> imgsprites = {};

	int target_sprite_index = 0;

	olc::vf2d panrange = {0,0};
	float scrollspeed = 3.0f;
	float totaldistance = 0.0f;
	olc::vf2d worldoffset = {0,0};

	olc::TransformedView tv;
	olc::Sprite* overlaysprite;
	olc::Decal* overlaydecal;

	olc::Sprite* test_image;

	std::vector<olc::Sprite*> imagesprites;

	bool expand = 1;
	float pulse_freq = 0.0f;

	olc::Sprite* newsp;
	olc::Decal* maskdecal;
	olc::vf2d cetrepos;

	olc::vf2d uvA = {0.0f, 1.0f};
	olc::vf2d uvB = {1.0f, 1.0f};
	olc::vf2d uvC = {0.5f, 0.0f};

	std::vector<std::vector<olc::vf2d>> uv_sets;
	const double PIEE = 3.14159265358979323846;
	uint32_t nProcGen = 0;
	std::vector<std::string> listb;	

	Triangle baseTriangle;
	Triangle* bt = &baseTriangle;
	Triangle borderTriangle;
	const int max_radius = 5; 

	int target_sprite = 0;

	//Camera utils:
	// OpenCV video capture object.	
	olc::Sprite* pCameraFrame = nullptr;
	olc::Decal* pCameraDecal = nullptr;
	cv::VideoCapture cap;
	int nCamWidth = 640;
	int nCamHeight = 480;


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
	//    from: https://github.com/OneLoneCoder/Javidx9/blob/master/PixelGameEngine/SmallerProjects/OneLoneCoder_PGE_ProcGen_Universe.cpp
	//.................................
	//.................................

	double rndDouble(double min, double max)
	{
		return ((double)rnd() / (double)(0x7FFFFFFF)) * (max - min) + min;
	};

	int rndInt(int min, int max)
	{
		return (rnd() % (max - min)) + min;
	};
	
	uint32_t rnd()
	{
		nProcGen += 0xe120fc15;
		uint64_t tmp;
		tmp = (uint64_t)nProcGen * 0x4a39b70d;
		uint32_t m1 = (tmp >> 32) ^ tmp;
		tmp = (uint64_t)m1 * 0x12fad5c9;
		uint32_t m2 = (tmp >> 32) ^ tmp;
		return m2;
	};


	//NOTE: ...........................
	//.................................
	//    Transformation Functions 
	//.................................
	//................................

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

	void MoveQuadSquare(QuadSquare &square, olc::vf2d newcentre){
		square.pos1 = (square.pos1 - square.center) + newcentre;
		square.pos2 = (square.pos2 - square.center) + newcentre;
		square.pos3 = (square.pos3 - square.center) + newcentre;
		square.pos4 = (square.pos4 - square.center) + newcentre;
		square.center = newcentre;
	};

	void RotateSquare(QuadSquare &square){
		olc::vd2d a;
		olc::vd2d b;
		olc::vd2d c; 
		olc::vd2d d; 

		double angleRad = square.shapeb.rdegrees * (PIEE / 180);

		float cosAngle = cos(angleRad);
		float sinAngle = sin(angleRad);
		if (square.shapeb.counterclockwiserotation &1) {
			sinAngle *= -1;
		}

		square.pos1 -= square.center;
		square.pos2 -= square.center;
		square.pos3 -= square.center;
		square.pos4 -= square.center;

		a.x = (square.pos1.x * cosAngle) - (square.pos1.y * sinAngle);
		a.y = (square.pos1.x * sinAngle) + (square.pos1.y * cosAngle);
		b.x = (square.pos2.x * cosAngle) - (square.pos2.y * sinAngle);
		b.y = (square.pos2.x * sinAngle) + (square.pos2.y * cosAngle);
		c.x = (square.pos3.x * cosAngle) - (square.pos3.y * sinAngle);
		c.y = (square.pos3.x * sinAngle) + (square.pos3.y * cosAngle);
		d.x = (square.pos4.x * cosAngle) - (square.pos4.y * sinAngle);
		d.y = (square.pos4.x * sinAngle) + (square.pos4.y * cosAngle);

		a += square.center;
		b += square.center;
		c += square.center;
		d += square.center;

		square.pos1 = a;
		square.pos2 = b;
		square.pos3 = c;
		square.pos4 = d;
	};



	int hex_distance(olc::vi2d h) {
	    return (std::abs(h.x) + std::abs(h.y) + std::abs(-h.x - h.y)) / 2;
	};

	void DrawStar(Star &star) {

		const int NUM_STAR_POINTS = 5;
		const double CENTER_X = star.center.x;
		const double CENTER_Y = star.center.y;
		const double OUTER_RADIUS = star.radius * 2.5;
		star.oradius = OUTER_RADIUS;
		const double INNER_RADIUS = star.radius;  

		const int totalVertices = NUM_STAR_POINTS * 2;

		// Calculate the angle between each vertex.
		const double angleStep = M_PI / NUM_STAR_POINTS;

		// -90 degrees (pointing straight up). M_PI / 2 is 90 degrees in radians.
		const double startingAngle = -M_PI / rndDouble(0, 12);
		star.shapeb.rdegrees = startingAngle;

		Line line;
		for (int i = 0; i < totalVertices; ++i) {
			// Alternate between the outer and inner radius.
			// Even numbers (0, 2, 4...) are outer points.
			// Odd numbers (1, 3, 5...) are inner points.
			double currentRadius = (i % 2 == 0) ? OUTER_RADIUS : INNER_RADIUS;
			// Calculate the angle for the current vertex.
			double currentAngle = startingAngle + i * angleStep;
			olc::vf2d newPoint;

			newPoint.x = CENTER_X + currentRadius * cos(currentAngle);
			newPoint.y = CENTER_Y + currentRadius * sin(currentAngle);
			star.coords.push_back(newPoint);
		}

		for(size_t i = 0; i < star.coords.size(); ++i) {
		    Line line;
		    line.a = star.coords[i];
		    line.b =  star.coords[(i + 1) % star.coords.size()];
		    star.lines.push_back(line);
		}
		for (size_t i = 0; i < star.lines.size(); ++i) {
			DrawLine(star.lines[i].a, star.lines[i].b, star.shapeb.colour);
		}
	};

	void AnmiateStar(Star &star) {

		star.center += star.shapeb.speed;

		const int NUM_STAR_POINTS = 5;
		const double CENTER_X = star.center.x;
		const double CENTER_Y = star.center.y;
		const double OUTER_RADIUS = star.radius * 2.5;
		const double INNER_RADIUS = star.radius;  

		const int totalVertices = NUM_STAR_POINTS * 2;

		// Calculate the angle between each vertex.
		const double angleStep = M_PI / NUM_STAR_POINTS;

		if (star.shapeb.counterclockwiserotation &1) {	
			star.shapeb.rdegrees -= rndDouble(0.001,0.005);
		} else {

			star.shapeb.rdegrees += rndDouble(0.001,0.005);
		}


		const double startingAngle = star.shapeb.rdegrees;
		Line line;
		star.coords.clear();
		for (int i = 0; i < totalVertices; ++i) {
			// Alternate between the outer and inner radius.
			// Even numbers (0, 2, 4...) are outer points.
			// Odd numbers (1, 3, 5...) are inner points.
			double currentRadius = (i % 2 == 0) ? OUTER_RADIUS : INNER_RADIUS;
			// Calculate the angle for the current vertex.
			double currentAngle = startingAngle + i * angleStep;
			olc::vf2d newPoint;

			newPoint.x = CENTER_X + currentRadius * cos(currentAngle);
			newPoint.y = CENTER_Y + currentRadius * sin(currentAngle);
			star.coords.push_back(newPoint);
		}

		star.lines.clear();
		for(size_t i = 0; i < star.coords.size(); ++i) {
		    Line line;
		    line.a = star.coords[i];
		    line.b =  star.coords[(i + 1) % star.coords.size()];
		    star.lines.push_back(line);
		}
		for (size_t i = 0; i < star.lines.size(); ++i) {
			DrawLine(star.lines[i].a, star.lines[i].b, star.shapeb.colour);
		}
	};

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

				QuadSquare sq;
				sq.pos1 = {float(x), float(y)};
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

				if (spawn == 1) {	
					sq.width = sqsize;
					sq.height = sqsize;

					sq.pos2 = {sq.pos1.x + sqsize, sq.pos1.y};
					sq.pos3 = {sq.pos1.x, sq.pos1.y + sqsize};
					sq.pos4 = {sq.pos2.x, sq.pos3.y};
					sq.center = {sq.pos1.x + (sqsize/2), sq.pos1.y-(sqsize/2)};

					sq.shapeb.counterclockwiserotation = rndInt(0,12);
					sq.shapeb.colour = randomColour();
					quadsquares.push_back(sq);
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

				Star starr;

				starr.shapeb.layer = layerEnum[rndInt(0,3)];
				switch(starr.shapeb.layer) {
					case BG:
						starr.radius = rndDouble(1,5);
						starr.shapeb.fill = rndInt(0,3);
						spawn = rndInt(0, 12);
						break;
					case MID: 
						starr.radius = rndDouble(6,10);
						starr.shapeb.fill = rndInt(0,2);
						spawn = rndInt(0, 36);
						break;
					case FG: 
						starr.radius = rndDouble(10,20);
						starr.shapeb.fill = rndInt(0,2);
						spawn = rndInt(0, 72);
						break;
				}

				if (spawn == 1) {
					starr.shapeb.counterclockwiserotation = rndInt(0,12);
					starr.shapeb.speed = {0, (float)rndDouble(0.01, 0.04)};
					starr.center = {float(x), float(y)};
					starr.shapeb.colour = randomColour(); 	
					DrawStar(starr);
					stars.push_back(starr);
				}
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
	};

	void RespawnQuadSquare(QuadSquare &sq) {
		olc::vf2d newc = {0,0};
		float min = std::min({sq.pos1.y, sq.pos2.y, sq.pos3.y, sq.pos4.y});
		min = sq.center.y - min;
		float newx = rndDouble(0, float(ScreenWidth()));
		if (( sq.center.y - min) >= ScreenHeight())  {
			sq.shapeb.colour = randomColour();
			newc = {newx, ((0 - min))};
			MoveQuadSquare(sq, newc);
		}
	};

	void ReSpawnCircle(Circle &circle) {
		olc::vf2d newc = {0,0};
		if (( circle.pos.y - circle.radius) >= ScreenHeight())  {
			float newx = rndDouble(0, float(ScreenWidth()));
			circle.shapeb.colour = randomColour();
			newc = {newx, ((0 - circle.radius))};
			circle.pos = newc;
		}
	};

	void ReSpawnStar(Star &star) {
		olc::vf2d newc = {0,0};
		if (( star.center.y - star.oradius) >= ScreenHeight())  {
			float newx = rndDouble(0, float(ScreenWidth()));
			star.shapeb.colour = randomColour();
			newc = {newx, ((0 - star.oradius))};
			star.center = newc;
		}
	};

	void ReSpawnRect(Square &square) {
		olc::vf2d newc = {0,0};
		if (( square.pos.y - square.size.y) >= ScreenHeight())  {
			float newx = rndDouble(0, float(ScreenWidth()));
			square.shapeb.colour = randomColour();
			newc = {newx, ((0 - square.size.y))};
			square.pos = newc;
		}
	};

	void AnimateShapes() {
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

		for (Star &star: stars) {
			AnmiateStar(star);
			ReSpawnStar(star);
		}


		for (QuadSquare &sq: quadsquares) {
			MoveQuadSquare(sq, sq.center + sq.shapeb.speed);
			RotateSquare(sq);
			if (sq.shapeb.fill == 1) {
				FillQuadRect(sq.pos1, sq.pos2, sq.pos3, sq.pos4, sq.shapeb.colour);
			} else {
				DrawQuadRect(sq.pos1, sq.pos2, sq.pos3, sq.pos4, sq.shapeb.colour);
			}
			RespawnQuadSquare(sq);
		}
	};


	//NOTE: From https://github.com/OneLoneCoder/Javidx9/blob/master/PixelGameEngine/SmallerProjects/OneLoneCoder_PGE_Dithering.cpp 
	void ScrollImageInit(olc::Sprite &image, olc::Sprite &quantized, olc::Sprite &dithered) 
	{

		// These lambda functions output a new olc::Pixel based on
		// the pixel it is given
		auto Convert_RGB_To_Greyscale = [](const olc::Pixel in)
		{
			uint8_t greyscale = uint8_t(0.2162f * float(in.r) + 0.7152f * float(in.g) + 0.0722f * float(in.b));
			return olc::Pixel(greyscale, greyscale, greyscale);
		};


		// Quantising functions
		auto Quantise_Greyscale_1Bit = [](const olc::Pixel in)
		{
			return in.r < 128 ? olc::BLACK : olc::WHITE;
		};

		auto Quantise_Greyscale_NBit = [](const olc::Pixel in)
		{
			constexpr int nBits = 2;
			constexpr float fLevels = (1 << nBits) - 1;
			uint8_t c = uint8_t(std::clamp(std::round(float(in.r) / 255.0f * fLevels) / fLevels * 255.0f, 0.0f, 255.0f));
			return olc::Pixel(c, c, c);
		};

		auto Quantise_RGB_NBit = [](const olc::Pixel in)
		{
			constexpr int nBits = 2;
			constexpr float fLevels = (1 << nBits) - 1;
			uint8_t cr = uint8_t(std::clamp(std::round(float(in.r) / 255.0f * fLevels) / fLevels * 255.0f, 0.0f, 255.0f));
			uint8_t cb = uint8_t(std::clamp(std::round(float(in.g) / 255.0f * fLevels) / fLevels * 255.0f, 0.0f, 255.0f));
			uint8_t cg = uint8_t(std::clamp(std::round(float(in.b) / 255.0f * fLevels) / fLevels * 255.0f, 0.0f, 255.0f));
			return olc::Pixel(cr, cb, cg);
		};

		auto Quantise_RGB_CustomPalette = [](const olc::Pixel in)
		{
			std::array<olc::Pixel, 5> nShades = { olc::BLACK, olc::WHITE, olc::YELLOW, olc::MAGENTA, olc::CYAN };
			
			float fClosest = INFINITY;
			olc::Pixel pClosest;

			for (const auto& c : nShades)
			{
				float fDistance = float(
					std::sqrt(
						std::pow(float(c.r) - float(in.r), 2) +
						std::pow(float(c.g) - float(in.g), 2) +
						std::pow(float(c.b) - float(in.b), 2)));

				if (fDistance < fClosest)
				{
					fClosest = fDistance;
					pClosest = c;
				}
			}
						
			return pClosest;
		};



		std::transform(
			image.pColData.begin(),
			image.pColData.end(),
			quantized.pColData.begin(), Quantise_RGB_NBit);

		Dither_FloydSteinberg(&image, &dithered, Quantise_RGB_NBit);
	};


	//NOTE: From https://github.com/OneLoneCoder/Javidx9/blob/master/PixelGameEngine/SmallerProjects/OneLoneCoder_PGE_Dithering.cpp 
	void Dither_FloydSteinberg(const olc::Sprite* pSource, olc::Sprite* pDest, std::function<olc::Pixel(const olc::Pixel)> funcQuantise)
	{		
		// The destination image is primed with the source image as the pixel
		// values become altered as the algorithm executes
		std::copy(pSource->pColData.begin(), pSource->pColData.end(), pDest->pColData.begin());		

		// Iterate through each pixel from top left to bottom right, compare the pixel
		// with that on the "allowed" list, and distribute that error to neighbours
		// not yet computed
		olc::vi2d vPixel;
		for (vPixel.y = 0; vPixel.y < pSource->height; vPixel.y++)
		{
			for (vPixel.x = 0; vPixel.x < pSource->width; vPixel.x++)
			{
				// Grap and get nearest pixel equivalent from our allowed
				// palette
				olc::Pixel op = pDest->GetPixel(vPixel);
				olc::Pixel qp = funcQuantise(op);

				// olc::Pixels are "inconveniently" clamped to sensible ranges using an unsigned type...
				// ...which means they cant be negative. This hampers us a tad here,
				// so will resort to manual alteration using a signed type
				int32_t error[3] =
				{
					op.r - qp.r,
					op.g - qp.g,
					op.b - qp.b
				};
				
				// Set destination pixel with nearest match from quantisation function
				pDest->SetPixel(vPixel, qp);

				// Distribute Error - Using a little utility lambda to keep the messy code
				// all in one place. It's important to allow pixels to temporarily become
				// negative in order to distribute the error to the neighbours in both
				// directions... value directions that is, not spatial!				
				auto UpdatePixel = [&vPixel, &pDest, &error](const olc::vi2d& vOffset, const float fErrorBias)
				{
					olc::Pixel p = pDest->GetPixel(vPixel + vOffset);
					int32_t k[3] = { p.r, p.g, p.b };
					k[0] += int32_t(float(error[0]) * fErrorBias);
					k[1] += int32_t(float(error[1]) * fErrorBias);
					k[2] += int32_t(float(error[2]) * fErrorBias);
					pDest->SetPixel(vPixel + vOffset, olc::Pixel(std::clamp(k[0], 0, 255), std::clamp(k[1], 0, 255), std::clamp(k[2], 0, 255)));
				};

				UpdatePixel({ +1,  0 }, 7.0f / 16.0f);
				UpdatePixel({ -1, +1 }, 3.0f / 16.0f);
				UpdatePixel({  0, +1 }, 5.0f / 16.0f);
				UpdatePixel({ +1, +1 }, 1.0f / 16.0f);
			}
		}
	};


	
	bool setup_cam() {
		cap.open(0);
		if (!cap.isOpened())
		{
			std::cerr << "ERROR: Could not open camera device." << std::endl;
			return false;
		}

		cap.set(cv::CAP_PROP_FRAME_WIDTH, nCamWidth);
		cap.set(cv::CAP_PROP_FRAME_HEIGHT, nCamHeight);
		
		pCameraFrame = new olc::Sprite(nCamWidth, nCamHeight);
		pCameraDecal = new olc::Decal(pCameraFrame);
		return true;
	};

	bool RenderCameraFrame(){
		cv::Mat frame;
		cap.read(frame); 

		if (frame.empty())
		{
			return 1;
		}
		
		// --- Update the Sprite with the new frame data ---
		for (int y = 0; y < frame.rows; y++)
		{
			for (int x = 0; x < frame.cols; x++)
			{
				cv::Vec3b color = frame.at<cv::Vec3b>(y, x);
				pCameraFrame->SetPixel(x, y, olc::Pixel(color[2], color[1], color[0])); // R, G, B
			}
		}

		return 0;
	};


	bool OnUserCreate() override
	{
		//SetPixelBlend(1.0);

		tv.Initialise({ ScreenWidth(), ScreenHeight() });

		if (!setup_cam()) return false;

		std::string ilist = "imagelist.txt";
		std::vector<std::string> lines;
		std::ifstream inputFile(ilist);
		std::string line;
		while (std::getline(inputFile, line)) {
			ImageSprites is;	
			std::ostringstream fp;
			//NOTE:: Images take from https://science.nasa.gov/mission/webb/
			fp << "SpaceImages/";
			fp << line;
			lines.push_back(fp.str());
			is.m_pImage = new olc::Sprite(fp.str());
			is.m_pQuantised = new olc::Sprite(is.m_pImage->width, is.m_pImage->height);
			is.m_pDithered = new olc::Sprite(is.m_pImage->width, is.m_pImage->height);
			ScrollImageInit(*is.m_pImage, *is.m_pDithered, *is.m_pQuantised);
			imgsprites.push_back(is);
		}

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
		bt->shapeb.colour = randomColour();
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

		newsp = new olc::Sprite(ScreenWidth()/4, ScreenHeight()/4);	
		maskdecal = new olc::Decal(newsp);

		overlaysprite = new olc::Sprite(ScreenWidth(), ScreenHeight());
		overlaydecal = new olc::Decal(overlaysprite);

		SpawnShapes(rndInt(8, 32));

		panrange = {(float)ScreenWidth()/2, (float)ScreenWidth()/2};

		tv.StartPan(panrange);

		worldoffset = tv.GetWorldOffset();
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		// Handle Pan & Zoom using defaults middle mouse button
		//SetDrawTarget(nullptr);
		Clear(olc::BLANK);	
		SetPixelMode(olc::Pixel::ALPHA);

		if (totaldistance == 0) {
			olc::vf2d newoffs = {(float)ScreenWidth()/2, imgsprites[target_sprite_index].m_pImage->height - ((float)ScreenHeight())}; 
			worldoffset = newoffs;
			tv.SetWorldOffset(newoffs);
			panrange = {((float)ScreenWidth()/2), ((float)ScreenHeight()/2)};
			tv.StartPan(panrange);
		}

		totaldistance += scrollspeed;

		if (totaldistance >= imgsprites[target_sprite_index].m_pImage->height - ((float)ScreenHeight())) {
			totaldistance = 0;
			target_sprite_index = rndInt(0, 6);
		}

		tv.UpdatePan({ScreenWidth()/2, (int)panrange.y + (int)totaldistance});		
		
		SetDrawTarget(newsp);
		Clear(olc::BLANK);
		SetPixelMode(olc::Pixel::Mode::ALPHA);


		if (GetKey(olc::Key::NP0).bPressed)
		{
			target_sprite = 0;
		}

		if (GetKey(olc::Key::NP1).bPressed)
		{
			target_sprite = 1;
		}

		if (GetKey(olc::Key::NP2).bPressed)
		{

			target_sprite = 2;
		}

		if (GetKey(olc::Key::NP3).bPressed)
		{
			target_sprite = 3;
		}

		if (GetKey(olc::Key::NP4).bPressed)
		{
			target_sprite = 4;
		}

		if (GetKey(olc::Key::NP5).bPressed)
		{
			target_sprite = 5;
		}

		if (target_sprite == 1) {
			bool render;
			render = RenderCameraFrame();
			
			if (render){
				return 1;
			}
			DrawSprite({0, 0}, pCameraFrame);
		}

		if (target_sprite == 2) {
			AnimateShapes();
		}

		if (target_sprite == 3) {
			tv.DrawSprite({ 0,0 }, imgsprites[target_sprite_index].m_pImage);
		}
		if (target_sprite == 4) {
			tv.DrawSprite({ 0,0 }, imgsprites[target_sprite_index].m_pDithered);
		}
		if (target_sprite == 5) {
			tv.DrawSprite({ 0,0 }, imgsprites[target_sprite_index].m_pQuantised);
		}



		SetDrawTarget(nullptr);
		Clear(olc::BLANK);

		maskdecal->Update();

		if (target_sprite != 0){
			for (Triangle &t: triangles) {
				DrawPolygonDecal(maskdecal, t.coords, t.texture);
				DrawTriangle(t.x, t.y, t.z, olc::VERY_DARK_RED);
			};
		} else {
			Clear(olc::DARK_BLUE);
			DrawStringDecal({10, 10}, "Press NumPad0 to come back here.", olc::WHITE, {2.0f, 2.0f});
			DrawStringDecal({10, 40}, "Press NumPad1 show webcam feed", olc::WHITE, {2.0f, 2.0f});
			DrawStringDecal({10, 70}, "Press NumPad2 to draw shapes into kaleidoscope viewport.", olc::WHITE, {2.0f, 2.0f});
			DrawStringDecal({10, 100}, "Press NumPad3 to draw space telescope images", olc::WHITE, {2.0f, 2.0f});	
			DrawStringDecal({10, 130}, "into the kaleidoscope viewport", olc::WHITE, {2.0f, 2.0f});
			DrawStringDecal({10, 160}, "Press NumPad4 to dithered draw space telescope images", olc::WHITE, {2.0f, 2.0f});	
			DrawStringDecal({10, 190}, "into the kaleidoscope viewport", olc::WHITE, {2.0f, 2.0f});
			DrawStringDecal({10, 210}, "Press NumPad5 to dithered draw quantized space telescope images", olc::WHITE, {2.0f, 2.0f});	
			DrawStringDecal({10, 240}, "into the kaleidoscope viewport", olc::WHITE, {2.0f, 2.0f});

		}
		overlaydecal->UpdateSprite();

		if(GetKey(olc::Key::INS).bPressed) {
			std::chrono::time_point<std::chrono::system_clock> tp1;
			tp1 = std::chrono::system_clock::now();
			time_t now;
			time(&now);
			std::string dateandtime = ctime(&now);
			std::string filename = "./Screenshots/pge_kaleidoscope_";
			filename += dateandtime;
			filename += ".png";
			std::replace(filename.begin(), filename.end(),' ','_');
			SaveSprite(overlaysprite, filename);
		}

 		return true;
	}

	bool OnUserDestroy() override
	{
		delete pCameraDecal;
		delete pCameraFrame;

		if(cap.isOpened())
		{
			cap.release();
		}
		return true;
	}

};


int main()
{
	Kaleidoscope demo;
	if (demo.Construct(1024, 960, 1, 1))

	//if (demo.Construct(1280, 720, 1, 1))
		demo.Start();
	return 0;
}
