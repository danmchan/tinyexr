#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <float.h>
#include <string.h>
#include <string.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "../../tinyexr.h"
#include "trackball.h"

static int mouse_x, mouse_y;
static int mouse_m_pressed;
static int mouse_r_pressed;
static int mouse_moving;
static int width = 512, height = 512;
static float view_org[3], view_tgt[3];
static float curr_quat[4], prev_quat[4];

DeepImage gDeepImage;

//
// --
//

static void
reshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (float)w / (float)h, 0.1f, 1000.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    width = w; height = h;
}

static void
draw_samples()
{
  glPointSize(1.0f);
  glColor3f(1.0f, 1.0f, 1.0f);

  glBegin(GL_POINTS);

  // find depth channel.
  int depthChan = 0;
  for (int c = 0; c < gDeepImage.num_channels; c++) {
    if (strncmp("Z", gDeepImage.channel_names[c], 1) == 0) {
      depthChan = c;
    }
  }

  for (int y = 0; y < gDeepImage.height; y++) {
    float py = 2.0f * (y / (float)gDeepImage.height) - 1.0f;
    int sampleNum = gDeepImage.offset_table[y][gDeepImage.width-1];
    for (int x = 0; x < gDeepImage.width-1; x++) {
      float px = 2.0f * (x / (float)gDeepImage.width) - 1.0f;
      int s = gDeepImage.offset_table[y][x+1];
      if (s >= sampleNum) {
        continue;
      }
      float pz = gDeepImage.image[depthChan][y][s];
      glVertex3f(px, py, pz);
    }
  }

  glEnd();
}

static void
display()
{
    GLfloat mat[4][4];

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // camera & rotate
    gluLookAt(view_org[0], view_org[1], view_org[2],
          view_tgt[0], view_tgt[1], view_tgt[2],
          0.0, 1.0, 0.0);

    build_rotmatrix(mat, curr_quat);
    glMultMatrixf(&mat[0][0]);

    draw_samples();

    //glBegin(GL_POLYGON);
    //    glTexCoord2f(0 , 0); glVertex2f(-0.9 , -0.9);
    //    glTexCoord2f(0 , 1); glVertex2f(-0.9 , 0.9);
    //    glTexCoord2f(1 , 1); glVertex2f(0.9 , 0.9);
    //    glTexCoord2f(1 , 0); glVertex2f(0.9 , -0.9);
    //glEnd();

    glutSwapBuffers();
}

static void
keyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 'q':
    case 27:
        exit(0);
        break; 
    default:
        break; 
    }

    glutPostRedisplay();
}

static void
mouse(int button, int state, int x, int y)
{
    int mod = glutGetModifiers();

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (mod == GLUT_ACTIVE_SHIFT) {
            mouse_m_pressed = 1;
        } else if (mod == GLUT_ACTIVE_CTRL) {
            mouse_r_pressed = 1;
        } else {
            trackball(prev_quat, 0, 0, 0, 0);
        }
        mouse_moving = 1;
        mouse_x = x;
        mouse_y = y;
    } else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        mouse_m_pressed = 0;
        mouse_r_pressed = 0;
        mouse_moving = 0;
    }
 
}
    
static void
motion(int x, int y)
{
    float w = 1.0;
    float mw = 0.1;

    if (mouse_moving) {
        if (mouse_r_pressed) {
            view_org[2] += mw * (mouse_y - y);
            view_tgt[2] += mw * (mouse_y - y);
        } else if (mouse_m_pressed) {
            view_org[0] += mw * (mouse_x - x);
            view_org[1] -= mw * (mouse_y - y);
            view_tgt[0] += mw * (mouse_x - x);
            view_tgt[1] -= mw * (mouse_y - y);
        } else {
            trackball(prev_quat,
                  w * (2.0 * mouse_x - width)  / width,
                  w * (height - 2.0 * mouse_y) / height,
                  w * (2.0 * x - width)  / width,
                  w * (height - 2.0 * y) / height);
            add_quats(prev_quat, curr_quat, curr_quat);
        }

        mouse_x = x;
        mouse_y = y;
        
    }

    glutPostRedisplay();
}

static void
init()
{
    trackball(curr_quat, 0, 0, 0, 0);

    view_org[0] = 0.0f;
    view_org[1] = 0.0f;
    view_org[2] = 3.0f;

    view_tgt[0] = 0.0f;
    view_tgt[1] = 0.0f;
    view_tgt[2] = 0.0f;
}

int
main(
    int argc,
    char **argv)
{
    const char *input = "input.exr";

    if (argc < 2) {
        printf("Usage: deepview <input.exr>\n");
        exit(1);
    }

    input = argv[1];
    const char* err;
    int ret = LoadDeepEXR(&gDeepImage, input, &err);
    if (ret != 0) {
      if (err) {
        fprintf(stderr, "ERR: %s\n", err);
      }
      exit(-1);
    }

    glutInit(&argc, argv);
    glutInitWindowSize(512, 512);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);

    init();

    glutCreateWindow("deepimage viewer");

    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutMainLoop();

    return 0;
}
