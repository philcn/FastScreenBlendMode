#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class ScreenBlendModeApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
};

void ScreenBlendModeApp::setup()
{
}

void ScreenBlendModeApp::mouseDown( MouseEvent event )
{
}

void ScreenBlendModeApp::update()
{
}

void ScreenBlendModeApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( ScreenBlendModeApp, RendererGl )
