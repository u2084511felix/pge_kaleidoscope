#define OLC_GFX_OPENGL33
#define OLC_PGE_APPLICATION
#include "pge/olcPixelGameEngine.h"

#define OLC_PGEX_SHADERS
#include <pge/extensions/olcPGEX_Shaders.h>

#include <vector>
#include <algorithm>
#include <cmath> 


#include <fstream>

olc::EffectConfig loadEffect(const std::string& filename) {
	//get file
	std::ifstream file(filename);
	if(file.fail()) return olc::fx::FX_NORMAL;

	//dump contents into str stream
	std::stringstream mid;
	mid<<file.rdbuf();

	return {
		DEFAULT_VS,
		mid.str(),
		1,
		1
	};
}

#include <memory>

class CorrectShaderDemo : public olc::PixelGameEngine
{
public:
    CorrectShaderDemo()
    {
        sAppName = "olc::Shade Example";
    }

private:
    // The main PGEX object for handling shader operations
    olc::Shade shade;

    // An Effect object to hold the compiled shader (e.g., Grayscale)
    olc::Effect myGreyscaleEffect;




    std::unique_ptr<olc::Decal> sqdecal;
    olc::vf2d sqpos;
    olc::vf2d sqsize;


    // Decals to hold our source scene and the final processed output
    std::unique_ptr<olc::Decal> decalSource;
    std::unique_ptr<olc::Decal> decalTarget;

public:
    bool OnUserCreate() override
    {
        
        // 2. Create the decals we'll use for rendering. They need to match the screen size.
        decalSource = std::make_unique<olc::Decal>(new olc::Sprite(ScreenWidth(), ScreenHeight()));
        decalTarget = std::make_unique<olc::Decal>(new olc::Sprite(ScreenWidth(), ScreenHeight()));


        sqpos = {100, 100};
        sqsize = {12,12};
        sqdecal = std::make_unique<olc::Decal>(new olc::Sprite(ScreenWidth(), ScreenHeight()));
        SetDrawTarget(sqdecal->sprite);
        SetPixelMode(olc::Pixel::ALPHA);
        FillRect(sqpos, sqsize, olc::VERY_DARK_GREEN);
        SetDrawTarget(nullptr);
        sqdecal->Update();

        SetDrawTarget(decalSource->sprite);
        Clear(olc::BLANK);
        SetPixelMode(olc::Pixel::ALPHA);
        DrawString(10, 10, "Hello, olc::Shade!", olc::YELLOW, 2);
        FillCircle(ScreenWidth() / 2, ScreenHeight() / 2, 50, olc::RED);
        DrawLine(0, 0, ScreenWidth(), ScreenHeight(), olc::WHITE);
        SetDrawTarget(nullptr);

        decalSource->Update();

        // 3. Create a built-in Greyscale effect from the predefined configs in the header.
        myGreyscaleEffect = shade.MakeEffect(olc::fx::FX_NORMAL);

        // Check if the effect was created successfully
        if (!myGreyscaleEffect.IsOK())
        {
            // You can check myGreyscaleEffect.GetStatus() for any compile errors
            return false;
        }

        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override
    {
        // === STEP 1: Draw the original scene to the source decal ===
        // We set the draw target to our decal's sprite, so all drawing
        // commands go there instead of the screen.
        SetDrawTarget(nullptr);
        Clear(olc::BLACK);

        //SetPixelMode(olc::Pixel::ALPHA); 
        SetDecalMode(olc::DecalMode::ADDITIVE);



        decalSource->Update();
        sqdecal->Update();


        // === STEP 2: Use olc::Shade to apply the effect ===
        // Tell the shader system to render into our target decal
        shade.SetTargetDecal(decalSource.get());
        // Tell it to use our scene decal as the input texture
        shade.SetSourceDecal(sqdecal.get());

        // Start the effect. All drawing commands between Start() and End()
        // will be processed by the active shader effect.
        shade.Start(&myGreyscaleEffect);

        // We need to draw something to trigger the shader pipeline.
        // A full-screen quad is drawn with coordinates from -1 to +1.
        shade.DrawQuad({ -1.0f, -1.0f }, { 2.0f, 2.0f });
        
        // Stop the effect
        shade.End();


        // === STEP 3: Draw the final, processed decal to the screen ===
        // Clear(olc::BLACK);
        DrawDecal({ 0, 0 }, decalSource.get());

        return true;
    }
};

int main()
{
    CorrectShaderDemo demo;
    if (demo.Construct(512, 480, 2, 2, false, false))
        demo.Start();
    return 0;
}
