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
  static float rot[2], trans[2];

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClearDepth(1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glLoadIdentity();
  glTranslated(0.0, 0.0, -6.0);

  glBegin(GL_TRIANGLES);
    glVertex3d(0.0, 1.0, 0.0);
    glVertex3d(-1.0, -1.0, 0.0);
    glVertex3d(1.0, -1.0, 0.0);
  glEnd();
}

}  // namespace

int main() {
  demo::Init("3dtest", demo::kStreamerDemo);

  InitRendering();

  drc::InputData input_data;
  while (demo::HandleEvents()) {
    demo::TryPushingFrame();

    demo::GetInputReceiver()->Poll(input_data);
    RenderFrame(input_data);

    demo::SwapBuffers();
  }

  demo::Quit();
  return 0;
}
