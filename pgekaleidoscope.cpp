#define OLC_PGE_APPLICATION

#include <pge/olcPixelGameEngine.h>
#include <pge/extensions/olcPGEX_Graphics3D.h>
#include <vector>
#include <algorithm> // For std::swap
//#include <pge/extensions/olcPGEX_TransformedView.h>
#include <random>
//olc::Pixel	shapeColour = olc::WHITE;

constexpr uint32_t shapeColours[8] = 
{
	0xFFFFFFFF, 0xFFD9FFFF, 0xFFA3FFFF, 0xFFFFC8C8,
	0xFFFFCB9D, 0xFF9F9FFF, 0xFF415EFF, 0xFF28199D
};

// enum ColourPalette { 
//
// 	GREY, DARK_GREY, VERY_DARK_GREY, RED, 
// 	DARK_RED, VERY_DARK_RED, YELLOW, DARK_YELLOW,
// 	VERY_DARK_YELLOW, GREEN, DARK_GREEN, VERY_DARK_GREEN,
// 	CYAN, DARK_CYAN, VERY_DARK_CYAN, BLUE, 
// 	DARK_BLUE, VERY_DARK_BLUE, MAGENTA, DARK_MAGENTA, 
// 	VERY_DARK_MAGENTA, WHITE, BLACK 
// };
//
// constexpr ColourPalette colourArray[23] = 
// {
// 	GREY, DARK_GREY, VERY_DARK_GREY, RED, 
// 	DARK_RED, VERY_DARK_RED, YELLOW, DARK_YELLOW,
// 	VERY_DARK_YELLOW, GREEN, DARK_GREEN, VERY_DARK_GREEN,
// 	CYAN, DARK_CYAN, VERY_DARK_CYAN, BLUE, 
// 	DARK_BLUE, VERY_DARK_BLUE, MAGENTA, DARK_MAGENTA, 
// 	VERY_DARK_MAGENTA, WHITE, BLACK 
// };

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
	olc::vf2d speed;
	int counterclockwiserotation;
};

struct Triangle {
	ShapeBase shapeb;
	float sidelength;
	float height;
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
	Triangle viewtri;
	std::vector<olc::vi2d> vtriview;
	std::vector<Triangle> triangles;
	std::vector<Triangle>* vptriangles;
	std::vector<Triangle*> ptriangles;
	std::vector<Circle> circles;
	std::vector<Square> squares;
	olc::vf2d vKaleidoscopeOffset = { 0,0 };
	bool expand = 1;
	float pulse_freq = 0.0f;

	olc::Sprite* Mask;
	olc::Sprite* Mask2;
	olc::Sprite* newsp;
	olc::Decal* maskdecal;
	olc::Decal* maskdecal2;
	olc::vf2d mpos;
	olc::vf2d msourcepos;
	olc::vi2d msize;

	int clusterSize;
	const double PIEE = 3.14159265358979323846;

	const double PI = 3.14159265358979323846;
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
		float height = triangle.sidelength * sqrt(3) / 2;
		triangle.height = height;
		triangle.pos2 = {triangle.pos.x+(triangle.sidelength/2), triangle.pos.y-height};
		triangle.pos3 = {triangle.pos.x+triangle.sidelength, triangle.pos.y};
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
		//Checking if its odd or even using binary
		if (triangle.shapeb.counterclockwiserotation &1) {
			sinAngle *= -1;
		}

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
						spawn = rndInt(0, 3);
						break;
					case MID: 
						t.sidelength = rndInt(20,40);
						t.shapeb.fill = rndInt(0,3);
						spawn = rndInt(0, 5);
						break;
					case FG: 
						t.sidelength = rndInt(40, 60);
						t.shapeb.fill = rndInt(0,3);
						spawn = rndInt(0, 4);
						break;
				}
				t.shapeb.counterclockwiserotation = rndInt(0,12);

				t.shapeb.speed = {0, (float)rndDouble(0.1, 0.3)};
				t.shapeb.colour = shapeColours[rndInt(0,8)];
				t.pos = {float(x), float(y)};
				//Make Equilateral: 
				TriangleCoords(t);
				TriangleCenter(t);
				t.shapeb.rdegrees = rndDouble(0.1f, 0.2f);

				if (spawn == 1) {
					triangles.push_back(t);
				}
				spawn=1;

				//Squares
				Square sq;

				sq.pos = {float(x), float(y)};
				sq.shapeb.layer = layerEnum[rndInt(0,3)];

				sq.shapeb.speed = {0, (float)rndDouble(0.1, 0.3)};
				float sqsize;
				switch(sq.shapeb.layer) {
					case BG: 
						sqsize = rndInt(1,4);
						sq.shapeb.fill = 1;
						spawn = rndInt(0, 2);
						break;
					case MID: 
						sqsize = rndInt(5,10);
						sq.shapeb.fill = rndInt(0,3);
						spawn = rndInt(0, 10);
						break;
					case FG: 
						sqsize = rndInt(20,40);

						sq.shapeb.fill = rndInt(0,2);
						spawn = rndInt(0, 10);
						break;
				}

				sq.size = {sqsize, sqsize};;
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

				c.shapeb.speed = {0, (float)rndDouble(0.1, 0.3)};
				switch(c.shapeb.layer) {
					case BG: 
						c.diameter = rndInt(1, 5);
						c.shapeb.fill = 1;
						spawn = rndInt(0, 3);
						break;
					case MID: 
						c.diameter = rndInt(5, 10);

						c.shapeb.fill = rndInt(0,3);
						spawn = rndInt(0, 5);
						break;
					case FG: 
						c.diameter = rndInt(20, 40);
						c.shapeb.fill = rndInt(0,2);
						spawn = rndInt(0, 10);
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
		float min = std::min({triangle.pos.y, triangle.pos2.y, triangle.pos3.y});
		min = triangle.center.y - min;

		float newx = rndDouble(0, float(ScreenWidth()));
		if (( triangle.center.y - min) >= ScreenHeight())  {
			//triangle.shapeb.speed.y = rndDouble(0.1, 0.3);
			triangle.shapeb.colour = shapeColours[rndInt(0,8)];
			newc = {newx, ((0 - min))};
			MoveTriangle(triangle, newc);
		}
	}

	void ReSpawnCircle(Circle &circle) {
		olc::vf2d newc = {0,0};
		if (( circle.pos.y - circle.radius) >= ScreenHeight())  {

			//circle.shapeb.speed.y = rndDouble(0.1, 0.3);
			float newx = rndDouble(0, float(ScreenWidth()));
			circle.shapeb.colour = shapeColours[rndInt(0,8)];
			newc = {newx, ((0 - circle.radius))};
			circle.pos = newc;
		}
	}


	void ReSpawnRect(Square &square) {
		olc::vf2d newc = {0,0};
		if (( square.pos.y - square.size.y) >= ScreenHeight())  {

			//square.shapeb.speed.y = rndDouble(0.1, 0.3);
			float newx = rndDouble(0, float(ScreenWidth()));
			square.shapeb.colour = shapeColours[rndInt(0,8)];
			newc = {newx, ((0 - square.size.y))};
			square.pos = newc;
		}
	}



	bool OnUserCreate() override
	{
		//SetPixelBlend(1.0);

		viewtri.sidelength = 250;
		viewtri.shapeb.fill = 1;
		viewtri.shapeb.colour = shapeColours[2];
		viewtri.pos = {(float)(ScreenWidth() / 2), (float)(ScreenHeight() / 2)};
		TriangleCoords(viewtri);
		TriangleCenter(viewtri);
		MoveTriangle(viewtri, viewtri.pos);

		SpawnShapes(rndInt(8, 32));

		//FillTriangle(viewtri.pos, viewtri.pos2, viewtri.pos3, olc::BLANK);

		//msize = {(int)viewtri.sidelength,(int)viewtri.height};	
		Mask = GetDrawTarget();

		//msourcepos = {viewtri.pos.x, viewtri.pos.y-viewtri.height};

		newsp = new olc::Sprite(ScreenWidth()/4, ScreenHeight()/4);	
		//Mask2 = new olc::Sprite(ScreenWidth(), ScreenHeight());	

		maskdecal = new olc::Decal(newsp);
		maskdecal2 = new olc::Decal(Mask);
		// for (int i = 0; i < newsp->width; i++) {
		// 	for (int y = 0; y < newsp->height; y++){
		// 		olc::Pixel sy = Mask->GetPixel(i,y);
		// 		// if (sy != olc::BLACK) {
		// 		// 	sy = olc::BLANK;
		// 		// }
		// 		newsp->SetPixel(i, y, sy);
		// 	}
		// } 




		

		//void DrawPartialSprite(const olc::vi2d& pos, Sprite* sprite, const olc::vi2d& sourcepos, const olc::vi2d& size, uint32_t scale = 1, uint8_t flip = olc::Sprite::NONE);

		//void PixelGameEngine::DrawPartialSprite(int32_t x, int32_t y, Sprite* sprite, int32_t ox, int32_t oy, int32_t w, int32_t h, uint32_t scale, uint8_t flip)

		

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



	
		SetDrawTarget(newsp);

		Clear(olc::BLACK);
	
		for (Triangle &tri: triangles) {	
			MoveTriangle(tri, tri.center + tri.shapeb.speed);
			RotateTriangle(tri);

			if (tri.shapeb.fill == 1) {

				FillTriangle(tri.pos, tri.pos2, tri.pos3, tri.shapeb.colour);
			} else {
				DrawTriangle(tri.pos, tri.pos2, tri.pos3, tri.shapeb.colour);
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

		//SetPixelMode(olc::Pixel::MASK); // Draw all pixels
		//

		SetDrawTarget(nullptr);
		//SetDrawTarget(Mask2);
		maskdecal->Update();

		//NOTE: Here follows terrible spaghetti code that should be put into structs, but I got carried away with adding more and more polygon decals.
		// Too late to refactor now sorry ! 
		//..............
		//.............
		//............
		//...........
		//..........
		//.........
		//........
		//.......
		//......
		//.....
		//....
		//...
		//..
		//.



		//Center
		olc::vf2d cpos1 = viewtri.pos;
		olc::vf2d cpos2 =  viewtri.pos2;
		olc::vf2d cpos3 =  viewtri.pos3;
		std::vector<olc::vf2d> cpos;
		cpos.push_back(cpos1);
		cpos.push_back(cpos2);
		cpos.push_back(cpos3);

		std::vector<olc::vf2d> c_textures = {{0,1}, {0.5,0.0}, {1,1}}; 
		DrawPolygonDecal(maskdecal, cpos, c_textures);

		//LeftCenter
		olc::vf2d lcpos1 = cpos1;
		olc::vf2d lcpos2 = cpos2;
		olc::vf2d lcpos3 = {viewtri.pos.x-(viewtri.sidelength/2), viewtri.pos2.y};
		std::vector<olc::vf2d> lcpos;
		lcpos.push_back(lcpos1);
		lcpos.push_back(lcpos2);
		lcpos.push_back(lcpos3);

		std::vector<olc::vf2d> lc_textures = {{0,1}, {0.5,0.0}, {1,1}}; 
		DrawPolygonDecal(maskdecal, lcpos,lc_textures);

		//Rightcenter
		olc::vf2d rcpos1 = cpos3;
		olc::vf2d rcpos2 = cpos2;
		olc::vf2d rcpos3 = {viewtri.pos3.x+(viewtri.sidelength/2), viewtri.pos2.y};
		std::vector<olc::vf2d> rcpos;
		rcpos.push_back(rcpos1);
		rcpos.push_back(rcpos2);
		rcpos.push_back(rcpos3);

		std::vector<olc::vf2d> rc_textures = {{1,1}, {0.5,0.0}, {0,1}}; 
		DrawPolygonDecal(maskdecal, rcpos, rc_textures);

		//Bottomcenter
		olc::vf2d bcpos1 = cpos1;
		olc::vf2d bcpos2 = {viewtri.pos2.x,viewtri.pos2.y+(viewtri.height*2)};
		olc::vf2d bcpos3 = cpos3;
		std::vector<olc::vf2d> bcpos;
		bcpos.push_back(bcpos1);
		bcpos.push_back(bcpos2);
		bcpos.push_back(bcpos3);

		std::vector<olc::vf2d> bc_textures = {{0,1}, {0.5,0.0}, {1,1}}; 
		DrawPolygonDecal(maskdecal, bcpos, bc_textures);
	


		//LeftBottom
		olc::vf2d lbcpos1 = {bcpos2.x - (viewtri.sidelength), bcpos2.y};
		olc::vf2d lbcpos2 = cpos1;
		olc::vf2d lbcpos3 = bcpos2;
		std::vector<olc::vf2d> lbcpos;
		lbcpos.push_back(lbcpos1);
		lbcpos.push_back(lbcpos2);
		lbcpos.push_back(lbcpos3);

		std::vector<olc::vf2d> lbc_textures = {{1,1}, {0,1},{0.5,0.0}}; 
		DrawPolygonDecal(maskdecal, lbcpos, lbc_textures);

		//RightBottomCenter
		olc::vf2d rbcpos1 = bcpos2;
		olc::vf2d rbcpos2 = cpos3;
		olc::vf2d rbcpos3 = {bcpos2.x + viewtri.sidelength, bcpos2.y};
		std::vector<olc::vf2d> rbcpos;
		rbcpos.push_back(rbcpos1);
		rbcpos.push_back(rbcpos2);
		rbcpos.push_back(rbcpos3);

		std::vector<olc::vf2d> rbc_textures = {{0.5,0.0}, {1,1},{0,1}}; 
		DrawPolygonDecal(maskdecal, rbcpos, rbc_textures);

		//TopCenterLeft
		olc::vf2d tclpos1 = lcpos3;
		olc::vf2d tclpos2 = cpos2;
		olc::vf2d tclpos3 = {cpos1.x, cpos1.y-(viewtri.height*2)};
		std::vector<olc::vf2d> tclpos;
		tclpos.push_back(tclpos1);
		tclpos.push_back(tclpos2);
		tclpos.push_back(tclpos3);

		std::vector<olc::vf2d> tcl_textures = {{1,1}, {0.5,0.0},{0,1}}; 
		DrawPolygonDecal(maskdecal, tclpos, tcl_textures);

		//TopCentreCentre
		olc::vf2d tccpos1 = tclpos3;
		olc::vf2d tccpos2 = cpos2;
		olc::vf2d tccpos3 = {tclpos3.x + viewtri.sidelength, tclpos3.y};
		std::vector<olc::vf2d> tccpos;
		tccpos.push_back(tccpos1);
		tccpos.push_back(tccpos2);
		tccpos.push_back(tccpos3);

		std::vector<olc::vf2d> tcc_textures = {{0,1},{0.5,0.0},{1,1}}; 
		DrawPolygonDecal(maskdecal, tccpos, tcc_textures);

		//TopCentreRight
		olc::vf2d tcrpos1 = cpos2;
		olc::vf2d tcrpos2 = tccpos3;
		olc::vf2d tcrpos3 = rcpos3;
		std::vector<olc::vf2d> tcrpos;
		tcrpos.push_back(tcrpos1);
		tcrpos.push_back(tcrpos2);
		tcrpos.push_back(tcrpos3);

		std::vector<olc::vf2d> tcr_textures = {{0.5,0.0}, {1,1},{0,1}}; 
		DrawPolygonDecal(maskdecal, tcrpos, tcr_textures);

		//LeftCentertop
		olc::vf2d lctpos1 = {cpos1.x - viewtri.sidelength, cpos1.y};
		olc::vf2d lctpos2 = lcpos3;
		olc::vf2d lctpos3 = cpos1;
		std::vector<olc::vf2d> lctpos;
		lctpos.push_back(lctpos1);
		lctpos.push_back(lctpos2);
		lctpos.push_back(lctpos3);

		std::vector<olc::vf2d> lct_textures = {{0.5,0.0}, {1,1},{0,1}}; 
		DrawPolygonDecal(maskdecal, lctpos, lct_textures);

		//Leftcenterbottom
		olc::vf2d lcbpos1 = lctpos1;
		olc::vf2d lcbpos2 = lbcpos1;
		olc::vf2d lcbpos3 = cpos1;
		std::vector<olc::vf2d> lcbpos;
		lcbpos.push_back(lcbpos1);
		lcbpos.push_back(lcbpos2);
		lcbpos.push_back(lcbpos3);

		std::vector<olc::vf2d> lcb_textures = {{0.5,0.0}, {1,1},{0,1}}; 
		DrawPolygonDecal(maskdecal, lcbpos, lcb_textures);

		//Toptop
		olc::vf2d ttpos1 = tccpos1;
		olc::vf2d ttpos2 = {tccpos2.x, tccpos2.y-(viewtri.height*2)};
		olc::vf2d ttpos3 = tccpos3;
		std::vector<olc::vf2d> ttpos;
		ttpos.push_back(ttpos1);
		ttpos.push_back(ttpos2);
		ttpos.push_back(ttpos3);
		//std::vector<olc::vf2d> tt_textures = {{0.5,0.0}, {1,1},{0,1}}; 
		DrawPolygonDecal(maskdecal, ttpos, c_textures);




		olc::vf2d llpos1 = {lcbpos2.x - viewtri.sidelength, lcbpos2.y};
		olc::vf2d llpos2 = lcbpos2;
		olc::vf2d llpos3 = lcbpos1;
		std::vector<olc::vf2d> llpos;
		llpos.push_back(llpos1);
		llpos.push_back(llpos2);
		llpos.push_back(llpos3);

		std::vector<olc::vf2d> ll_textures = {{0,1},{1,1},{0.5,0.0}}; 
		DrawPolygonDecal(maskdecal, llpos, ll_textures);





		olc::vf2d rcrpos1 = cpos3;
		olc::vf2d rcrpos2 = tcrpos3;
		olc::vf2d rcrpos3 = {cpos3.x + viewtri.sidelength, cpos1.y};
		std::vector<olc::vf2d> rcrpos;
		rcrpos.push_back(rcrpos1);
		rcrpos.push_back(rcrpos2);
		rcrpos.push_back(rcrpos3);

		std::vector<olc::vf2d> rcr_textures = {{1,1},{0,1},{0.5,0.0}}; 
		DrawPolygonDecal(maskdecal, rcrpos, rcr_textures);




		olc::vf2d rbbpos1 = cpos3;
		olc::vf2d rbbpos2 = rbcpos3;
		olc::vf2d rbbpos3 = rcrpos3;
		std::vector<olc::vf2d> rbbpos;
		rbbpos.push_back(rbbpos1);
		rbbpos.push_back(rbbpos2);
		rbbpos.push_back(rbbpos3);

		std::vector<olc::vf2d> rbb_textures = {{1,1},{0,1},{0.5,0.0}}; 
		DrawPolygonDecal(maskdecal, rbbpos, rbb_textures);




		olc::vf2d rrpos1 = rbcpos3;
		olc::vf2d rrpos2 = rcrpos3;
		olc::vf2d rrpos3 = {rbcpos3.x + viewtri.sidelength, rbcpos3.y};
		std::vector<olc::vf2d> rrpos;
		rrpos.push_back(rrpos1);
		rrpos.push_back(rrpos2);
		rrpos.push_back(rrpos3);
		
		std::vector<olc::vf2d> rr_textures = {{1,1},{0,1},{0.5,0.0}}; 
		DrawPolygonDecal(maskdecal, rrpos, c_textures);




		olc::vf2d reb1pos1 = rrpos2;
		olc::vf2d reb1pos2 = rrpos3;
		olc::vf2d reb1pos3 = {rrpos2.x + viewtri.sidelength, rrpos2.y};
		std::vector<olc::vf2d> reb1pos;
		reb1pos.push_back(reb1pos1);
		reb1pos.push_back(reb1pos2);
		reb1pos.push_back(reb1pos3);
		
		std::vector<olc::vf2d> reb1_textures = {{0.5,0.0},{1,1},{0,1}}; 
		DrawPolygonDecal(maskdecal, reb1pos, reb1_textures);


		olc::vf2d rem1pos1 = rrpos2;
		olc::vf2d rem1pos2 = {reb1pos2.x, reb1pos2.y-(viewtri.height*2)};
		olc::vf2d rem1pos3 = reb1pos3;
		std::vector<olc::vf2d> rem1pos;
		rem1pos.push_back(rem1pos1);
		rem1pos.push_back(rem1pos2);
		rem1pos.push_back(rem1pos3);
		
		std::vector<olc::vf2d> rem1_textures = {{0.5,0.0}, {1,1},{0,1},}; 
		DrawPolygonDecal(maskdecal, rem1pos, rem1_textures);

		olc::vf2d rem2pos1 = rcrpos2;
		olc::vf2d rem2pos2 = rem1pos1;
		olc::vf2d rem2pos3 = rem1pos2;
		std::vector<olc::vf2d> rem2pos;
		rem2pos.push_back(rem2pos1);
		rem2pos.push_back(rem2pos2);
		rem2pos.push_back(rem2pos3);
		
		std::vector<olc::vf2d> rem2_textures = { {0,1},{0.5,0.0},{1,1},}; 
		DrawPolygonDecal(maskdecal, rem2pos, rem2_textures);


		olc::vf2d rem3pos1 = rem2pos1;
		olc::vf2d rem3pos2 = {rem2pos2.x, rem2pos2.y-(viewtri.height*2)};
		olc::vf2d rem3pos3 = rem2pos3;
		std::vector<olc::vf2d> rem3pos;
		rem3pos.push_back(rem3pos1);
		rem3pos.push_back(rem3pos2);
		rem3pos.push_back(rem3pos3);
		
		std::vector<olc::vf2d> rem3_textures = { {0,1},{0.5,0.0},{1,1},}; 
		DrawPolygonDecal(maskdecal, rem3pos, rem3_textures);

		olc::vf2d rem4pos1 = tcrpos2;
		olc::vf2d rem4pos2 = rem3pos1; //{rem2pos2.x, rem2pos2.y-(viewtri.height*2)};
		olc::vf2d rem4pos3 = rem3pos2;
		std::vector<olc::vf2d> rem4pos;
		rem4pos.push_back(rem4pos1);
		rem4pos.push_back(rem4pos2);
		rem4pos.push_back(rem4pos3);
		
		std::vector<olc::vf2d> rem4_textures = { {1,1},{0,1},{0.5,0.0}}; 
		DrawPolygonDecal(maskdecal, rem4pos, rem4_textures);

		olc::vf2d ret1pos1 = rem4pos1;
		olc::vf2d ret1pos2 = {rem4pos2.x, rem4pos2.y-(viewtri.height*2)};
		olc::vf2d ret1pos3 = rem4pos3;
		std::vector<olc::vf2d> ret1pos;
		ret1pos.push_back(ret1pos1);
		ret1pos.push_back(ret1pos2);
		ret1pos.push_back(ret1pos3);
		
		std::vector<olc::vf2d> ret1_textures = { {1,1},{0,1},{0.5,0.0}}; 
		DrawPolygonDecal(maskdecal, ret1pos, ret1_textures);

		olc::vf2d ret2pos1 = ttpos2;
		olc::vf2d ret2pos2 = ret1pos1;//{rem4pos2.x, rem4pos2.y-(viewtri.height*2)};
		olc::vf2d ret2pos3 = ret1pos2;
		std::vector<olc::vf2d> ret2pos;
		ret2pos.push_back(ret2pos1);
		ret2pos.push_back(ret2pos2);
		ret2pos.push_back(ret2pos3);
		
		std::vector<olc::vf2d> ret2_textures = { {0.5,0.0},{1,1},{0,1}}; 
		DrawPolygonDecal(maskdecal, ret2pos, ret2_textures);

		olc::vf2d leb1pos1 = {llpos3.x-(viewtri.sidelength), llpos3.y};
		olc::vf2d leb1pos2 = llpos1;//{rem4pos2.x, rem4pos2.y-(viewtri.height*2)};
		olc::vf2d leb1pos3 = llpos3;
		std::vector<olc::vf2d> leb1pos;
		leb1pos.push_back(leb1pos1);
		leb1pos.push_back(leb1pos2);
		leb1pos.push_back(leb1pos3);
		
		std::vector<olc::vf2d> leb1_textures = {{1,1},{0,1}, {0.5,0.0}}; 
		DrawPolygonDecal(maskdecal, leb1pos, leb1_textures);


		olc::vf2d lem1pos1 = leb1pos1; //{llpos3.x-(viewtri.sidelength), llpos3.y};
		olc::vf2d lem1pos2 = {leb1pos2.x, leb1pos2.y-(viewtri.height*2)};
		olc::vf2d lem1pos3 = leb1pos3;
		std::vector<olc::vf2d> lem1pos;
		lem1pos.push_back(lem1pos1);
		lem1pos.push_back(lem1pos2);
		lem1pos.push_back(lem1pos3);
		
		std::vector<olc::vf2d> lem1_textures = {{1,1},{0,1}, {0.5,0.0}}; 
		DrawPolygonDecal(maskdecal, lem1pos, lem1_textures);


		olc::vf2d lem2pos1 = lem1pos2; //{llpos3.x-(viewtri.sidelength), llpos3.y};
		olc::vf2d lem2pos2 = lem1pos3; 
		//{leb1pos2.x, leb1pos2.y-(viewtri.height*2)};
		olc::vf2d lem2pos3 = {lem1pos2.x+(viewtri.sidelength), lem1pos2.y};
		std::vector<olc::vf2d> lem2pos;
		lem2pos.push_back(lem2pos1);
		lem2pos.push_back(lem2pos2);
		lem2pos.push_back(lem2pos3);
		
		std::vector<olc::vf2d> lem2_textures = {{0,1},{0.5,0.0},{1,1}}; 
		DrawPolygonDecal(maskdecal, lem2pos, lem2_textures);



		olc::vf2d lem3pos1 = lem2pos1; //{llpos3.x-(viewtri.sidelength), llpos3.y};
		olc::vf2d lem3pos2 = {lem2pos2.x, lem2pos2.y-(viewtri.height*2)};
		olc::vf2d lem3pos3 = lem2pos3;//{lem1pos2.x+(viewtri.sidelength), lem1pos2.y};
		std::vector<olc::vf2d> lem3pos;
		lem3pos.push_back(lem3pos1);
		lem3pos.push_back(lem3pos2);
		lem3pos.push_back(lem3pos3);
		
		std::vector<olc::vf2d> lem3_textures = {{0,1},{0.5,0.0},{1,1}}; 
		DrawPolygonDecal(maskdecal, lem3pos, lem3_textures);


		olc::vf2d lem4pos1 = lem3pos2; //{llpos3.x-(viewtri.sidelength), llpos3.y};
		olc::vf2d lem4pos2 = lem3pos3; //{lem2pos2.x, lem2pos2.y-(viewtri.height*2)};
		olc::vf2d lem4pos3 = {lem3pos2.x+(viewtri.sidelength), lem3pos2.y};
		std::vector<olc::vf2d> lem4pos;
		lem4pos.push_back(lem4pos1);
		lem4pos.push_back(lem4pos2);
		lem4pos.push_back(lem4pos3);
		
		std::vector<olc::vf2d> lem4_textures = {{0.5,0.0},{1,1},{0,1}}; 
		DrawPolygonDecal(maskdecal, lem4pos, lem4_textures);


		olc::vf2d let1pos1 = lem4pos1; //{llpos3.x-(viewtri.sidelength), llpos3.y};
		olc::vf2d let1pos2 = {lem4pos2.x, lem4pos2.y-(viewtri.height*2)};
		olc::vf2d let1pos3 = lem4pos3;

		//{lem3pos2.x+(viewtri.sidelength), lem3pos2.y};
		std::vector<olc::vf2d> let1pos;
		let1pos.push_back(let1pos1);
		let1pos.push_back(let1pos2);
		let1pos.push_back(let1pos3);
		
		std::vector<olc::vf2d> let1_textures = {{0.5,0.0},{1,1},{0,1}}; 
		DrawPolygonDecal(maskdecal, let1pos, let1_textures);


		olc::vf2d let2pos1 = let1pos2; //{llpos3.x-(viewtri.sidelength), llpos3.y};
		olc::vf2d let2pos2 = let1pos3; //{lem4pos2.x, lem4pos2.y-(viewtri.height*2)};
		olc::vf2d let2pos3 = ttpos2;

		//{lem3pos2.x+(viewtri.sidelength), lem3pos2.y};
		std::vector<olc::vf2d> let2pos;
		let2pos.push_back(let2pos1);
		let2pos.push_back(let2pos2);
		let2pos.push_back(let2pos3);
		
		std::vector<olc::vf2d> let2_textures = {{1,1},{0,1}, {0.5,0.0}}; 
		DrawPolygonDecal(maskdecal, let2pos, let2_textures);


		//NOTE: Lower section
		olc::vf2d bel1pos1 = llpos1; //{llpos3.x-(viewtri.sidelength), llpos3.y};
		olc::vf2d bel1pos2 = {llpos3.x, llpos3.y+(viewtri.height*2)};
		olc::vf2d bel1pos3 = llpos2;

		//{lem3pos2.x+(viewtri.sidelength), lem3pos2.y};
		std::vector<olc::vf2d> bel1pos;
		bel1pos.push_back(bel1pos1);
		bel1pos.push_back(bel1pos2);
		bel1pos.push_back(bel1pos3);
		
		std::vector<olc::vf2d> bel1_textures = {{0,1},{0.5,0.0},{1,1}}; 
		DrawPolygonDecal(maskdecal, bel1pos, bel1_textures);




		olc::vf2d bel2pos1 = bel1pos2; //{llpos3.x-(viewtri.sidelength), llpos3.y};
		olc::vf2d bel2pos2 = bel1pos3; //{llpos3.x, llpos3.y+(viewtri.height*2)};
		olc::vf2d bel2pos3 = {bel1pos2.x+(viewtri.sidelength), bel1pos2.y};
		std::vector<olc::vf2d> bel2pos;
		bel2pos.push_back(bel2pos1);
		bel2pos.push_back(bel2pos2);
		bel2pos.push_back(bel2pos3);
		
		std::vector<olc::vf2d> bel2_textures = {{0.5,0.0},{1,1},{0,1}}; 
		DrawPolygonDecal(maskdecal, bel2pos, bel2_textures);


		olc::vf2d bel3pos1 = bel2pos2; //{llpos3.x-(viewtri.sidelength), llpos3.y};
		olc::vf2d bel3pos2 = bel2pos3; //{llpos3.x, llpos3.y+(viewtri.height*2)};
		olc::vf2d bel3pos3 = lbcpos3;
			//{bel1pos2.x+(viewtri.sidelength), bel1pos2.y};
		std::vector<olc::vf2d> bel3pos;
		bel3pos.push_back(bel3pos1);
		bel3pos.push_back(bel3pos2);
		bel3pos.push_back(bel3pos3);
		
		std::vector<olc::vf2d> bel3_textures = {{1,1},{0,1}, {0.5,0.0}}; 
		DrawPolygonDecal(maskdecal, bel3pos, bel3_textures);


		olc::vf2d becpos1 = bel3pos2; //{llpos3.x-(viewtri.sidelength), llpos3.y};
		olc::vf2d becpos2 = bel3pos3; //{llpos3.x, llpos3.y+(viewtri.height*2)};
		olc::vf2d becpos3 = {bel3pos2.x+(viewtri.sidelength), bel3pos2.y};
		std::vector<olc::vf2d> becpos;
		becpos.push_back(becpos1);
		becpos.push_back(becpos2);
		becpos.push_back(becpos3);
		
		std::vector<olc::vf2d> bec_textures = {{0,1},{0.5,0.0},{1,1}}; 
		DrawPolygonDecal(maskdecal, becpos, bec_textures);



		olc::vf2d ber3pos1 = becpos2; //{llpos3.x-(viewtri.sidelength), llpos3.y};
		olc::vf2d ber3pos2 = becpos3; //{llpos3.x, llpos3.y+(viewtri.height*2)};

		olc::vf2d ber3pos3 = rbcpos3;
		//{bel3pos2.x+(viewtri.sidelength), bel3pos2.y};
		std::vector<olc::vf2d> ber3pos;
		ber3pos.push_back(ber3pos1);
		ber3pos.push_back(ber3pos2);
		ber3pos.push_back(ber3pos3);
		
		std::vector<olc::vf2d> ber3_textures = {{0.5,0.0},{0,1},{1,1}}; 
		DrawPolygonDecal(maskdecal, ber3pos, ber3_textures);



		olc::vf2d ber2pos1 = ber3pos2; //{llpos3.x-(viewtri.sidelength), llpos3.y};
		olc::vf2d ber2pos2 = ber3pos3; //{llpos3.x, llpos3.y+(viewtri.height*2)};

		olc::vf2d ber2pos3 = {ber3pos2.x+(viewtri.sidelength), ber3pos2.y};
		std::vector<olc::vf2d> ber2pos;
		ber2pos.push_back(ber2pos1);
		ber2pos.push_back(ber2pos2);
		ber2pos.push_back(ber2pos3);
		
		std::vector<olc::vf2d> ber2_textures = {{1,1},{0,1}, {0.5,0.0}}; 
		DrawPolygonDecal(maskdecal, ber2pos, ber2_textures);


		olc::vf2d ber1pos1 = ber2pos2; //{llpos3.x-(viewtri.sidelength), llpos3.y};
		olc::vf2d ber1pos2 = ber2pos3; //{llpos3.x, llpos3.y+(viewtri.height*2)};

		olc::vf2d ber1pos3 = reb1pos2;
		//{ber3pos2.x+(viewtri.sidelength), ber3pos2.y};
		std::vector<olc::vf2d> ber1pos;
		ber1pos.push_back(ber1pos1);
		ber1pos.push_back(ber1pos2);
		ber1pos.push_back(ber1pos3);
		
		std::vector<olc::vf2d> ber1_textures = {{0,1},{0.5,0.0},{1,1}}; 
		DrawPolygonDecal(maskdecal, ber1pos, ber1_textures);



		//Create a fractal like decal
	// 	SetDrawTarget(nullptr);
	// 	maskdecal2->Update();
	// 	maskdecal2->UpdateSprite();
	//
	// 	Triangle sbigt;
	// 
	// 	//Leftcenterbollom
	// 	sbigt.sidelength = viewtri.sidelength * 4;
	// 	olc::vf2d bigtpos1 = llpos1;
	// 	sbigt.pos = bigtpos1;
	// 	//Make Equilateral: 
	// 	TriangleCoords(sbigt);
	// 	TriangleCenter(sbigt);
	//
	// 	olc::vf2d bigtpos2 = {ttpos2.x, ttpos2.y+(sbigt.height*2)};
	// 	olc::vf2d bigtpos3 = rrpos3;
	//
	// 	std::vector<olc::vf2d> bigtpos;
	// 	bigtpos.push_back(bigtpos1);
	// 	bigtpos.push_back(bigtpos2);
	// 	bigtpos.push_back(bigtpos3);
	//
	// 	std::vector<olc::vf2d> bigt_textures = {{1,1},{0.5,0.0},{0,1}}; 
	// 	DrawPolygonDecal(maskdecal2, bigtpos, bigt_textures);

		

		//SetPixelMode(olc::Pixel::NORMAL); // Draw all pixels
		// static void TexturedTriangle(int x1, int y1, float u1, float v1, float w1,
		// 	int x2, int y2, float u2, float v2, float w2,
		// 	int x3, int y3, float u3, float v3, float w3, olc::Sprite* spr);


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
