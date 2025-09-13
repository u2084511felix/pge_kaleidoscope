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

#include <opencv2/videoio.hpp>
#include <iostream>
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

	olc::Sprite* overlaysprite;
	olc::Decal* overlaydecal;

	olc::Sprite* newsp;
	olc::Decal* maskdecal;
	olc::vf2d cetrepos;

	olc::vf2d uvA = {0.0f, 1.0f};
	olc::vf2d uvB = {1.0f, 1.0f};
	olc::vf2d uvC = {0.5f, 0.0f};

	std::vector<std::vector<olc::vf2d>> uv_sets;
	Triangle baseTriangle;
	Triangle* bt = &baseTriangle;
	const int max_radius = 5; 

	//Camera utils:
	// OpenCV video capture object.
	olc::Sprite* pCameraFrame = nullptr;
	olc::Decal* pCameraDecal = nullptr;
	cv::VideoCapture cap;
	int nCamWidth = 640;
	int nCamHeight = 480;


public:

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

		if (!setup_cam()) return false;

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

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{

		Clear(olc::BLANK);	
		SetPixelMode(olc::Pixel::ALPHA);

		SetDrawTarget(newsp);
		Clear(olc::BLANK);
		SetPixelMode(olc::Pixel::Mode::ALPHA);

		bool render;
		render = RenderCameraFrame();
		
		if (render){
			return 1;
		}
		DrawSprite({0, 0}, pCameraFrame);

		SetDrawTarget(nullptr);
		Clear(olc::BLANK);

		maskdecal->Update();

		for (Triangle &t: triangles) {
			DrawPolygonDecal(maskdecal, t.coords, t.texture);
			DrawTriangle(t.x, t.y, t.z, olc::VERY_DARK_RED);
		};

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
			std::string filename = "./Screenshots/camera_kaleidoscope_";
			filename += dateandtime;
			filename += ".png";
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
		demo.Start();
	return 0;
}
