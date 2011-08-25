#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class GesichtertauschApp : public AppBasic {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
};

void GesichtertauschApp::setup()
{
}

void GesichtertauschApp::mouseDown( MouseEvent event )
{
}

void GesichtertauschApp::update()
{
}

void GesichtertauschApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 
}


CINDER_APP_BASIC( GesichtertauschApp, RendererGl )
