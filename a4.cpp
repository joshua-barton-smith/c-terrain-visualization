#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>
#endif

#include "mathLib3D.h"
#include "camera.h"
#include "light.h"
#include "material.h"
#include "PPM.h"
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>

// the two Vec3D represent the eye position and the lookAt position
Camera camera = Camera(Vec3D(-5.0, 1.0, 41.0), Vec3D(-5.0, 1.0, -5.0));

// width and height of the screen.
int screen_width = 600;
int screen_height = 600;

// movement inputs
bool movement[] = {false, false, false, false};

// size of the grid x, z
int x_size;
int z_size;

// the height array
float **heightmap;
float **currentheight;

// rendering mode
int render_mode = 0;

// texturing mode
int texture_mode = 0;

// shading mode
bool shading = true;

// lighting mode
bool lighting = true;

// mesh mode
bool mesh = false;

// light objects
Light l, l1;

// maximum height
float max_height = 1;

// normal vectors for each vertex
Vec3D **normals;

// forward declaration bc the function dependencies are a little messy
void init_terrain();

// instructions
const char *instructions =  "Move the camera with W/S/A/D and mouse.\n"
                            "Swap rendering mode (filled polys, wires, or both) with the F key.\n"
                            "Enable and disable Gouraud shading with the H key.\n"
                            "Enable and disable lighting with the L key.\n"
                            "Generate new terrain with the R key.\n"
                            "Swap between terrain textures with the T key.\n"
                            "Swap between a quad or a triangle mesh with the M key.";

// this code is directly from lab 6.
struct Image {
    int mWidth;
    int mHeight;
    GLubyte * mImage;

    void load(char * filename) {
        mImage = LoadPPM(filename, &mWidth, &mHeight);
    }

    void draw(unsigned int x, unsigned int y) {
        glRasterPos2i(x + mWidth, y);
        /**
         * If you are on a retina display then you should replace the values
         * from -1, 1 to -2, 2 to ensure they appear at full size!
         *
         * The PPM parser packs the image mirrored horizontally. Thus we use
         * glPixelZoom to "flip" the image back the correct way by scaling the
         * x axis by -1. This is the same concept as mirroring an object with
         * glScalef(-1, 1, 1).
         *
         * Aside: Using the parsing code from last year. I don't think the code
         * is very good. There is definitely a way to parse PPM bitmaps without
         * needing to flip the image like this.
         */
        glPixelZoom(-1, 1);
        glDrawPixels(mWidth, mHeight, GL_RGB, GL_UNSIGNED_BYTE, mImage);
    }

    void texture() {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mWidth, mHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, mImage);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
};

// 4x textures to use on the terrain
Image marble;
Image teapot;
Image aerial;
Image baboon;


/**
* Handles regular keyboard inputs (e.g. w/s/a/d for movement)
*/
void handleKeyboard(unsigned char key, int _x, int _y)
{
    switch (key) {
        // movement keys...
        case 'w': {
            movement[CAMERA_MOVE_FORWARD] = true;
            break;
        }
        case 's': {
            movement[CAMERA_MOVE_BACKWARD] = true;
            break;
        }
        case 'a': {
            movement[CAMERA_STRAFE_LEFT] = true;
            break;
        }
        case 'd': {
            movement[CAMERA_STRAFE_RIGHT] = true;
            break;
        }
        // swap rendering mode (solid, wires, both)
        case 'f': {
            render_mode++;
            render_mode = render_mode % 3;
            break;
        }
        // swap shading (flat, smooth)
        case 'h': {
            shading = !shading;
            if (shading) glShadeModel(GL_SMOOTH);
            else glShadeModel(GL_FLAT);
            break;
        }
        // enable or disable lighting
        case 'l': {
            lighting = !lighting;
            if (lighting) glEnable(GL_LIGHTING);
            else glDisable(GL_LIGHTING);
            break;
        }
        // reset terrain to regenerate
        case 'r': {
            init_terrain();
            break;
        }
        // swap texturing mode (none, or one of textures 1, 2, 3, 4)
        case 't': {
            texture_mode++;
            texture_mode = texture_mode % 5;
            if (texture_mode == 0) glDisable(GL_TEXTURE_2D);
            else glEnable(GL_TEXTURE_2D);
            break;
        }
        // swap mesh mode
        case 'm': {
            mesh = !mesh;
            break;
        }
        // quit
        case 'q': {
            exit(0);
            break;
        }
    }
}

/**
* Handles keyboard key releases
*/
void handleKeyboardUp(unsigned char key, int _x, int _y)
{
    switch(key) {
        // movement keys released
        case 'w': {
            movement[CAMERA_MOVE_FORWARD] = false;
            break;
        }
        case 's': {
            movement[CAMERA_MOVE_BACKWARD] = false;
            break;
        }
        case 'a': {
            movement[CAMERA_STRAFE_LEFT] = false;
            break;
        }
        case 'd': {
            movement[CAMERA_STRAFE_RIGHT] = false;
            break;
        }
    }
}

/**
* Handles special key inputs (i.e. arrow keys).
*/
void special(int key, int x, int y) {
    switch(key) {
    }
}

/**
* Handles special keys (i.e. arrow keys) being released
*/
void specialUp(int key, int x, int y) {
    switch(key) {
    }
}

/**
* Renders a 2D HUD over the top of the 3D scene.
*/
void drawHUD() {
    // disable lighting since 2d hud should just be colored
    if(lighting) glDisable(GL_LIGHTING);
    // disable texturing if enabled
    if(texture_mode > 0) glDisable(GL_TEXTURE_2D);

    // reset matrices so the 3d view is not affecting
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // setup 2d ortho perspective
    glOrtho(-1, 1, -1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    // build a string to display
    std::stringstream stream;
    stream << "(" << camera.camPos.mX << "," << camera.camPos.mY << "," << camera.camPos.mZ << ")" << std::endl;
    stream << "angles: " << camera.pitch << "," << camera.yaw << std::endl;
    if (render_mode == 0) stream << "Filled Rendering" << std::endl;
    else if (render_mode == 1) stream << "Wire Rendering" << std::endl;
    else  stream << "Doubled Rendering" << std::endl;
    if (shading) stream << "Gouraud shading" << std::endl;
    if (lighting) stream << "Lighting enabled" << std::endl;

    if (texture_mode == 0) stream << "No Textures" << std::endl;
    else stream << "Texture " << texture_mode << std::endl;

    if (mesh) stream << "Triangle Mode" << std::endl;
    else stream << "Quads Mode" << std::endl;
    std::string output = stream.str();

    // color and position
    glColor4f(1.0, 0.0, 0.0, 1.0);
    glRasterPos2f(-1, 0.9);
    // write string to screen
    glutBitmapString(GLUT_BITMAP_HELVETICA_18, reinterpret_cast<const unsigned char*>(output.c_str()));

    // part 2 of hud: we want to draw a minimap as a bonus feature
    // 1. draw a gray quad to represent the region the minimap will occupy
    glColor4f(0.6, 0.6, 0.6, 1.0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_QUADS);
        glVertex3f(0.35, 0.35, 0.0);
        glVertex3f(0.95, 0.35, 0.0);
        glVertex3f(0.95, 0.95, 0.0);
        glVertex3f(0.35, 0.95, 0.0);
    glEnd();

    // 2. draw the terrain overview between 0.4 and 0.9 (leaving some of the grey quad visible as a border)
    // use the x,z coords as x,y in 2d space and color according to the y coord of 3d space
    for (int i = 0; i < x_size; i++) {
        for (int j = 0; j < z_size; j++) {
            float y = currentheight[i][j];
            float green_comp = 1 - (2 * (y / max_height));
            if (green_comp < 0) green_comp = 0;
            float red_comp = 1 - green_comp;
            glColor4f(red_comp, green_comp, 0, 0.8);
            // convert (i,j) (which are coords from (0, axis_size)) into coords in (0.4, 0.9)
            float i_p = ((float) i / (float) x_size) * 0.5;
            float j_p = ((float) j / (float) z_size) * 0.5;
            glBegin(GL_POINTS);
                glVertex3f(i_p + 0.4, j_p + 0.4, 0.5);
            glEnd();
        }
    }

    // 3. draw a blue cross for the location of the camera, if it is on the grid
    float px = camera.camPos.mX;
    float pz = camera.camPos.mZ;

    if (px >= 0 && px <= x_size && pz >= 0 && pz <= z_size) {
        // translate to coords in (0.4, 0.9)
        float px_p = ((float) px / (float) x_size) * 0.5;
        float pz_p = ((float) pz / (float) z_size) * 0.5;

        // render a small cross
        glColor4f(0.0, 0.0, 1.0, 1.0);
        glBegin(GL_LINES);
            glVertex3f(px_p + 0.4 - 0.01, pz_p + 0.4 - 0.01, 1.0);
            glVertex3f(px_p + 0.4 + 0.01, pz_p + 0.4 + 0.01, 1.0);
            glVertex3f(px_p + 0.4 - 0.01, pz_p + 0.4 + 0.01, 1.0);
            glVertex3f(px_p + 0.4 + 0.01, pz_p + 0.4 - 0.01, 1.0);
        glEnd();
    }

    if(lighting) glEnable(GL_LIGHTING);
    if(texture_mode > 0) glEnable(GL_TEXTURE_2D);
}

/**
* Keeps track of mouse motion and updates pitch/yaw accordingly.
*/
void motion(int x, int y)
{
    // compute how far the mouse has moved relative to the center
    float xoff = x - ((float)screen_width / 2);
    float yoff = y - ((float)screen_height / 2);

    // update the camera rotation based on the mouse movement
    camera.updateRotation(xoff, yoff);

    // move mouse cursor back to center
    glutWarpPointer(((int)screen_width / 2), ((int)screen_height / 2));
}

/**
* Binds a material for a vertex based on its height, as a ratio of the max height.
*/
void bindTopographicMaterial(float y) {
    // compute green/red components of the material
    float green_comp = 1 - (2 * (y / max_height));
    if (green_comp < 0) green_comp = 0;
    float red_comp = 1 - green_comp;
    // if lighting is enabled bind a material
    if (lighting) {
        float amb[4] = {((float)0.3 * red_comp), ((float)0.3 * green_comp), 0.0, 1.0};
        float diff[4] = {((float)0.6 * red_comp), ((float)0.6 * green_comp), 0.0, 1.0};
        float spec[4] = {((float)1.0 * red_comp), ((float)1.0 * green_comp), 0.0, 1.0};
        float shin = 100;
        Material(amb, diff, spec, shin).bind();
    } else {
        // otherwise just use glColor
        glColor3f(red_comp, green_comp, 0.0);
    }
}

// fixes y-axis of a cross product, since all our vertex normals are pointing up.
Vec3D yfix(Vec3D in) {
    if (in.mY < 0) return Vec3D(in.mX, -in.mY, in.mZ).normalize();
    return in.normalize();
}

// binds a normal via gl function calls
void bindNormals(int x, int z) {
    glNormal3f(normals[x][z].mX, normals[x][z].mY, normals[x][z].mZ);
}

/**
* Draws the terrain from the mesh in currentheight.
*/
void drawTerrain(bool shouldUseWire) {
    if (!shouldUseWire) {
        // this should be replaced
        switch (texture_mode) {
            case 0: {
                glBindTexture(GL_TEXTURE_2D, 0);
                break;
            }
            case 1: {
                marble.texture();
                break;
            }
            case 2: {
                aerial.texture();
                break;
            }
            case 3: {
                teapot.texture();
                break;
            }
            case 4: {
                baboon.texture();
                break;
            }
        }

        for (int x = 0; x < (x_size-1); x++) {
            for (int z = 0; z < (z_size-1); z++) {
                // for each vertex, render it and bind a material for it
                if(!mesh){
                    glBegin(GL_QUADS);
                        bindTopographicMaterial(currentheight[0+x][1+z]);
                        glTexCoord2f(0, 0);
                        bindNormals(x, z+1);
                        glVertex3f(0+x, currentheight[0+x][1+z], 1+z);

                        bindTopographicMaterial(currentheight[1+x][1+z]);
                        glTexCoord2f(1, 0);
                        bindNormals(x+1, z+1);
                        glVertex3f(1+x, currentheight[1+x][1+z], 1+z);

                        bindTopographicMaterial(currentheight[1+x][0+z]);
                        glTexCoord2f(1, 1);
                        bindNormals(x+1, z);
                        glVertex3f(1+x, currentheight[1+x][0+z], 0+z);

                        bindTopographicMaterial(currentheight[0+x][0+z]);
                        glTexCoord2f(0, 1);
                        bindNormals(x, z);
                        glVertex3f(0+x, currentheight[0+x][0+z], 0+z);
                    glEnd();
                }
                else{
                    glBegin(GL_TRIANGLE_STRIP);
                        bindTopographicMaterial(currentheight[0+x][0+z]);
                        glTexCoord2f(0, 0);
                        bindNormals(x, z);
                        glVertex3f(0+x, currentheight[0+x][0+z], 0+z);

                        bindTopographicMaterial(currentheight[0+x][1+z]);
                        glTexCoord2f(0, 1);
                        bindNormals(x, z+1);
                        glVertex3f(0+x, currentheight[0+x][1+z], 1+z);

                        bindTopographicMaterial(currentheight[1+x][0+z]);
                        glTexCoord2f(1, 0);
                        bindNormals(x+1, z);
                        glVertex3f(1+x, currentheight[1+x][0+z], 0+z);

                        bindTopographicMaterial(currentheight[1+x][1+z]);
                        glTexCoord2f(1, 1);
                        bindNormals(x+1, z+1);
                        glVertex3f(1+x, currentheight[1+x][1+z], 1+z);
                    glEnd();
                }
            }
        }
    } else {
        // if 'shouldUseWire' is true we use a blue material instead
        // so the wires are visible against the filled terrain
        glNormal3f(0.0, 1.0, 0.0);
        glColor3f(0.0, 0.0, 1.0);
        float amb[4] = {0.0, 0.0, 0.4, 1.0};
        float diff[4] = {0.0, 0.0, 0.7, 1.0};
        float spec[4] = {0.0, 0.0, 1.0, 1.0};
        float shin = 100;
        Material(amb, diff, spec, shin).bind();
        for (int x = 0; x < (x_size-1); x++) {
            for (int z = 0; z < (z_size-1); z++) {
                // for each vertex, render it and bind a material for it
                if(!mesh){
                    glBegin(GL_QUADS);
                        glTexCoord2f(0, 0);
                        bindNormals(x, z+1);
                        glVertex3f(0+x, currentheight[0+x][1+z], 1+z);

                        glTexCoord2f(1, 0);
                        bindNormals(x+1, z+1);
                        glVertex3f(1+x, currentheight[1+x][1+z], 1+z);

                        glTexCoord2f(1, 1);
                        bindNormals(x+1, z);
                        glVertex3f(1+x, currentheight[1+x][0+z], 0+z);

                        glTexCoord2f(0, 1);
                        bindNormals(x, z);
                        glVertex3f(0+x, currentheight[0+x][0+z], 0+z);
                    glEnd();
                }
                else{
                    glBegin(GL_TRIANGLE_STRIP);
                        glTexCoord2f(0, 0);
                        bindNormals(x, z);
                        glVertex3f(0+x, currentheight[0+x][0+z], 0+z);

                        glTexCoord2f(0, 1);
                        bindNormals(x, z+1);
                        glVertex3f(0+x, currentheight[0+x][1+z], 1+z);

                        glTexCoord2f(1, 0);
                        bindNormals(x+1, z);
                        glVertex3f(1+x, currentheight[1+x][0+z], 0+z);

                        glTexCoord2f(1, 1);
                        bindNormals(x+1, z+1);
                        glVertex3f(1+x, currentheight[1+x][1+z], 1+z);
                    glEnd();
                }
            }
        }
    }
}


/**
* Display function
*/
void display()
{
    // set up camera perspective and point it at the looking point
    camera.setupPerspective();
    // clear screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera.lookAt();

    // only render the lights if lighting is enabled
    if (lighting) {
        l.render();
        l1.render();
    }

    // draw the terrain
    if (render_mode == 0) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        drawTerrain(false);
    } else if (render_mode == 1) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        drawTerrain(false);
    } else if (render_mode == 2) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        drawTerrain(false);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        drawTerrain(true);
    }

    // draw a 2d HUD
    drawHUD();

    // swap buffers
    glutSwapBuffers();
}

// used to dynamically animate the height of the terrain
// (so it raises to its actual height after generation)
void updateHeights() {
    for (int x = 0; x < x_size; x++) {
        for (int z = 0; z < z_size; z++) {
            if (currentheight[x][z] < heightmap[x][z]) {
                currentheight[x][z] += 0.01;
            }
        }
    }
}

/**
* FPS timing function to lock program to around 60fps
*/
void FPS(int val)
{
    // applies rotations
    camera.applyRotation();
    // apply movement for each of the input keys
    for (int i = 0; i < 4; i++) {
        if (movement[i]) {
            camera.applyMovement(i, 0.5);
        }
    }

    // update heights if needed
    updateHeights();

    glutPostRedisplay();
    glutTimerFunc(17, FPS, val);
}

// circle terrain generation algorithm
void terrain(int tx, int tz, float disp) {
    Point3D center = Point3D(tx, 0, tz);
    float terrainCircleSize = (x_size + z_size) / 20;
    for (int i = 0; i < x_size; i++) {
        for (int j = 0; j < z_size; j++) {
            float pd = (center.distanceTo(Point3D(i, 0, j)) * 2) / terrainCircleSize;
            if (fabs(pd) <= 1.0) {
                heightmap[i][j] += disp/2 + (cos(pd*3.14)*disp)/2;
                if (heightmap[i][j] > max_height) max_height = heightmap[i][j];
            }
        }
    }
}

// generates a new heightmap
void init_terrain() {
    delete[] heightmap;
    delete[] currentheight;
    delete[] normals;

    // initialize first dimension for heightmap
    heightmap = new float*[x_size];
    for (int i = 0; i < x_size; i++) {
        // initialize second dimension for heightmap
        heightmap[i] = new float[z_size];
        for (int j = 0; j < z_size; j++) {
            heightmap[i][j] = 0;
        }
    }

    // initialize first dimension for currentheight
    currentheight = new float*[x_size];
    for (int i = 0; i < x_size; i++) {
        // initialize second dimension for currentheight
        currentheight[i] = new float[z_size];
        for (int j = 0; j < z_size; j++) {
            currentheight[i][j] = 0;
        }
    }

    // initialize first dimension for normals
    normals = new Vec3D*[x_size];
    for (int i = 0; i < x_size; i++) {
        // initialize second dimension for normals
        normals[i] = new Vec3D[z_size];
        for (int j = 0; j < z_size; j++) {
            normals[i][j] = Vec3D();
        }
    }

    // do 1000 iterations of terrain algorithm
    float disp = (x_size+z_size) / 80;
    for (int i = 0; i < (x_size+z_size)*2.5; i++) {
        int tx = 0 + (rand() % static_cast<int>(x_size + 1));
        int tz = 0 + (rand() % static_cast<int>(z_size + 1));
        terrain(tx, tz, disp);
        disp /= 1.0005;
    }

    // compute the maximum height in use
    for (int i = 0; i < x_size; i++) {
        for (int j = 0; j < z_size; j++) {
            if (heightmap[i][j] > max_height) max_height = heightmap[i][j];
        }
    }

    // iterate over each index and compute average vertex normal from all intersections
    for (int i = 0; i < x_size; i++) {
        for (int j = 0; j < z_size; j++) {
            Vec3D up = Vec3D();
            Vec3D down = Vec3D();
            Vec3D left = Vec3D();
            Vec3D right = Vec3D();

            // compute vectors along grid lines if present
            if (i+1 < x_size) right = Vec3D(1, heightmap[i+1][j] - heightmap[i][j], 0);
            if (i-1 >= 0) left = Vec3D(-1, heightmap[i-1][j] - heightmap[i][j], 0);
            if (j+1 < z_size) up = Vec3D(0, heightmap[i][j+1] - heightmap[i][j], 1);
            if (j-1 >= 0) down = Vec3D(0, heightmap[i][j-1] - heightmap[i][j], -1);

            // compute cross products
            Vec3D ur = yfix(up.cross(right));
            Vec3D rd = yfix(right.cross(down));
            Vec3D dl = yfix(down.cross(left));
            Vec3D lu = yfix(left.cross(up));

            // average of vectors
            Vec3D fin = Vec3D((ur.mX + rd.mX + dl.mX + lu.mX) / 4, (ur.mY + rd.mY + dl.mY + lu.mY) / 4, (ur.mZ + rd.mZ + dl.mZ + lu.mZ) / 4).normalize();

            // store
            normals[i][j] = fin;
        }
    }
}

int main(int argc, char** argv)
{
    srand(time(NULL));
    // input for x and z size
    if (argc != 3) {
        std::cout << "not enough arguments" << std::endl;
        return -1;
    }
    x_size = atoi(argv[1]);
    z_size = atoi(argv[2]);

    init_terrain();

    marble.load("marble.ppm");
    aerial.load("aerial.ppm");
    teapot.load("teapot.ppm");
    baboon.load("baboon.ppm");

    std::cout << instructions << std::endl;

    // glut initialization stuff
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(screen_width, screen_height);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("A4 - Terrain");

    // disable cursor (seems not to work on unix systems)
    glutSetCursor(GLUT_CURSOR_NONE);

    // set screen clear color to black
    glClearColor(0.0, 0.0, 0.0, 1.0);

    // enable smooth shading
    glShadeModel(GL_SMOOTH);

    // depth test/face culling
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    glEnable(GL_TEXTURE_2D);

    // light properties
    float pos[4] = {0, ((float)(x_size+z_size) / 80) + 10, 0, 1};
    float pos2[4] = {(float)x_size, ((float)(x_size+z_size) / 80) + 10, (float)z_size, 1};

    float amb[4] = {0.3, 0.3, 0.3, 1.0};
    float diff[4] = {0.7, 0.7, 0.7, 1.0};
    float spec[4] = {1.0, 1.0, 1.0, 1.0};

    float amb2[4] = {0.3, 0.3, 0.3, 1.0};
    float diff2[4] = {0.5, 0.5, 0.5, 1.0};
    float spec2[4] = {0.7, 0.7, 0.7, 1.0};

    l = Light(GL_LIGHT0, pos, amb, diff, spec);
    l1 = Light(GL_LIGHT1, pos2, amb2, diff2, spec2);

    glEnable(GL_LIGHTING);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // callbacks
    glutKeyboardFunc(handleKeyboard);
    glutKeyboardUpFunc(handleKeyboardUp);
    glutSpecialFunc(special);
    glutSpecialUpFunc(specialUp);
    glutMotionFunc(motion);
    glutPassiveMotionFunc(motion);
    glutDisplayFunc(display);
    glutTimerFunc(0, FPS, 0);

    // kick off main loop
    glutMainLoop();

    return 0;
}