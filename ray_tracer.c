	// uses framework Cocoa
	// uses framework OpenGL
#include "MicroGlut.h"
#include "GL_utilities.h"
#include "VectorUtils3.h"
#include "LittleOBJLoader.h"
#include "LoadTGA.h"
#include "math.h"

#define RESOLUTION_X 1024.0f
#define RESOLUTION_Y 576.0f
#define ASPECT_RATIO RESOLUTION_X / RESOLUTION_Y
#define FOV 55.0f * M_PI / 180.0f
#define NUM_SPHERES 25
#define ROOM_1 0.0f
#define ROOM_2 2000.0f


// Reference to shader program
GLuint program;

// Camera values
vec3 cam = {0.0f, 1.0f, 1.0f};
vec3 look_at_point = {0.0f, 1.0f, -1.0f};
vec3 up_vector = {0.0f, 1.0f, 0.0f};

// Camera movement
vec3 cam_movement = {0.0f, 0.0f, 0.0f};
vec3 look_movement = {0.0f, 0.0f, 0.0f};

// Speed of sphere moving between portals, global so it can be changed by input
float portal_mover_speed = 0.005f;

// Setting up triangles that should cover screen
GLfloat triangle_vertices[] =
{
	-1.0f, 1.0f, 0.0f,
	-1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	1.0f, 1.0f, 0.0f
};
GLubyte triangle_indices[] =
{ 0, 2, 1,  0, 3, 2};
// VAO
unsigned int vertexArrayObjID;

// Setting up spheres
vec3 spherePos[] = {
  {0.0f + ROOM_1, -500.0f, 0.0f},  // Room 1
  {0.0f + ROOM_1, 507.0f, 0.0f},
  {-505.0f + ROOM_1, 5.0f, 0.0f},
  {505.0f + ROOM_1, 5.0f, 0.0f},
  {0.0f + ROOM_1, 5.0f, -505.0f},
  {0.0f + ROOM_1, 5.0f, 505.0f},

  {0.0f + ROOM_2, -500.0f, 0.0f},  // Room 2
  {0.0f + ROOM_2, 507.0f, 0.0f},
  {-505.0f + ROOM_2, 5.0f, 0.0f},
  {505.0f + ROOM_2, 5.0f, 0.0f},
  {0.0f + ROOM_2, 5.0f, -505.0f},
  {0.0f + ROOM_2, 5.0f, 505.0f},

  {-2.0f, 1.01f, -2.0f},  // Mirror sphere
  {0.0f, 0.701f, -2.5f},  // Refractive 1
  {-2.8f, 0.501f, -0.5f},  // Reflective
  {3.0f, 1.11f, 3.0f},  // Portal 1
  {-3.0f + ROOM_2, 1.11f, -3.0f},  // Portal 2
  {-3.2f + ROOM_2, 4.01f, 3.0f},  // Portal 3
  {3.2f + ROOM_2, 4.01f, 3.0f},  // Portal 4
  {0.0f + ROOM_2, 4.01f, 3.0f},  // Portal mover
  {-3.2f, 4.01f, 3.0f}, // Portal 5
  {3.2f, 4.01f, 3.0f}, // Portal 6
  {2.0f + ROOM_2, 2.5f, -2.0f}, // Planet 1
  {0.0f + ROOM_2, 2.0f, 0.0f}, // Planet 2
  {-2.5f + ROOM_2, 2.0f, 0.0f} // Planet 3
};
vec3 spherePosFake[] = {
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 0.0f}
};
// Using rotations to rotate textures
mat3 sphereRot[NUM_SPHERES];
vec3 sphereColors[] = {
  {0.0f, 1.0f, 0.0f},  // Room 1
  {1.0f, 1.0f, 0.0f},
  {0.9f, 0.9f, 0.9f},
  {0.9f, 0.9f, 0.9f},
  {0.45f, 0.9f, 0.82f},
  {0.45f, 0.9f, 0.82f},

  {0.9f, 0.9f, 0.9f},  // Room 2
  {0.0f, 1.0f, 1.0f},
  {0.98f, 0.28f, 0.76f},
  {0.98f, 0.28f, 0.76f},
  {0.9f, 0.9f, 0.9f},
  {0.9f, 0.9f, 0.9f},

  {0.9f, 0.9f, 0.9f},  // Mirror sphere
  {0.9f, 0.9f, 0.9f},  // Refractive 1
  {0.9f, 0.0f, 0.9f},  // Reflective
  {0.9f, 0.9f, 0.9f},  // Portal 1
  {0.9f, 0.9f, 0.9f},  // Portal 2
  {0.9f, 0.9f, 0.9f},  // Portal 3
  {0.9f, 0.9f, 0.9f},  // Portal 4
  {0.0f, 1.0f, 0.0f},  // Portal mover
  {0.9f, 0.9f, 0.9f},  // Portal 5
  {0.9f, 0.9f, 0.9f},  // Portal 6
  {1.0f, 1.0f, 0.0f},  // Planet 1
  {0.9f, 0.9f, 0.9f},  // Planet 2
  {0.0f, 0.0f, 1.0f}  // Planet 3
};
vec2 k_rt[] = {
  {0.2f, 0.0f},  // Room 1
  {0.02f, 0.0f},
  {0.2f, 0.0f},
  {0.2f, 0.0f},
  {0.2f, 0.0f},
  {0.2f, 0.0f},

  {0.2f, 0.0f},  // Room 2
  {0.02f, 0.0f},
  {0.2f, 0.0f},
  {0.2f, 0.0f},
  {0.2f, 0.0f},
  {0.2f, 0.0f},

  {0.7f, 0.0f},  // Mirror sphere
  {0.1f, 0.7f},  // Refractive 1
  {0.1f, 0.0f},  // Reflective
  {0.0f, 0.0f},  // Portal 1
  {0.0f, 0.0f},  // Portal 2
  {0.0f, 0.0f},  // Portal 3
  {0.0f, 0.0f},  // Portal 4
  {0.7f, 0.0f},  // Portal mover
  {0.1f, 0.0f},  // Portal 5
  {0.1f, 0.0f},  // Portal 6
  {0.01f, 0.0f},  // Planet 1
  {0.01f, 0.0f},  // Planet 2
  {0.01f, 0.0f}  // Planet 3
};
float sphereRadius[] = { 500.0f, 500.0f, 500.0f, 500.0f, 500.0f, 500.0f,
																				500.0f, 500.0f, 500.0f, 500.0f, 500.0f, 500.0f,
																				1.0f, 0.7f, 0.5f, 1.1f, 1.1f, 1.1f, 1.1f, 0.6f, 1.1f, 1.1f, 0.6f, 0.23f, 0.13f };
GLint sphereTextureIndex[] = { 0, -1, -1, -1, -1, -1,  // Room 1
																										0, -1, -1, -1, -1, -1,  // Room 2
																										-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 3, 1, 2 };
GLint spherePortalIndices[] = { -1, -1, -1, -1, -1, -1,
																										-1, -1, -1, -1, -1, -1,
																										-1, -1, -1, 16, 15, 21, 20, -1, 18, 17, -1, -1, -1 };
GLint insidePortal[] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
// Setting up textures for spheres
GLuint tex1, tex2, tex3, tex4;

// Setting up light light sources
vec3 lightSourcesPositions[] = {
  {0.0f + ROOM_1, 5.0f, 0.0f},
  {0.0f + ROOM_2, 5.0f, 0.0f}
};
GLfloat lightSourcesIntensities[] = { 1.0f, 1.0f };
GLfloat specularExponents[] = { 160.0f, 160.0f };


void uploadSphereInformation(void)
{
  glUniform3fv(glGetUniformLocation(program, "pos"),
                          NUM_SPHERES, &spherePos[0].x);
  glUniform3fv(glGetUniformLocation(program, "posFake"),
                          NUM_SPHERES, &spherePosFake[0].x);
  glUniformMatrix3fv(glGetUniformLocation(program, "rot"),
                          NUM_SPHERES, GL_TRUE, sphereRot[0].m);
  glUniform3fv(glGetUniformLocation(program, "color"),
                          NUM_SPHERES, &sphereColors[0].x);
  glUniform2fv(glGetUniformLocation(program, "k_rt"),
                          NUM_SPHERES, &k_rt[0].x);
  glUniform1fv(glGetUniformLocation(program, "radius"),
                          NUM_SPHERES, sphereRadius);
  glUniform1iv(glGetUniformLocation(program, "textureIndex"),
                          NUM_SPHERES, sphereTextureIndex);
  glUniform1iv(glGetUniformLocation(program, "spherePortalIndex"),
                          NUM_SPHERES, spherePortalIndices);
  glUniform1iv(glGetUniformLocation(program, "insidePortal"),
                          NUM_SPHERES, insidePortal);
}

void init(void)
{
	// GL inits
	glClearColor(0.2, 0.2, 0.5, 0);
	printError("GL inits");

	// Load and compile shader
	program = loadShaders("ray_tracer.vert", "ray_tracer.frag");
	glUseProgram(program);
	printError("Init shader");

  // VAO for triangles
  glGenVertexArrays(1, &vertexArrayObjID);
  glBindVertexArray(vertexArrayObjID);

	// VBOs for triangles
	unsigned int triangleVerticesBufferID;
	unsigned int triangleIndicesBufferID;

  // Set up vertices
  glGenBuffers(1, &triangleVerticesBufferID);
  glBindBuffer(GL_ARRAY_BUFFER, triangleVerticesBufferID);
  glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_vertices)*sizeof(GLfloat), triangle_vertices, GL_STATIC_DRAW);
  glVertexAttribPointer(glGetAttribLocation(program, "in_position"), 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(glGetAttribLocation(program, "in_position"));
	printError("Set up triangle vertices");

  // Set up indices
  glGenBuffers(1, &triangleIndicesBufferID);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangleIndicesBufferID);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangle_indices)*sizeof(GLubyte), triangle_indices, GL_STATIC_DRAW);
	printError("Set up triangle indices");

	// Upload resolution vector, aspect ratio, and fov to fragment shader
	glUniform2f(glGetUniformLocation(program, "resolution"), RESOLUTION_X, RESOLUTION_Y);
	glUniform1f(glGetUniformLocation(program, "aspect_ratio"), ASPECT_RATIO);
	glUniform1f(glGetUniformLocation(program, "fov"), FOV);
	printError("Upload resolution error");

	// Create initial rotation matrices, for rotating the textures on the spheres
	// Rotate 89 degrees to avoid seam on floor
	for (int i = 0; i < NUM_SPHERES; ++i)
		sphereRot[i] = mat4tomat3(IdentityMatrix());
	sphereRot[0] = mat4tomat3(Rz(89 * M_PI / 180));
	sphereRot[1] = mat4tomat3(Rz(89 * M_PI / 180));
	sphereRot[6] = mat4tomat3(Rz(89 * M_PI / 180));
	sphereRot[7] = mat4tomat3(Rz(89 * M_PI / 180));

	// Upload spheres to shader
	uploadSphereInformation();

	// Set up and upload textures
	LoadTGATextureSimple("checkerboard.tga", &tex1);
	LoadTGATextureSimple("hoth.tga", &tex2);
	LoadTGATextureSimple("planet.tga", &tex3);
	LoadTGATextureSimple("mars.tga", &tex4);
	// IMPORTANT! Load before binding, since load function binds it incorrectly first
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, tex2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, tex3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, tex4);
	glUniform1i(glGetUniformLocation(program, "tex0"), 0);
	glUniform1i(glGetUniformLocation(program, "tex1"), 1);
	glUniform1i(glGetUniformLocation(program, "tex2"), 2);
	glUniform1i(glGetUniformLocation(program, "tex3"), 3);

	// Upload light sources to shader
  glUniform3fv(glGetUniformLocation(program, "light_source_pos_arr"),
                          2, &lightSourcesPositions[0].x);
  glUniform1fv(glGetUniformLocation(program, "light_source_intensity"),
                        	2, lightSourcesIntensities);
  glUniform1fv(glGetUniformLocation(program, "specular_exponent"),
                        	2, specularExponents);
}

void miscControls(void)
{
  if (glutKeyIsDown('n'))
  {
		portal_mover_speed = (portal_mover_speed - 0.00005f > 0.0f) ? portal_mover_speed - 0.00005f : 0.0f;
  }
  if (glutKeyIsDown('m'))
  {
		portal_mover_speed = (portal_mover_speed + 0.00005f <= 0.01f) ? portal_mover_speed + 0.00005f : 0.01f;
  }
  if (glutKeyIsDown('1'))
  {
	  glUniform1i(glGetUniformLocation(program, "aa"), 1);
  }
  if (glutKeyIsDown('2'))
  {
	  glUniform1i(glGetUniformLocation(program, "aa"), 2);
  }
  if (glutKeyIsDown('3'))
  {
	  glUniform1i(glGetUniformLocation(program, "aa"), 3);
  }
}

void smoothMovement(void)
{
	vec3 forward_vector = VectorSub(look_at_point, cam);
	vec3 left_vector = CrossProduct(forward_vector, up_vector);

  if (glutKeyIsDown('w'))
  {
		cam_movement.z -= 0.0001f;
  }
  if (glutKeyIsDown('s'))
  {
		cam_movement.z += 0.0001f;
  }
  if (glutKeyIsDown('a'))
  {
		cam_movement.x -= 0.0001f;
  }
  if (glutKeyIsDown('d'))
  {
		cam_movement.x += 0.0001f;
  }
	if (glutKeyIsDown(GLUT_KEY_UP))
	{
		look_movement.y += 0.0001f;
	}
	if (glutKeyIsDown(GLUT_KEY_DOWN))
	{
		look_movement.y -= 0.0001f;
	}
	if (glutKeyIsDown(GLUT_KEY_LEFT))
	{
		look_movement.x -= 0.0001f;
	}
	if (glutKeyIsDown(GLUT_KEY_RIGHT))
	{
		look_movement.x += 0.0001f;
	}

	forward_vector = VectorAdd(forward_vector, ScalarMult(left_vector, look_movement.x));
	forward_vector = VectorAdd(forward_vector, ScalarMult(up_vector, look_movement.y));
	look_at_point = VectorAdd(cam, forward_vector);

	cam = VectorAdd(cam, ScalarMult(left_vector, cam_movement.x));
	cam = VectorSub(cam, ScalarMult(forward_vector, cam_movement.z));
	look_at_point = VectorAdd(look_at_point, ScalarMult(left_vector, cam_movement.x));
	look_at_point = VectorSub(look_at_point, ScalarMult(forward_vector, cam_movement.z));
}

void normalMovement(void)
{
	float movementFactor = 0.02f;
  vec3 dir = Normalize(VectorSub(look_at_point, cam));
  if (glutKeyIsDown('w'))
  {
		vec3 movementVector = ScalarMult(dir, movementFactor);
    look_at_point = VectorAdd(look_at_point, movementVector);
    cam = VectorAdd(cam, movementVector);
  }
  if (glutKeyIsDown('s'))
  {
		vec3 movementVector = ScalarMult(dir, movementFactor);
    look_at_point = VectorSub(look_at_point, movementVector);
    cam = VectorSub(cam, movementVector);
  }
  if (glutKeyIsDown('a'))
  {
    vec3 movementVector = ScalarMult(CrossProduct(dir, up_vector), movementFactor);
    look_at_point = VectorSub(look_at_point, movementVector);
    cam = VectorSub(cam, movementVector);
  }
  if (glutKeyIsDown('d'))
  {
    vec3 movementVector = ScalarMult(CrossProduct(dir, up_vector), movementFactor);
    look_at_point = VectorAdd(look_at_point, movementVector);
    cam = VectorAdd(cam, movementVector);
  }
	if (glutKeyIsDown(GLUT_KEY_UP))
	{
		look_at_point = VectorAdd(look_at_point, ScalarMult(up_vector, movementFactor));
	}
	if (glutKeyIsDown(GLUT_KEY_DOWN))
	{
		look_at_point = VectorSub(look_at_point, ScalarMult(up_vector, movementFactor));
	}
	if (glutKeyIsDown(GLUT_KEY_LEFT))
	{
	  dir = Normalize(VectorSub(look_at_point, cam));
    vec3 newDir = MultVec3(Ry(movementFactor), dir);
		look_at_point = VectorAdd(cam, newDir);
	}
	if (glutKeyIsDown(GLUT_KEY_RIGHT))
	{
	  dir = Normalize(VectorSub(look_at_point, cam));
    vec3 newDir = MultVec3(Ry(-movementFactor), dir);
		look_at_point = VectorAdd(cam, newDir);
	}
}

void handlePlayerPortalInteraction(void)
{
	for (int i = 0; i < NUM_SPHERES; ++i)
	{
		if (spherePortalIndices[i] != -1)
		{
			vec3 camToSphere = VectorSub(spherePos[i], cam);
			vec3 sphereNormal = Normalize(VectorSub(cam, spherePos[i]));
			vec3 camToPoint = VectorSub(look_at_point, cam);
			if (Norm(camToSphere) <= sphereRadius[i])
			{
				// Calculate exit position at the connected portal
				vec3 normal = ScalarMult(sphereNormal, sphereRadius[spherePortalIndices[i]]);
				float throughDistance = fabs(DotProduct(sphereNormal, Normalize(camToSphere)))
																							 * 2.01f * sphereRadius[spherePortalIndices[i]];
				vec3 throughVector = ScalarMult(Normalize(camToSphere), throughDistance);
				vec3 normalFar = VectorAdd(normal, throughVector);

				// Update camera position and look at point
				cam = VectorAdd(spherePos[spherePortalIndices[i]], normalFar);
				look_at_point = VectorAdd(cam, camToPoint);
				break;
			}
		}
	}
}

void handleSpherePortalInteraction(void)
{
	for (int i = 0; i < NUM_SPHERES; ++i)
	{
		insidePortal[i] = -1;
		for (int j = 0; j < NUM_SPHERES; ++j)
		{
			if (i == j) continue;

			// Check if sphere is partly within  portal, if so, spawn fake double in the other portal
			vec3 centerToCenter = VectorSub(spherePos[j], spherePos[i]);
			if (spherePortalIndices[j] != -1 && Norm(centerToCenter) - sphereRadius[i] <= sphereRadius[j] &&
					 sphereRadius[i] < sphereRadius[j])
			{
				insidePortal[i] = j;

				// Calculate position for fake double
				float throughDistance = 1.01f * sphereRadius[spherePortalIndices[j]] +
																							 (sphereRadius[j] - Norm(centerToCenter));
				vec3 fakePosition = VectorAdd(spherePos[spherePortalIndices[j]],
																													 ScalarMult(Normalize(centerToCenter), throughDistance));
				spherePosFake[i] = fakePosition;

				// Check if sphere is completely within portal, if so, move the sphere to the other portal
				if (Norm(centerToCenter) + sphereRadius[i] <= sphereRadius[j])
				{
					insidePortal[i] = -1;
				  spherePos[i] = fakePosition;
				}

				break;
			}
		}
	}
}

void display(void)
{
	// Update keyboard input
	miscControls();
	normalMovement();

	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT);

  // Upload time
  GLfloat t = (GLfloat)glutGet(GLUT_ELAPSED_TIME);
  glUniform1f(glGetUniformLocation(program, "time"), t);

	// Rotate floor
	sphereRot[0] = MultMat3(mat4tomat3(Ry(-0.00001)), sphereRot[0]);

	// Move portal sphere
	vec3 movement = ScalarMult(VectorSub(spherePos[17], spherePos[18]), portal_mover_speed);
	spherePos[19] = VectorAdd(movement, spherePos[19]);

	// Move planets
	vec3 axis = {0.3f, 0.7f, 0.0f};
	vec3 z_axis = {0.0f, 0.0f, -1.0f};
	vec3 rotationArm = ScalarMult(CrossProduct(axis, z_axis), 2.0f);
	spherePos[23] = VectorAdd(spherePos[22], MultVec3(ArbRotate(axis, t / 1000.0f), rotationArm));
	rotationArm = ScalarMult(CrossProduct(axis, z_axis), 0.65f);
	spherePos[24] = VectorAdd(spherePos[23], MultVec3(ArbRotate(axis, t / 500.0f), rotationArm));

	// Rotate planets
	sphereRot[22] = MultMat3(mat4tomat3(Ry(-0.02)), sphereRot[22]);
	sphereRot[23] = MultMat3(mat4tomat3(ArbRotate(axis, 0.03)), sphereRot[23]);
	sphereRot[24] = MultMat3(mat4tomat3(ArbRotate(axis, -0.04f)), sphereRot[24]);

	// Handle portal system
	handlePlayerPortalInteraction();
	handleSpherePortalInteraction();

	// Finally upload all updated sphere information to the shader
	uploadSphereInformation();

	// Build and upload inverse view matrix
	mat4 view_matrix = lookAt(cam.x, cam.y, cam.z,
                                                look_at_point.x, look_at_point.y, look_at_point.z,
                                                0.0, 1.0, 0.0);
	glUniformMatrix4fv(glGetUniformLocation(program, "inverse_view_matrix"), 1, GL_TRUE,
																		InvertMat4(view_matrix).m);
	printError("View matrix error");

  // Draw two triangles covering entire viewport, to activate fragment shader for every pixel
  glBindVertexArray(vertexArrayObjID);
  glDrawElements(GL_TRIANGLES, sizeof(triangle_indices), GL_UNSIGNED_BYTE, NULL);
	printError("Draw elements error");

	glutSwapBuffers();
}

void timer(int i)
{
	glutTimerFunc(16, &timer, i);
	glutPostRedisplay();
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
	glutInitContextVersion(3, 2);
	glutInitWindowSize (RESOLUTION_X, RESOLUTION_Y);
	glutCreateWindow ("Ray Tracer");
	glutDisplayFunc(display); init ();
	glutTimerFunc(16, &timer, 0);
	glutMainLoop();
	exit(0);
}
