#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "CinderImGui.h"

using namespace ci;
using namespace ci::app;
using namespace std;

// Based on http://wiki.nuaj.net/index.php?title=Lens_Flares
// Photoshop "Screen" blend mode is implemented as Dst' = 1 - (1 - Src) * (1 - Dst).
// We can't directly express this function in OpenGL, but we can implement
// Dst' = (1 - Src) * (1 - Dst), with an additional invert color after the last pass.
// We use a shader that outputs (1 - Src) instead of Src and a blend func of
// Dst' = Src * (1 - Dst) to achieve that.

#define BLEND_LAYER_COUNT 4

struct BlendMode {
	enum Type : int { Alpha, Additive, Screen, Count };
};

static const char *sBlendModeNames[] = { "Alpha", "Additive", "Screen" };

class ScreenBlendModeApp : public App {
  public:
	ScreenBlendModeApp();
	
	void setup() override;
	void update() override;
	void draw() override;
	
	void drawTexture(const gl::Texture2dRef &tex, bool screenBlendMode);
	void invertBackBufferColor();
	
	gl::Texture2dRef mBackgroundTex;
	gl::Texture2dRef mFullscreenOverlayTex;
	std::vector<gl::Texture2dRef> mBlendLayers;
	
	gl::GlslProgRef mPassthroughShader;
	gl::GlslProgRef mScreenBlendShader;
	gl::BatchRef mPassthroughBatch;
	gl::BatchRef mScreenBlendBatch;
	
	int mNumBlendLayers;
	BlendMode::Type mBlendMode;
	float mAlphaOverride;
};

ScreenBlendModeApp::ScreenBlendModeApp() :
	mNumBlendLayers(BLEND_LAYER_COUNT),
	mBlendMode(BlendMode::Screen),
	mAlphaOverride(1.0f)
{
}

void ScreenBlendModeApp::setup()
{
	mBackgroundTex = gl::Texture::create(loadImage(loadAsset("textures/Background.png")));
	mFullscreenOverlayTex = gl::Texture::create(loadImage(loadAsset("textures/Dirty.png")));
	for (int i = 0; i < BLEND_LAYER_COUNT; ++i) {
		mBlendLayers.emplace_back(gl::Texture::create(loadImage(loadAsset("textures/Layer" + to_string(i) + ".png"))));
	}
	
	try {
		mScreenBlendShader = gl::GlslProg::create(loadAsset("shaders/passthrough.vert"), loadAsset("shaders/screen.frag"));
	} catch (gl::GlslProgExc &e) {
		console() << e.what() << std::endl;
		exit(1);
	}
	
	mScreenBlendShader->uniform("uInputTexture", 0);
	mScreenBlendBatch = gl::Batch::create(geom::Rect(), mScreenBlendShader);
	
	mPassthroughShader = gl::getStockShader(gl::ShaderDef().texture().color());
	mPassthroughBatch = gl::Batch::create(geom::Rect(), mPassthroughShader);
	
	auto uiOptions = ui::Options().darkTheme().color(ImGuiCol_WindowBg, ColorA(0.078, 0.078, 0.078, 1.0)).alpha(0.95).font(getAssetPath("segoeui.ttf"), toPixels(16.0)).framePadding(toPixels(vec2(4, 2))).frameRounding(toPixels(3)).itemSpacing(toPixels(vec2(6, 1))).itemInnerSpacing(toPixels(vec2(2, 4))).scrollBarSize(toPixels(9)).grabMinSize(toPixels(4)).grabRounding(toPixels(1));
	ui::initialize(uiOptions);
}

void ScreenBlendModeApp::update()
{
	ui::Combo("BlendMode", (int *)&mBlendMode, sBlendModeNames, BlendMode::Count);
	ui::SliderInt("Draw Layers", &mNumBlendLayers, 0, BLEND_LAYER_COUNT);
	ui::SliderFloat("Alpha Override", &mAlphaOverride, 0.0f, 1.0f);
}

void ScreenBlendModeApp::drawTexture(const gl::Texture2dRef &tex, bool screenBlendMode)
{
	gl::ScopedModelMatrix scopedModelMatrix;
	
	// Calculate a rect that fills the screen with image respecting aspect ratio
	Rectf filledRect = Rectf(tex->getBounds()).getCenteredFill(getWindowBounds(), true);
	gl::translate(getWindowSize() / 2);
	gl::scale(filledRect.getSize());

	gl::ScopedTextureBind scopedTex(tex, 0);
	
	if (screenBlendMode) {
		// dstColor = srcColor * dstColor, dstAlpha = srcAlpha * dstAlpha
		gl::ScopedBlend screenBlend(GL_DST_COLOR, GL_ZERO, GL_DST_ALPHA, GL_ZERO);
		
		// because the shader outputs complement color (1 - srcColor), the blend func effectly becomes
		// dstColor = (1 - srcColor) * dstColor
		mScreenBlendBatch->draw();
	} else {
		mPassthroughBatch->draw();
	}
}

void ScreenBlendModeApp::invertBackBufferColor()
{
	gl::ScopedColor scopedColor(ColorAf(1.0, 1.0, 1.0, 1.0));
	gl::ScopedBlend screenBlend(GL_ONE_MINUS_DST_COLOR, GL_ZERO, GL_DST_ALPHA, GL_ZERO);

	gl::drawSolidRect(getWindowBounds());
}

void ScreenBlendModeApp::draw()
{
	gl::ScopedMatrices scopedMatrix;
	gl::setMatricesWindow(getWindowSize());
	
	gl::clear(Color(0, 0, 0 ));
	
	// Draw initial content without blend mode
	drawTexture(mBackgroundTex, false);
	
	// Pass alpha value to shader for overriding
	gl::ScopedColor scopedColor(ColorAf(1.0, 1.0, 1.0, mAlphaOverride));
	
	if (mBlendMode == BlendMode::Screen) {
		// The first screen-blend draw call needs (1 - dstColor) as the dstColor in blend equation
		invertBackBufferColor();
		
		// Draw images with screen blend mode
		drawTexture(mFullscreenOverlayTex, true);
		
		// Draw additional layers
		for (int i = 0; i < mNumBlendLayers; ++i) {
			drawTexture(mBlendLayers[i], true);
		}
		
		// After all screen-blend passes, invert the back buffer again
		invertBackBufferColor();
	} else if (mBlendMode == BlendMode::Alpha) {
		gl::ScopedBlendAlpha scopedBlendAlpha;
		
		drawTexture(mFullscreenOverlayTex, false);
		
		for (int i = 0; i < mNumBlendLayers; ++i) {
			drawTexture(mBlendLayers[i], false);
		}
	} else if (mBlendMode == BlendMode::Additive) {
		gl::ScopedBlendAdditive scopedBlendAdditive;
		
		drawTexture(mFullscreenOverlayTex, false);
		
		for (int i = 0; i < mNumBlendLayers; ++i) {
			drawTexture(mBlendLayers[i], false);
		}
	}
}

CINDER_APP(ScreenBlendModeApp, RendererGl, [] (App::Settings *settings) {
	settings->setWindowSize(800, 400);
	settings->setHighDensityDisplayEnabled();
})
