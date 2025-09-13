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

#define OLC_GFX_OPENGL33
#define OLC_PGE_APPLICATION
#include "pge/olcPixelGameEngine.h"

#define OLC_PGEX_TRANSFORMEDVIEW
#include "pge/extensions/olcPGEX_TransformedView.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cmath> 


struct Triangle {
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

class Kaleidoscope : public olc::PixelGameEngine
{
public:
	Kaleidoscope()
	{
		sAppName = "Kaleidoscope";
	}
	std::vector<Triangle> triangles;

	std::vector<ImageSprites> imgsprites = {};
	int target_sprite_index = 0;

	olc::vf2d panrange = {0,0};
	float scrollspeed = 3.0f;
	float totaldistance = 0.0f;
	olc::vf2d worldoffset = {0,0};

	olc::TransformedView tv;
	olc::Sprite* overlaysprite;
	olc::Decal* overlaydecal;
	std::vector<olc::Sprite*> imagesprites;


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

public:

	//NOTE: ...........................
	//.................................
	//    ProcGen / Randomizer utils: 
	//    from: https://github.com/OneLoneCoder/Javidx9/blob/master/PixelGameEngine/SmallerProjects/OneLoneCoder_PGE_ProcGen_Universe.cpp
	//.................................
	//.................................

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

	int hex_distance(olc::vi2d h) {
	    return (std::abs(h.x) + std::abs(h.y) + std::abs(-h.x - h.y)) / 2;
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

	bool OnUserCreate() override
	{
		//SetPixelBlend(1.0);

		tv.Initialise({ ScreenWidth(), ScreenHeight() });

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

		panrange = {(float)ScreenWidth()/2, (float)ScreenWidth()/2};
		tv.StartPan(panrange);
		worldoffset = tv.GetWorldOffset();
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{

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


		if (target_sprite == 1) {
			tv.DrawSprite({ 0,0 }, imgsprites[target_sprite_index].m_pImage);
		}
		if (target_sprite == 2) {
			tv.DrawSprite({ 0,0 }, imgsprites[target_sprite_index].m_pDithered);
		}
		if (target_sprite == 3) {
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
			DrawStringDecal({10, 100}, "Press NumPad1 to draw space telescope images", olc::WHITE, {2.0f, 2.0f});	
			DrawStringDecal({10, 130}, "into the kaleidoscope viewport", olc::WHITE, {2.0f, 2.0f});
			DrawStringDecal({10, 160}, "Press NumPad2 to dithered draw space telescope images", olc::WHITE, {2.0f, 2.0f});	
			DrawStringDecal({10, 190}, "into the kaleidoscope viewport", olc::WHITE, {2.0f, 2.0f});
			DrawStringDecal({10, 220}, "Press NumPad3 to dithered draw quantized space telescope images", olc::WHITE, {2.0f, 2.0f});	
			DrawStringDecal({10, 250}, "into the kaleidoscope viewport", olc::WHITE, {2.0f, 2.0f});

		}

		overlaydecal->UpdateSprite();

		if(GetKey(olc::Key::INS).bPressed) {
			std::chrono::time_point<std::chrono::system_clock> tp1;
			tp1 = std::chrono::system_clock::now();
			time_t now;
			time(&now);
			std::string dateandtime = ctime(&now);
			std::replace( dateandtime.begin(), dateandtime.end(), ':', '_');
			std::replace( dateandtime.begin(), dateandtime.end(), ' ', '_');
			std::replace( dateandtime.begin(), dateandtime.end(), '$', '_');
			std::replace( dateandtime.begin(), dateandtime.end(), '\n', '_');
			std::string filename = "./Screenshots/image_kaleidoscope_";
			filename += dateandtime;
			filename += ".png";
			SaveSprite(overlaysprite, filename);
		}

 		return true;
	}

	bool OnUserDestroy() override
	{
		delete overlaydecal;
		delete maskdecal;

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
