#ifdef __APPLE__
  #include <OpenGL/gl.h>
  #include <OpenGL/glu.h>
  #include <GLUT/glut.h>
#else
  #include <GL/gl.h>
  #include <GL/glu.h>
  #include <GL/freeglut.h>
#endif

#include "light.h"

Light::Light() {}

/**
* Constructs a light object, wrapper for GL's builtin lights
* light should be e.g. GL_LIGHT0, the four floats are arrays for light colors/positions.
*/
Light::Light(GLenum light, float position[], float ambient[], float diffuse[], float specular[]) {
	this->boundLight = light;
	for (int i = 0; i < 4; i++) {
		this->position[i] = position[i];
		this->ambient[i] = ambient[i];
		this->diffuse[i] = diffuse[i];
		this->specular[i] = specular[i];
	}

	// set attenuations for light falloff
	this->constant = 0.0;
	this->linear = 0.02;
	this->quadratic = 0.0;

	// enable the light
	glEnable(this->boundLight);
}

/**
* Rendering the scene, light needs to be re-applied.
*/
void Light::render() {
	glLightfv(this->boundLight, GL_POSITION, this->position);
	glLightfv(this->boundLight, GL_AMBIENT, this->ambient);
	glLightfv(this->boundLight, GL_DIFFUSE, this->diffuse);
	glLightfv(this->boundLight, GL_SPECULAR, this->specular);
	glLightf(this->boundLight, GL_CONSTANT_ATTENUATION, this->constant);
	glLightf(this->boundLight, GL_LINEAR_ATTENUATION, this->linear);
	glLightf(this->boundLight, GL_QUADRATIC_ATTENUATION, this->quadratic);
}

/**
* Updates a light's specific property.
*/
void Light::update(GLenum property, float data[]) {
	switch(property) {
		case GL_POSITION:
		{
			for (int i = 0; i < 4; i++) {
				this->position[i] = data[i];
			}
			break;
		}
		case GL_AMBIENT:
		{
			for (int i = 0; i < 4; i++) {
				this->ambient[i] = data[i];
			}
			break;
		}
		case GL_DIFFUSE:
		{
			for (int i = 0; i < 4; i++) {
				this->diffuse[i] = data[i];
			}
			break;
		}
		case GL_SPECULAR:
		{
			for (int i = 0; i < 4; i++) {
				this->specular[i] = data[i];
			}
			break;
		}
		case GL_CONSTANT_ATTENUATION:
		{
			this->constant = data[0];
			break;
		}
		case GL_LINEAR_ATTENUATION:
		{
			this->linear = data[0];
			break;
		}
		case GL_QUADRATIC_ATTENUATION:
		{
			this->quadratic = data[0];
			break;
		}
	}
}