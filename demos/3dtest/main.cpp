#include "../framework/framework.h"

#include <drc/input.h>
#include <drc/screen.h>
#include <GL/glu.h>

namespace {

void InitRendering() {
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  glViewport(0, 0, drc::kScreenWidth, drc::kScreenHeight);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45.0, drc::kScreenAspectRatio, 0.1, 100.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void RenderFrame(const drc::InputData& input_data) {
  static float rot[2], trans[2], bkgd;

  if (input_data.valid) {
    trans[0] += input_data.left_stick_x / 40.0;
    trans[1] += input_data.left_stick_y / 40.0;

    rot[0] += input_data.right_stick_x * 2.0;
    rot[1] += input_data.right_stick_y * 2.0;

    if (input_data.buttons & drc::InputData::kBtnA) {
      bkgd = 1.0;
    }
  }

  glClearColor(bkgd, bkgd, bkgd, 0.0);
  glClearDepth(1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glLoadIdentity();
  glTranslated(0.0, 0.0, -6.0);

  glTranslated(trans[0], trans[1], 0.0);
  glRotated(rot[0], 0.0, 1.0, 0.0);
  glRotated(rot[1], 1.0, 0.0, 0.0);

  // Draw a cube (from NeHe's tutorials...).
  glBegin(GL_QUADS);
    glColor3f(0.0f,1.0f,0.0f);
    glVertex3f( 1.0f, 1.0f,-1.0f);
    glVertex3f(-1.0f, 1.0f,-1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    glVertex3f( 1.0f, 1.0f, 1.0f);

    glColor3f(1.0f,0.5f,0.0f);
    glVertex3f( 1.0f,-1.0f, 1.0f);
    glVertex3f(-1.0f,-1.0f, 1.0f);
    glVertex3f(-1.0f,-1.0f,-1.0f);
    glVertex3f( 1.0f,-1.0f,-1.0f);

    glColor3f(1.0f,0.0f,0.0f);
    glVertex3f( 1.0f, 1.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    glVertex3f(-1.0f,-1.0f, 1.0f);
    glVertex3f( 1.0f,-1.0f, 1.0f);

    glColor3f(1.0f,1.0f,0.0f);
    glVertex3f( 1.0f,-1.0f,-1.0f);
    glVertex3f(-1.0f,-1.0f,-1.0f);
    glVertex3f(-1.0f, 1.0f,-1.0f);
    glVertex3f( 1.0f, 1.0f,-1.0f);

    glColor3f(0.0f,0.0f,1.0f);
    glVertex3f(-1.0f, 1.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f,-1.0f);
    glVertex3f(-1.0f,-1.0f,-1.0f);
    glVertex3f(-1.0f,-1.0f, 1.0f);

    glColor3f(1.0f,0.0f,1.0f);
    glVertex3f( 1.0f, 1.0f,-1.0f);
    glVertex3f( 1.0f, 1.0f, 1.0f);
    glVertex3f( 1.0f,-1.0f, 1.0f);
    glVertex3f( 1.0f,-1.0f,-1.0f);
  glEnd();

  if (bkgd > 0.0) {
    bkgd -= 0.02;
  }
}

}  // namespace

int main() {
  demo::Init("3dtest", demo::kStreamerGLDemo);

  InitRendering();

  drc::InputData input_data;
  while (demo::HandleEvents()) {
    demo::TryPushingGLFrame();

    demo::GetInputReceiver()->Poll(input_data);
    RenderFrame(input_data);

    demo::SwapBuffers();
  }

  demo::Quit();
  return 0;
}
