#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define NO_RANDOM_BRICKS 15
using namespace std;
struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;
GLuint programID;
float zoom=1 ,pan=0,pan_mous_loc_x=0,pan_mous_loc_y=0;
int pan_selected=0;
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {
	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}
	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}
	GLint Result = GL_FALSE;
	int InfoLogLength;
	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);
	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);
	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);
	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);
	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);
	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
}
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }
    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}
void draw3DObject (struct VAO* vao)
{
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);
    glBindVertexArray (vao->VertexArrayID);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);
	GLfloat fov = 90.0f;
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);
    Matrices.projection = glm::ortho((-4.0f+pan)/zoom, (4.0f+pan)/zoom, -4.0f/zoom, 4.0f/zoom, 0.1f, 400.0f);
}

VAO* createRectangle(float w,float h)
{
    GLfloat vertex_buffer_data [2*9]={
        0,0,0,
        w,0,0,
        w,h,0,

        0,0,0,
        w,h,0,
        0,h,0

    };
    GLfloat color_buffer_data [2*9]={
        0,0,0,
        0,0,0,
        0,0,0,
        0,0,0,
        0,0,0,
        0,0,0
    };
    return create3DObject(GL_TRIANGLES, 2*3 , vertex_buffer_data, color_buffer_data, GL_FILL);
}



float camera_rotation_angle = 90;
struct COLOR {
    float r;
    float g;
    float b;
};
typedef struct COLOR COLOR;
COLOR grey = {168.0/255.0,168.0/255.0,168.0/255.0};
COLOR gold = {218.0/255.0,165.0/255.0,32.0/255.0};
COLOR coingold = {255.0/255.0,223.0/255.0,0.0/255.0};
COLOR broungold ={153/255.0,101/255.0,21/255.0};
COLOR steel ={123/255.0,137/255.0,155/255.0};
COLOR red = {255.0/255.0,51.0/255.0,51.0/255.0};
COLOR darkred ={255.0/255.0,0.0/255.0,0.0/255.0};
COLOR lightgreen = {57/255.0,230/255.0,0/255.0};
COLOR darkgreen = {51/255.0,102/255.0,0/255.0};
COLOR black = {30/255.0,30/255.0,21/255.0};
COLOR blue = {0,0,1};
COLOR darkbrown = {46/255.0,46/255.0,31/255.0};
COLOR lightbrown = {95/255.0,63/255.0,32/255.0};
COLOR brown1 = {117/255.0,78/255.0,40/255.0};
COLOR brown2 = {134/255.0,89/255.0,40/255.0};
COLOR brown3 = {46/255.0,46/255.0,31/255.0};
COLOR cratebrown = {153/255.0,102/255.0,0/255.0};
COLOR cratebrown1 = {121/255.0,85/255.0,0/255.0};
COLOR cratebrown2 = {102/255.0,68/255.0,0/255.0};
COLOR skyblue2 = {113/255.0,185/255.0,209/255.0};
COLOR skyblue1 = {123/255.0,201/255.0,227/255.0};
COLOR skyblue = {132/255.0,217/255.0,245/255.0};
COLOR cloudwhite = {229/255.0,255/255.0,255/255.0};
COLOR cloudwhite1 = {204/255.0,255/255.0,255/255.0};
COLOR lightpink = {255/255.0,122/255.0,173/255.0};
COLOR darkpink = {255/255.0,51/255.0,119/255.0};
COLOR white = {255/255.0,255/255.0,255/255.0};
COLOR score = {117/255.0,78/255.0,40/255.0};

void initGL (GLFWwindow* window, int width, int height)
{
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");
	reshapeWindow (window, width, height);
	glClearColor (135/255.0f, 260/255.0f, 250/255.0f, 0.0f);
	glClearDepth (1.0f);
	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);
    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}


GLfloat digitopbar[] = {
                  0.1/2,0.35/2,1/2,
                  -0.1/2,0.35/2,1/2,
                  -0.1/2,0.30/2,1/2,

                  -0.1/2,0.30/2,1/2,
                  0.1/2,0.30/2,1/2,
                  0.1/2,0.35/2,1
                };
GLfloat digitmidbar [] = {
                  0.1/2,0.05/2,1/2,
                  -0.1/2,0.05/2,1/2,
                  -0.1/2,-0.05/2,1/2,

                  -0.1/2,-0.05/2,1/2,
                  0.1/2,-0.05/2,1/2,
                  0.1/2,0.05/2,1
                };
GLfloat digitbotbar [] = {
                0.1/2,-0.30/2,1/2,
                -0.1/2,-0.30/2,1/2,
                -0.1/2,-0.35/2,1/2,

                -0.1/2,-0.35/2,1/2,
                0.1/2,-0.35/2,1/2,
                0.1/2,-0.30/2,1
                };
GLfloat digitlefttopbar [] = {
                              -0.05/2,0.30/2,1/2,
                              -0.1/2,0.30/2,1/2,
                              -0.1/2,0.05/2,1/2,

                              -0.1/2,0.05/2,1/2,
                              -0.05/2,0.05/2,1/2,
                              -0.05/2,0.30/2,1
                            };
GLfloat digitleftbotbar [] = {
                              -0.05/2,-0.05/2,1/2,
                              -0.1/2,-0.05/2,1/2,
                              -0.1/2,-0.30/2,1/2,

                              -0.1/2,-0.30/2,1/2,
                              -0.05/2,-0.30/2,1/2,
                              -0.05/2,-0.05/2,1
                              };
GLfloat digitrighttopbar [] = {
                              0.1/2,0.30/2,1/2,
                              0.05/2,0.30/2,1/2,
                              0.05/2,0.05/2,1/2,

                              0.05/2,0.05/2,1/2,
                              0.1/2,0.05/2,1/2,
                              0.1/2,0.30/2,1
                              };
GLfloat digitrightbotbar [] = {
                            0.1/2,-0.05/2,1/2,
                            0.05/2,-0.05/2,1/2,
                            0.05/2,-0.30/2,1/2,

                            0.05/2,-0.30/2,1/2,
                            0.1/2,-0.30/2,1/2,
                            0.1/2,-0.05/2,1
                            };



float dis(float x1,float y1,float x2,float y2){
    return (x1-x2)*(x1-x2)+(y1-y2)*(y1-y2);
}
int mirrTouch(float laser_x,float laser_y,float mirror_x,float mirror_y,float ang){
    if((mirror_y-laser_y)-tan(ang*M_PI/180.0f)*(mirror_x-laser_x)<0.2 && (mirror_y-laser_y)-tan(ang*M_PI/180.0f)*(mirror_x-laser_x)>-0.2 ){
        return 1;
    }else{
        return 0;
    }
}
class mirrorClass{
    public:
        VAO *drawingObj;
        float x,y,z;
        float rotation_angle;
        mirrorClass(float x_coor=0, float y_coor=0,  float z_coor=0,float angle=0){
            x = x_coor;
            y = y_coor;
            z = z_coor;
            rotation_angle = angle;
        }
        VAO* createRegularPolygon()
        {
            GLfloat vertex_buffer_data [9]={
                0.5,  0,     0,
                0,    0.25,  0,
                -0.5,  0,   0
            };
            GLfloat color_buffer_data [9]={
                0.5, 0.5,0.5,
                1 ,1,1,
                0.5, 0.5,0.5
            };
            drawingObj = create3DObject(GL_TRIANGLES, 3 , vertex_buffer_data, color_buffer_data, GL_FILL);
        }
        void draw ()
        {
            glUseProgram (programID);
            glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
            glm::vec3 target (0, 0, 0);
            glm::vec3 up (0, 1, 0);
            Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
            glm::mat4 VP = Matrices.projection * Matrices.view;
            glm::mat4 MVP;	// MVP = Projection * View * Model

            Matrices.model = glm::mat4(1.0f);
            glm::mat4 transl = glm::translate (glm::vec3(x,y,z));
            glm::mat4 rotate = glm::rotate((float)(rotation_angle*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
            Matrices.model *= (transl*rotate);
            MVP = VP * Matrices.model;
            glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
            draw3DObject(drawingObj);
        }

};

class blockClass{
    public:
        VAO *drawingObj;
        float x,y,z;
        float rotation_angle;
        int ang_x,ang_y,ang_z;


    public:
        blockClass(float x_coor=0, float y_coor=0,  float z_coor=0,float angle=0,int ang_x_set=0,int ang_y_set=0,int ang_z_set=1){
            x = x_coor;
            y = y_coor;
            z = z_coor;
            rotation_angle = angle;
            ang_x=ang_x_set;
            ang_y=ang_y_set;
            ang_z=ang_z_set;
        }
        void loc_trans(float a,float b,float c ,float ang){
            x = x + a;
            y = y + b;
            z = z + c;
            rotation_angle = rotation_angle + ang ;
        }
        void ang_set(float ang ,int x,int y,int z){
            rotation_angle=ang;
            ang_x=x;
            ang_y=y;
            ang_z=z;
        }
        void loc_set(float a,float b,float c)
        {
            x=a;
            y=b;
            z=c;
        }
        VAO* createRegularPolygon(float radius , int no_sides,COLOR coco)
        {
            GLfloat vertex_buffer_data [no_sides*9];
            GLfloat color_buffer_data [no_sides*9];
            int k=0;
            float angle_suspended = 360/no_sides;
            while(k < no_sides)//some stuf
            {
                vertex_buffer_data [k*9+0]=0;
                color_buffer_data [k*9+0]=coco.r;
                vertex_buffer_data [k*9+1]=0;
                color_buffer_data [k*9+1]=coco.g;
                vertex_buffer_data [k*9+2]=0;
                color_buffer_data [k*9+2]=coco.b;

                vertex_buffer_data [k*9+3]=radius*cos((float)k*angle_suspended*M_PI/180.0f);
                color_buffer_data [k*9+3]=coco.r;
                vertex_buffer_data [k*9+4]=radius*sin((float)k*angle_suspended*M_PI/180.0f);
                color_buffer_data [k*9+4]=coco.g;
                vertex_buffer_data [k*9+5]=0;
                color_buffer_data [k*9+5]=coco.b;

                vertex_buffer_data [k*9+6]=radius*cos((float)(k+1)*angle_suspended*M_PI/180.0f);;
                color_buffer_data [k*9+6]=coco.r;
                vertex_buffer_data [k*9+7]=radius*sin((float)(k+1)*angle_suspended*M_PI/180.0f);;
                color_buffer_data [k*9+7]=coco.g;
                vertex_buffer_data [k*9+8]=0;
                color_buffer_data [k*9+8]=coco.b;
                k++;
            }
            drawingObj = create3DObject(GL_TRIANGLES, no_sides*3 , vertex_buffer_data, color_buffer_data, GL_FILL);
        }
        void draw ()
        {
            glUseProgram (programID);
            glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
            glm::vec3 target (0, 0, 0);
            glm::vec3 up (0, 1, 0);
            Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
            glm::mat4 VP = Matrices.projection * Matrices.view;
            glm::mat4 MVP;	// MVP = Projection * View * Model

            Matrices.model = glm::mat4(1.0f);
            glm::mat4 transl = glm::translate (glm::vec3(x,y,z));
            glm::mat4 rotate = glm::rotate((float)(rotation_angle*M_PI/180.0f), glm::vec3(ang_x,ang_y,ang_z)); // rotate about vector (-1,1,1)
            Matrices.model *= (transl*rotate);
            MVP = VP * Matrices.model;
            glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
            draw3DObject(drawingObj);
        }
};


//<<<<<<<<<<<<<<<<<ALL GLOBAL VARIABLES>>>>>>>>>>>>>>>>>>>>>

blockClass testclass ;
blockClass canon_hindge , canon_body ,laser;
blockClass bin_left_top,bin_right_top,bin_left_body,bin_right_body,bin_left_bottom,bin_right_bottom;
blockClass pig_left_ear,pig_left_body,pig_left_leg1,pig_left_leg2,pig_left_tale,pig_left_nose,pig_left_eye;
blockClass pig_right_ear,pig_right_body,pig_right_leg1,pig_right_leg2,pig_right_tale,pig_right_nose,pig_right_eye;

mirrorClass mirror1(2.2,2,0,-50),mirror2(3,-2,0,-110),mirror3(-1,-2.5,0,150),mirror4(-3,3,0,40);
blockClass bricks_body[NO_RANDOM_BRICKS];

float bin_width=0.4,bin_height=0.3,canon_angle=0,laser_angle=0;
VAO *canon_piston;
int spaceclicked=0 ,bin_right_selected=0,bin_left_selected=0,canon_selected=0;
int pig_left_selected=0,pig_right_selected=0;
float triangle_rot_dir = 1;
float rectangle_rot_dir = 1;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;
int gamelaps=5;
void draw ()
{
      glUseProgram (programID);
      glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
      glm::vec3 target (0, 0, 0);
      glm::vec3 up (0, 1, 0);
      Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
      glm::mat4 VP = Matrices.projection * Matrices.view;
      glm::mat4 MVP;
    //<<<<<<<<<<<<<<<<<canon_piston>>>>>>>>>>>>>>>>>>>>
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translate_canon_piston_origin = glm::translate (glm::vec3(0,-0.025,0));
    glm::mat4 translate_canon_piston = glm::translate (glm::vec3(canon_body.x, canon_body.y, canon_body.z));
    glm::mat4 rotate_canon_piston = glm::rotate((float)(canon_angle*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
    Matrices.model *= (translate_canon_piston*rotate_canon_piston*translate_canon_piston_origin);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(canon_piston);
}

void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
     // Function is called first on GLFW_PRESS.

    if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_SPACE:
                if(spaceclicked==0){
                    spaceclicked=1;
                    laser_angle=canon_angle;
                }
                break;
            case GLFW_KEY_W:
                canon_body.loc_trans(0,0.2,0,0);
                canon_hindge.loc_trans(0,0.2,0,0);
                laser.loc_trans(0,0.2,0,0);
                break;
            case GLFW_KEY_S:
                canon_body.loc_trans(0,-0.2,0,0);
                canon_hindge.loc_trans(0,-0.2,0,0);
                laser.loc_trans(0,-0.2,0,0);
                break;
            case GLFW_KEY_A:
                canon_angle = canon_angle+10;
                break;
            case GLFW_KEY_D:
                canon_angle = canon_angle-10;
                break;
            case GLFW_KEY_Z:
                pig_left_body.loc_trans(-0.3,0,0,0);
                pig_left_ear.loc_trans(-0.3,0,0,0);
                pig_left_eye.loc_trans(-0.3,0,0,0);
                pig_left_nose.loc_trans(-0.3,0,0,0);
                pig_left_tale.loc_trans(-0.3,0,0,0);
                pig_left_leg2.loc_trans(-0.3,0,0,0);
                pig_left_leg1.loc_trans(-0.3,0,0,0);
                break;
            case GLFW_KEY_X:
                pig_left_body.loc_trans(0.3,0,0,0);
                pig_left_ear.loc_trans(0.3,0,0,0);
                pig_left_eye.loc_trans(0.3,0,0,0);
                pig_left_tale.loc_trans(0.3,0,0,0);
                pig_left_nose.loc_trans(0.3,0,0,0);
                pig_left_leg2.loc_trans(0.3,0,0,0);
                pig_left_leg1.loc_trans(0.3,0,0,0);
                break;
            case GLFW_KEY_C:
                pig_right_body.loc_trans(-0.3,0,0,0);
                pig_right_ear.loc_trans(-0.3,0,0,0);
                pig_right_tale.loc_trans(-0.3,0,0,0);
                pig_right_eye.loc_trans(-0.3,0,0,0);
                pig_right_nose.loc_trans(-0.3,0,0,0);
                pig_right_leg2.loc_trans(-0.3,0,0,0);
                pig_right_leg1.loc_trans(-0.3,0,0,0);
                break;
            case GLFW_KEY_V:
                pig_right_body.loc_trans(0.3,0,0,0);
                pig_right_ear.loc_trans(0.3,0,0,0);
                pig_right_tale.loc_trans(0.3,0,0,0);
                pig_right_eye.loc_trans(0.3,0,0,0);
                pig_right_nose.loc_trans(0.3,0,0,0);
                pig_right_leg2.loc_trans(0.3,0,0,0);
                pig_right_leg1.loc_trans(0.3,0,0,0);
                break;
            case GLFW_KEY_N:
                if(gamelaps>=2)
                    gamelaps = gamelaps-1;
                else
                    gamelaps = 1;
                break;
            case GLFW_KEY_M:
                gamelaps = gamelaps+1;
                break;
            case GLFW_KEY_LEFT:
                pan = pan + 0.1;
                reshapeWindow(window,800,800);
                break;
            case GLFW_KEY_RIGHT:
                pan = pan -0.1 ;
                reshapeWindow(window,800,800);
                break;
            case GLFW_KEY_UP:
                zoom = zoom + 0.1;
                reshapeWindow(window,800,800);
                break;
            case GLFW_KEY_DOWN:
                zoom = zoom -0.1;
                reshapeWindow(window,800,800);
                break;
            default:
                break;
        }
    }
    else if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                quit(window);
                break;
            default:
                break;
        }
    }
}
void numberdraw(int number){
    int num = number;
    float x=0;
    glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
    glm::vec3 target (0, 0, 0);
    glm::vec3 up (0, 1, 0);
    Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
    glm::mat4 VP = Matrices.projection * Matrices.view;
    glm::mat4 MVP;
    GLfloat color_buffer_data []={
        0,0,0,
        0,0,0,
        0,0,0,
        0,0,0,
        0,0,0,
        0,0,0
    };
    while(num>0)
    {
        switch(num%10)
        {
            case 1:
                Matrices.model = glm::mat4(1.0f);
                Matrices.model *= glm::translate(glm::vec3(3.5+x,3.5,0));
                MVP = VP * Matrices.model;
                glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitrighttopbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitrightbotbar, color_buffer_data, GL_FILL));

                break;
            case 2:
                Matrices.model = glm::mat4(1.0f);
                Matrices.model *= glm::translate(glm::vec3(3.5+x,3.5,0));
                MVP = VP * Matrices.model;
                glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitopbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitbotbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitmidbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitleftbotbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitrighttopbar, color_buffer_data, GL_FILL));
                break;
            case 3:
                Matrices.model = glm::mat4(1.0f);
                Matrices.model *= glm::translate(glm::vec3(3.5+x,3.5,0));
                MVP = VP * Matrices.model;
                glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitopbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitbotbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitmidbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitrightbotbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitrighttopbar, color_buffer_data, GL_FILL));
                break;
            case 4:
                Matrices.model = glm::mat4(1.0f);
                Matrices.model *= glm::translate(glm::vec3(3.5+x,3.5,0));
                MVP = VP * Matrices.model;
                glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitrighttopbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitrightbotbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitmidbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitlefttopbar, color_buffer_data, GL_FILL));
            case 5:
                Matrices.model = glm::mat4(1.0f);
                Matrices.model *= glm::translate(glm::vec3(3.5+x,3.5,0));
                MVP = VP * Matrices.model;
                glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitopbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitbotbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitmidbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitrightbotbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitlefttopbar, color_buffer_data, GL_FILL));
                break;
            case 6:
                Matrices.model = glm::mat4(1.0f);
                Matrices.model *= glm::translate(glm::vec3(3.5+x,3.5,0));
                MVP = VP * Matrices.model;
                glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitopbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitbotbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitmidbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitrightbotbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitlefttopbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitleftbotbar, color_buffer_data, GL_FILL));
                break;
            case 7:
                Matrices.model = glm::mat4(1.0f);
                Matrices.model *= glm::translate(glm::vec3(3.5+x+x,3.5,0));
                MVP = VP * Matrices.model;
                glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitrighttopbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitrightbotbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitopbar, color_buffer_data, GL_FILL));
                break;
            case 8:
                Matrices.model = glm::mat4(1.0f);
                Matrices.model *= glm::translate(glm::vec3(3.5+x,3.5,0));
                MVP = VP * Matrices.model;
                glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitopbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitbotbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitmidbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitrightbotbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitlefttopbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitleftbotbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitrighttopbar, color_buffer_data, GL_FILL));
                break;
            case 9:
                Matrices.model = glm::mat4(1.0f);
                Matrices.model *= glm::translate(glm::vec3(3.5+x,3.5,0));
                MVP = VP * Matrices.model;
                glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitopbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitmidbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitrightbotbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitlefttopbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitrighttopbar, color_buffer_data, GL_FILL));
                break;
            case 0:
                Matrices.model = glm::mat4(1.0f);
                Matrices.model *= glm::translate(glm::vec3(3.5+x,3.5,0));
                MVP = VP * Matrices.model;
                glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitopbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitbotbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitrightbotbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitlefttopbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitleftbotbar, color_buffer_data, GL_FILL));
                draw3DObject(create3DObject(GL_TRIANGLES, 6 , digitrighttopbar, color_buffer_data, GL_FILL));
                break;
            default:
                break;
        }
        num = num/10;
        x = x - 0.2;
    }
}

void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
            quit(window);
            break;
		default:
			break;
	}
}
void mouseScroll(GLFWwindow* window, double xoffset, double yoffset){
  if(yoffset == 1){
      zoom+=0.1;
    reshapeWindow(window, 800,800);
  }
  if(yoffset == -1){
      zoom-=0.1;
    reshapeWindow(window, 800,800);
  }
}
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            if (action == GLFW_PRESS)
            {
                double a,b;
                glfwGetCursorPos(window,&a,&b);
                double cur_x=(a-400)/100 ,cur_y= -(b-400)/100;

                canon_selected=0;
                pig_left_selected=0;
                pig_right_selected=0;
                if(dis(cur_x,cur_y,canon_body.x,canon_body.y) < 0.5)
                {
                    canon_selected=1;
                }
                else if(dis(cur_x,cur_y,pig_left_body.x,pig_left_body.y)<0.5)
                {
                    pig_left_selected=1;
                }
                else if(dis(cur_x,cur_y,pig_right_body.x,pig_right_body.y)<0.5)
                {
                    pig_right_selected=1;
                }
            }
            else if (action == GLFW_RELEASE)
            {
                double a,b;
                glfwGetCursorPos(window,&a,&b);
                double cur_x=(a-400)/100 ,cur_y= -(b-400)/100;

                canon_selected=0;
                pig_left_selected=0;
                pig_right_selected=0;
                canon_angle = atan((cur_y-canon_body.y)/(cur_x-canon_body.x))*180/M_PI;
            }
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_PRESS){
                double a,b;
                glfwGetCursorPos(window,&a,&b);
                double cur_x=(a-400)/100 ,cur_y= -(b-400)/100;
                pan_selected =1;
                pan_mous_loc_x=cur_x;
                pan_mous_loc_y=cur_y;
                }
            else if (action == GLFW_RELEASE){
                pan_selected = 0;
            }
            break;
        default:
            break;
    }
}
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window;

    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
    {
//        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
        glfwTerminate();
//        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);
    glfwSetWindowCloseCallback(window, quit);
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling
    glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks
    glfwSetScrollCallback(window,mouseScroll);
    return window;
}

int timerCount = 0;
int bricks_body_color[NO_RANDOM_BRICKS];
int main (int argc, char** argv)
{

	int width = 800;
	int height = 800;
    int TotalScore =0;
    GLFWwindow* window = initGLFW(width, height);

    //canon
    canon_body.createRegularPolygon(0.15,8,steel);
    canon_body.loc_set(-3.75,0,0);
    canon_hindge.createRegularPolygon(0.2,3,steel);
    canon_hindge.loc_set(-3.92,0,0);
    canon_piston = createRectangle(0.3,0.05);
    laser.createRegularPolygon(0.05,10,red);
    laser.loc_set(-3.75,0,0);

    // left pig
    pig_left_body.createRegularPolygon(0.5,20,broungold);
    pig_left_body.loc_set(-2,-3.54,0);
    pig_left_body.ang_set(-40,1,0,0);
    pig_left_ear.createRegularPolygon(0.2,3,broungold);
    pig_left_ear.loc_set(-2.3,-3.24,0);
    pig_left_leg1.createRegularPolygon(0.15,4,broungold);
    pig_left_leg1.loc_set(-1.7,-3.89,0);
    pig_left_leg1.ang_set(-30,0,0,1);
    pig_left_leg2.createRegularPolygon(0.15,4,broungold);
    pig_left_leg2.loc_set(-2.3,-3.89,0);
    pig_left_leg2.ang_set(30,0,0,1);
    pig_left_tale.createRegularPolygon(0.08,3,broungold);
    pig_left_tale.loc_set(-1.7,-3.2,0);
    pig_left_tale.ang_set(60,0,0,1);
    pig_left_nose.createRegularPolygon(0.15,4,broungold);
    pig_left_nose.loc_set(-2.5,-3.49,0);
    pig_left_nose.ang_set(50,0,0,1);
    pig_left_eye.createRegularPolygon(0.06,8,white);
    pig_left_eye.loc_set(-2.3,-3.44,0);

    //right pig
    pig_right_body.createRegularPolygon(0.5,20,coingold);
    pig_right_body.loc_set(2,-3.54,0);
    pig_right_body.ang_set(-40,1,0,0);
    pig_right_ear.createRegularPolygon(0.2,3,coingold);
    pig_right_ear.loc_set(pig_right_body.x-0.3,pig_right_body.y+0.3,pig_right_body.z);
    pig_right_leg1.createRegularPolygon(0.15,4,coingold);
    pig_right_leg1.loc_set(pig_right_body.x+0.3,pig_right_body.y-0.35,pig_right_body.z);
    pig_right_leg1.ang_set(-30,0,0,1);
    pig_right_leg2.createRegularPolygon(0.15,4,coingold);
    pig_right_leg2.loc_set(pig_right_body.x-0.3,pig_right_body.y-0.35,pig_right_body.z);
    pig_right_leg2.ang_set(30,0,0,1);
    pig_right_tale.createRegularPolygon(0.08,3,coingold);
    pig_right_tale.loc_set(pig_right_body.x+0.3,pig_right_body.y+.35,pig_right_body.z);
    pig_right_tale.ang_set(60,0,0,1);
    pig_right_nose.createRegularPolygon(0.15,4,coingold);
    pig_right_nose.loc_set(pig_right_body.x-0.5,pig_right_body.y+0.05,pig_right_body.z);
    pig_right_nose.ang_set(50,0,0,1);
    pig_right_eye.createRegularPolygon(0.06,8,white);
    pig_right_eye.loc_set(pig_right_body.x-0.3,pig_right_body.y+0.1,pig_right_body.z);

    //brick
    COLOR col[3]={black,broungold,coingold};
    for(int i=0;i<NO_RANDOM_BRICKS;i++){
        float a = rand()%5-2;
        float b = rand()%8-4;
        int c = rand()%7;
        int d;
        if(c%5==0){
            d=0;//give black
        }
        else{
            d =rand()%2+1;
        }
        bricks_body[i].createRegularPolygon(0.1,6,col[d]);
        bricks_body_color[i]=d;
        bricks_body[i].loc_set(a,b,0);
    }
    //mirror
    mirror1.createRegularPolygon();
    mirror2.createRegularPolygon();
    mirror3.createRegularPolygon();
    mirror4.createRegularPolygon();
    laser_angle = canon_angle;
    initGL (window, width, height);
    double last_update_time = glfwGetTime(), current_time;
    int mirrorhit=0;
    while (!glfwWindowShouldClose(window)) {
        double a,b;
        glfwGetCursorPos(window,&a,&b);
        double cur_x=(a-400)/100 ,cur_y= -(b-400)/100;
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        draw();
        laser.draw();
        canon_body.draw();
        canon_hindge.draw();

        numberdraw(TotalScore);

        pig_left_body.draw();
        pig_left_ear.draw();
        pig_left_leg1.draw();
        pig_left_leg2.draw();
        pig_left_tale.draw();
        pig_left_nose.draw();
        pig_left_eye.draw();

        pig_right_body.draw();
        pig_right_ear.draw();
        pig_right_leg1.draw();
        pig_right_leg2.draw();
        pig_right_tale.draw();
        pig_right_nose.draw();
        pig_right_eye.draw();
        for(int i=0;i<NO_RANDOM_BRICKS;i++){
            bricks_body[i].draw();
        }

        if(canon_selected==1)
        {
            canon_body.loc_trans(0,-canon_body.y+cur_y,0,0);
            laser.loc_trans(0,-laser.y+cur_y,0,0);
            canon_hindge.loc_trans(0,-canon_hindge.y+cur_y,0,0);
        }
        else if(pig_left_selected==1)
        {
            float a=-pig_left_body.x+cur_x;
            pig_left_body.loc_trans(a,0,0,0);
            pig_left_ear.loc_trans(a,0,0,0);
            pig_left_eye.loc_trans(a,0,0,0);
            pig_left_nose.loc_trans(a,0,0,0);
            pig_left_tale.loc_trans(a,0,0,0);
            pig_left_leg1.loc_trans(a,0,0,0);
            pig_left_leg2.loc_trans(a,0,0,0);
        }
        else if(pig_right_selected==1)
        {
            float a= -pig_right_body.x+cur_x;
            pig_right_body.loc_trans(a,0,0,0);
            pig_right_ear.loc_trans(a,0,0,0);
            pig_right_tale.loc_trans(a,0,0,0);
            pig_right_eye.loc_trans(a,0,0,0);
            pig_right_nose.loc_trans(a,0,0,0);
            pig_right_leg2.loc_trans(a,0,0,0);
            pig_right_leg1.loc_trans(a,0,0,0);
        }
        if (pan_selected==1)
        {
            pan =-cur_x+pan_mous_loc_x;
            reshapeWindow(window,800,800);
        }

        mirror1.draw();
        mirror2.draw();
        mirror3.draw();
        mirror4.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
        //speed and timer laocation od compntes
        current_time = glfwGetTime(); // Time in seconds
        int bricks_laps = 5,laser_laps = 1;
        if ((current_time - last_update_time) >= gamelaps){
            last_update_time = current_time;
        }
        if(spaceclicked==1 && timerCount%(laser_laps*gamelaps)==0){

            laser.loc_trans(0.05*cos(laser_angle*M_PI/180.0f),0.05*sin(laser_angle*M_PI/180.0f),0,0);
        }
        if(timerCount%(10*gamelaps)){
            mirrorhit = 1;
        }
        if(timerCount%(bricks_laps*gamelaps)==0){
            for(int i=0;i<NO_RANDOM_BRICKS;i++){
                bricks_body[i].loc_trans(0,-0.05,0,0);
            }
        }
        timerCount=timerCount+5;
        if(laser.x > 4 ||laser.y >4 || laser.y < -4){
            laser.loc_set(canon_body.x,canon_body.y,canon_body.z);
            laser_angle = canon_angle;
            TotalScore = TotalScore -1;
            spaceclicked = 0;
        }
        for(int i=0;i<NO_RANDOM_BRICKS;i++){
            if(bricks_body[i].y < -4 ){
                float a = rand()%5-2;
                float b = rand()%8+4;
                bricks_body[i].loc_set(a,b,0);
            }
            if(dis(laser.x,laser.y,bricks_body[i].x,bricks_body[i].y)<=0.05){
                if(bricks_body_color[i]==0)
                {
                    TotalScore = TotalScore +5;
                    std::cout << "hit black add 10" <<TotalScore<< std::endl;
                }
                else
                {
                    TotalScore = TotalScore -2;
                    std::cout << "hit other sub 2" <<TotalScore<< std::endl;
                }

                float a = rand()%5-2;
                float b = rand()%8+4;
                bricks_body[i].loc_set(a,b,0);
                laser.loc_set(canon_body.x,canon_body.y,canon_body.z);
                laser_angle = canon_angle;
                spaceclicked = 0;
            }
            if(dis(pig_left_body.x,pig_left_body.y,bricks_body[i].x,bricks_body[i].y) <= 0.1)
            {
                if(bricks_body_color[i]==1)
                {
                    TotalScore = TotalScore +3;
                    std::cout << "cot broun gold add 3" << std::endl;
                }
                else
                {
                    TotalScore = TotalScore -1;
                    std::cout << "cot another "<< TotalScore << std::endl;
                }
                float a = rand()%5-2;
                float b = rand()%8+4;
                bricks_body[i].loc_set(a,b,0);
            }
            if(dis(pig_right_body.x,pig_right_body.y,bricks_body[i].x,bricks_body[i].y) <= 0.1)
            {
                if(bricks_body_color[i]==2)
                {
                    TotalScore = TotalScore +5;
                    std::cout << "cot white gold add 5" << std::endl;
                }
                else
                {
                    TotalScore = TotalScore -1;
                    std::cout << "cot another "<< TotalScore << std::endl;
                }
                float a = rand()%5-2;
                float b = rand()%8+4;
                bricks_body[i].loc_set(a,b,0);
            }
        }
        if(mirrTouch(laser.x,laser.y,mirror1.x,mirror1.y,mirror1.rotation_angle)==1 && dis(laser.x,laser.y,mirror1.x,mirror1.y)<=0.5 && mirrorhit==1){
             laser_angle=2*mirror1.rotation_angle-laser_angle;
             mirrorhit =0;
        }
        else if(mirrTouch(laser.x,laser.y,mirror2.x,mirror2.y,mirror2.rotation_angle)==1 && dis(laser.x,laser.y,mirror2.x,mirror2.y)<=0.5 &&mirrorhit==1){
             laser_angle=2*mirror2.rotation_angle-laser_angle;
             mirrorhit = 0;
        }
        else if(mirrTouch(laser.x,laser.y,mirror3.x,mirror3.y,mirror3.rotation_angle)==1 && dis(laser.x,laser.y,mirror3.x,mirror3.y)<=0.5 && mirrorhit==1){
             laser_angle=2*mirror3.rotation_angle-laser_angle;
             mirrorhit = 0;
        }
        else if(mirrTouch(laser.x,laser.y,mirror4.x,mirror4.y,mirror4.rotation_angle)==1 && dis(laser.x,laser.y,mirror4.x,mirror4.y)<=0.5 && mirrorhit==1){
             laser_angle=2*mirror4.rotation_angle-laser_angle;
             mirrorhit = 0;
        }
    }
    glfwTerminate();
}
