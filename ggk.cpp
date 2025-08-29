#define OLC_PGE_APPLICATION
#include "pge/olcPixelGameEngine.h"

#include <vector>
#include <cmath>

// A simple struct to define a 2D circle
struct Circle {
    olc::vf2d center;
    float radius;
    olc::Pixel color;
};

class IsoscelesKaleidoscope : public olc::PixelGameEngine {
public:
    IsoscelesKaleidoscope() {
        sAppName = "Isosceles Kaleidoscope with DrawPartialRotatedDecal";
    }

private:
    olc::vf2d m_center;

    const double PI = 3.14159265358979323846;
    float m_baseAngle = PI / 3.0f; // 60 degrees for equilateral.
    float m_viewport_height;
    float m_viewport_half_base;

    olc::Sprite* m_viewport_sprite = nullptr;
    olc::Decal* m_viewport_decal = nullptr;
    std::vector<Circle> m_circles;

    void draw_scene_to_viewport() {
        SetDrawTarget(m_viewport_sprite);
        Clear(olc::BLACK);

        for (const auto& c : m_circles) {
            FillCircle(c.center, c.radius, c.color);
        }

        SetDrawTarget(nullptr);
    }

    // Reflects a point `p` across a line. The line is defined by a point `a` and its normal `n`.
    olc::vf2d reflect_across_line(const olc::vf2d& p, const olc::vf2d& a, const olc::vf2d& n) {
        olc::vf2d v = p - a;
        return p - 2.0f * (v.dot(n)) * n;
    }

    void draw_reflections(olc::vf2d pos, float angle, bool flip_x, int depth) {
        if (depth > 6) {
            return;
        }

        olc::vf2d decal_scale = { flip_x ? -1.0f : 1.0f, 1.0f };
        DrawPartialRotatedDecal(pos, m_viewport_decal, angle, m_center, {0,0}, {(float)ScreenWidth(), (float)ScreenHeight()}, decal_scale);

        // Calculate and draw the reflections recursively
        // Reflect across the left mirror
        draw_reflections(pos, angle + (m_baseAngle * 2), flip_x, depth + 1);

        // Reflect across the right mirror
        draw_reflections(pos, angle - (m_baseAngle * 2), flip_x, depth + 1);
        
        // Reflect across the base mirror
        // Note: Reflection across the base is a rotation by PI and a horizontal flip
        draw_reflections(pos, angle + PI, !flip_x, depth + 1);
    }
    
public:
    bool OnUserCreate() override {
        m_center = { (float)ScreenWidth() / 2.0f, (float)ScreenHeight() / 2.0f };
        m_viewport_height = (float)ScreenHeight() * 0.75f;
        m_viewport_half_base = m_viewport_height / std::tan(m_baseAngle);

        m_viewport_sprite = new olc::Sprite(ScreenWidth(), ScreenHeight());
        m_viewport_decal = new olc::Decal(m_viewport_sprite);

        m_circles.push_back({m_center + olc::vf2d{0.0f, -50.0f}, 30.0f, olc::RED});
        m_circles.push_back({m_center + olc::vf2d{20.0f, 20.0f}, 25.0f, olc::GREEN});
        m_circles.push_back({m_center + olc::vf2d{-30.0f, 30.0f}, 40.0f, olc::BLUE});
        m_circles.push_back({m_center + olc::vf2d{0.0f, 0.0f}, 15.0f, olc::YELLOW});

        draw_scene_to_viewport();
        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override {
        draw_scene_to_viewport();
        Clear(olc::BLACK);
        
        // Use a recursive function to draw reflections, starting from the base viewport
        // draw_reflections(m_center, 0.0f, false, 0);
        //
        // // Draw the central viewport outline
        // olc::vf2d p1 = m_center + olc::vf2d{ 0.0f, -m_viewport_height / 2.0f };
        // olc::vf2d p2 = m_center + olc::vf2d{ -m_viewport_half_base, m_viewport_height / 2.0f };
        // olc::vf2d p3 = m_center + olc::vf2d{ m_viewport_half_base, m_viewport_height / 2.0f };
        // DrawTriangle(p1, p2, p3, olc::GREEN);
        
        return true;
    }
    

};

int main() {
    IsoscelesKaleidoscope demo;
    if (demo.Construct(512, 512, 1, 1)) {
        demo.Start();
    }
    return 0;
}

