#include "../Include/Common.h"

//For GLUT to handle 
#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3
#define MENU_ORGIN 4
#define MENU_PURE_COLOR 5


//GLUT timer variable
GLubyte timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;

using namespace glm;
using namespace std;

mat4 view;					// V of MVP, viewing matrix
mat4 projection;			// P of MVP, projection matrix
mat4 model;					// M of MVP, model matrix
vec3 temp = vec3();			// a 3 dimension vector which represents how far did the ladybug should move
//-------mycode-----
vec3 mouse_old= vec3();
mat4 mouse_rotation = mat4();
//-------mycode-----
GLint um4p;	
GLint um4mv;

GLuint program;			// shader program id
GLuint program_pure_color;

typedef struct
{
	GLuint vao;			// vertex array object
	GLuint vbo;			// vertex buffer object
	GLuint vboTex;		// vertex buffer object of texture
	GLuint ebo;			

	GLuint p_normal;
	int materialId;
	int indexCount;
	GLuint m_texture;
} Shape;

Shape m_shpae[4];

// color 
char* blue = "blue.png";
char* red = "red.png";
char* yellow = "yellow.png";
char* green = "green.png";

inline GLfloat range(GLfloat upper, GLfloat lower, GLfloat x) {
	upper = deg2rad(upper);
	lower = deg2rad(lower);
	if (x > upper) {
		return upper;
	}
	else if (x < lower) {
		return lower;
	}
	return x;
}

// Load shader file to program
char** loadShaderSource(const char* file)
{
    FILE* fp = fopen(file, "rb");
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *src = new char[sz + 1];
    fread(src, sizeof(char), sz, fp);
    src[sz] = '\0';
    char **srcp = new char*[1];
    srcp[0] = src;
    return srcp;
}

// Free shader file
void freeShaderSource(char** srcp)
{
    delete srcp[0];
    delete srcp;
}

// Load .obj model
void My_LoadModels(char* objName, int k, char* color)
{
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err;

	bool ret = tinyobj::LoadObj(shapes, materials, err, objName);
	if(err.size()>0)
	{
		printf("Load Models Fail! Please check the solution path");
		return;
	}
	
	printf("Load Models Success ! Shapes size %d Maerial size %d\n", shapes.size(), materials.size());

	for(int i = 0; i < shapes.size(); i++)
	{
		glGenVertexArrays(1, &m_shpae[k].vao);
		glBindVertexArray(m_shpae[k].vao);

		glGenBuffers(3, &m_shpae[k].vbo);
		glGenBuffers(1,&m_shpae[k].p_normal);
		glBindBuffer(GL_ARRAY_BUFFER, m_shpae[k].vbo);
		glBufferData(GL_ARRAY_BUFFER, shapes[i].mesh.positions.size() * sizeof(float)+shapes[i].mesh.normals.size() * sizeof(float), NULL, GL_STATIC_DRAW);

		glBufferSubData(GL_ARRAY_BUFFER, 0, shapes[i].mesh.positions.size() * sizeof(float), &shapes[i].mesh.positions[0]);
		glBufferSubData(GL_ARRAY_BUFFER, shapes[i].mesh.positions.size() * sizeof(float), shapes[i].mesh.normals.size() * sizeof(float), &shapes[i].mesh.normals[0]);
		
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void *) (shapes[i].mesh.positions.size() * sizeof(float)));

		glBindBuffer(GL_ARRAY_BUFFER, m_shpae[k].p_normal);
		glBufferData(GL_ARRAY_BUFFER, shapes[i].mesh.normals.size() * sizeof(float), shapes[i].mesh.normals.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, m_shpae[k].vboTex);
		glBufferData(GL_ARRAY_BUFFER, shapes[i].mesh.texcoords.size() * sizeof(float), shapes[i].mesh.texcoords.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_shpae[k].ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapes[i].mesh.indices.size() * sizeof(unsigned int), shapes[i].mesh.indices.data(), GL_STATIC_DRAW);
		m_shpae[k].materialId = shapes[i].mesh.material_ids[0];
		m_shpae[k].indexCount = shapes[i].mesh.indices.size();


		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
	}

	texture_data tdata = load_png(color);

	glGenTextures( 1, &m_shpae[k].m_texture );
	glBindTexture( GL_TEXTURE_2D, m_shpae[k].m_texture);


	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
}

// OpenGL initialization
void My_Init()
{
	
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);


	// Create Shader Program
    program = glCreateProgram();
	
	
	// Create customize shader by tell openGL specify shader type 
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	// Load shader file
	char** vertexShaderSource = loadShaderSource("vertex.vs.glsl");
	char** fragmentShaderSource = loadShaderSource("fragment.fs.glsl");

	// Assign content of these shader files to those shaders we created before
    glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
    glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);

	// Free the shader file string(won't be used any more)
	//freeShaderSource(vertexShaderSource);
	freeShaderSource(fragmentShaderSource);

	// Compile these shaders
    glCompileShader(vertexShader);
    glCompileShader(fragmentShader);

	// Logging
	shaderLog(vertexShader);
    shaderLog(fragmentShader);
	
	// Assign the program we created before with these shaders
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

	// Get the id of inner variable 'um4p' and 'um4mv' in shader programs
	um4p = glGetUniformLocation(program, "um4p");
    um4mv = glGetUniformLocation(program, "um4mv");

	glUseProgram(program);
	
	//----------------mycode-----------------
	// Create Shader Program
	program_pure_color = glCreateProgram();

	// Create customize shader by tell openGL specify shader type 
	//GLint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader2 = glCreateShader(GL_FRAGMENT_SHADER);

	// Load shader file
	//char** vertexShaderSource = loadShaderSource("vertex.vs.glsl");
	char** fragmentShaderSource2 = loadShaderSource("fragment2.fs.glsl");

	// Assign content of these shader files to those shaders we created before
	//glShaderSource(vertexShader2, 1, vertexShaderSource, NULL);
	glShaderSource(fragmentShader2, 1, fragmentShaderSource2, NULL);

	// Free the shader file string(won't be used any more)
	freeShaderSource(vertexShaderSource);
	freeShaderSource(fragmentShaderSource2);

	// Compile these shaders
	glCompileShader(vertexShader);
	glCompileShader(fragmentShader2);

	// Logging
	shaderLog(vertexShader);
	shaderLog(fragmentShader2);

	// Assign the program we created before with these shaders
	glAttachShader(program_pure_color, vertexShader);
	glAttachShader(program_pure_color, fragmentShader2);
	glLinkProgram(program_pure_color);

	// Get the id of inner variable 'um4p' and 'um4mv' in shader programs
	um4p = glGetUniformLocation(program_pure_color, "um4p");  
	um4mv = glGetUniformLocation(program_pure_color, "um4mv");
	glUseProgram(program);


	//----------------mycode------------------
	
	// Tell OpenGL to use this shader program now
    //glUseProgram(program);

	My_LoadModels("Cube.obj",0 ,blue);
	My_LoadModels("Sphere.obj" ,1 ,yellow);
	My_LoadModels("Capsule.obj", 2, red);
	My_LoadModels("Cylinder.obj", 3, green);
}

// GLUT callback. Called to draw the scene.
void My_Display()
{
	// Clear display buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Bind a vertex array for OpenGL (OpenGL will apply operation only to the vertex array objects it bind)
	glBindVertexArray(m_shpae[0].vao);

	// Tell openGL to use the shader program we created before
	//glUseProgram(program);


	// Todo
	// Practice 2 : Build 2 matrix (translation matrix and rotation matrix)
	// then multiply these matrix with proper order
	// final result should restore in the variable 'model'

	//Build translation matrix
	mat4 translation_matrix = translate(mat4(), temp + vec3(0.0, 2.0 ,0.0));
	//Build rotation matrix
	GLfloat degree = glutGet(GLUT_ELAPSED_TIME) / 500.0;
	
	vec3 rotate_axis = vec3(0.0, 1.0, 0.0);
	mat4 rotation_matrix = rotate(mat4(), degree, rotate_axis);
	
	model = translation_matrix * rotation_matrix;
	//model = translation_matrix;
	
	// Transfer value of (view*model) to both shader's inner variable 'um4mv'; 
    glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * model));
	
	// Transfer value of projection to both shader's inner variable 'um4p';
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	// mycode
	glBindTexture(GL_TEXTURE_2D, m_shpae[0].m_texture); //加上這行可替photo塗上不同的顏色
	// Tell openGL to draw the vertex array we had binded before
	glDrawElementsInstanced(GL_TRIANGLES, m_shpae[0].indexCount, GL_UNSIGNED_INT, 0 ,2);

	//-------------mycode-------------
	GLint my_speed = glutGet(GLUT_ELAPSED_TIME);
	// Head
	mat4 Head_Model = translate(model,vec3(0.0, 1.0, 0.0));

	glBindTexture(GL_TEXTURE_2D, m_shpae[1].m_texture);  //加上這行可替photo塗上不同的顏色
	
	// Transfer value of (view*model) to both shader's inner variable 'um4mv'; 
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * Head_Model));

	// Transfer value of projection to both shader's inner variable 'um4p';
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));
	
	glBindVertexArray(m_shpae[1].vao);
	// Tell openGL to draw the vertex array we had binded before
	glDrawElements(GL_TRIANGLES, m_shpae[1].indexCount, GL_UNSIGNED_INT, 0);


	// ------------------------------------
	// R1a
	mat4 R1a_Model_after_translate = translate(model, vec3(0.0, 0.0, 1.0));
	mat4 R1a_Model = rotate(R1a_Model_after_translate, deg2rad((my_speed / 10 + 45) % 60 + 45), vec3(0.0, 0.0, -1.0));

	glBindTexture(GL_TEXTURE_2D, m_shpae[2].m_texture);  //加上這行可替photo塗上不同的顏色

	// Transfer value of (view*model) to both shader's inner variable 'um4mv'; 
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * R1a_Model));

	// Transfer value of projection to both shader's inner variable 'um4p';
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));

	glBindVertexArray(m_shpae[2].vao);
	// Tell openGL to draw the vertex array we had binded before
	glDrawElements(GL_TRIANGLES, m_shpae[2].indexCount, GL_UNSIGNED_INT, 0);

	//---------------------------------------
	//R2a 上臂的相對座標
	mat4 R2a_Model_after_translate = translate(R1a_Model, vec3(0.0, 1.5, 0.0));
	mat4 R2a_Model = rotate(R2a_Model_after_translate, deg2rad(0), vec3(0.0, 0.0, 1.0));

	glBindTexture(GL_TEXTURE_2D, m_shpae[3].m_texture);  //加上這行可替photo塗上不同的顏色

	// Transfer value of (view*model) to both shader's inner variable 'um4mv'; 
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * R2a_Model));

	// Transfer value of projection to both shader's inner variable 'um4p';
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));

	glBindVertexArray(m_shpae[3].vao);
	// Tell openGL to draw the vertex array we had binded before
	glDrawElements(GL_TRIANGLES, m_shpae[3].indexCount, GL_UNSIGNED_INT, 0);

	// ------------------------
	// L1a 有黃色的那隻手
	mat4 L1a_Model_after_translate = translate(model, vec3(0.0, 0.0, -1.0));
	GLfloat left_arm_ratation_speed = (glutGet(GLUT_ELAPSED_TIME) / 10 ) % 60 + 45;  //如此一來可以限制活動範圍，轉的過去但會突兀地回到原點
	mat4 L1a_Model = rotate(L1a_Model_after_translate, deg2rad(left_arm_ratation_speed), vec3(0.0, 0.0, -1.0)); //problem
	

	glBindTexture(GL_TEXTURE_2D, m_shpae[2].m_texture);  //加上這行可替photo塗上不同的顏色

	// Transfer value of (view*model) to both shader's inner variable 'um4mv';
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * L1a_Model));

	// Transfer value of projection to both shader's inner variable 'um4p';
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));

	glBindVertexArray(m_shpae[2].vao);
	// Tell openGL to draw the vertex array we had binded before
	glDrawElements(GL_TRIANGLES, m_shpae[2].indexCount, GL_UNSIGNED_INT, 0);

	//--------------------------------
	//L2a
	mat4 L2a_Model_after_translate = translate(L1a_Model, vec3(0.0, 1.5, 0.0));
	mat4 L2a_Model = rotate(L2a_Model_after_translate, deg2rad(0), vec3(0.0, 0.0, 1.0));
	// 現在黃色是左upper arm
	glBindTexture(GL_TEXTURE_2D, m_shpae[1].m_texture);  //加上這行可替photo塗上不同的顏色

	// Transfer value of (view*model) to both shader's inner variable 'um4mv'; 
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * L2a_Model));

	// Transfer value of projection to both shader's inner variable 'um4p';
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));

	glBindVertexArray(m_shpae[3].vao);
	// Tell openGL to draw the vertex array we had binded before
	glDrawElements(GL_TRIANGLES, m_shpae[3].indexCount, GL_UNSIGNED_INT, 0);

	//-------------------------------------
	//R1L
	mat4 R1L_Model_after_translate = translate(model, vec3(0.0, -2.5, 0.5));
	mat4 R1L_Model = rotate(R1L_Model_after_translate, deg2rad(0), vec3(0.0, 0.0, -1.0));
	
	glBindTexture(GL_TEXTURE_2D, m_shpae[2].m_texture);  //加上這行可替photo塗上不同的顏色

	// Transfer value of (view*model) to both shader's inner variable 'um4mv'; 
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * R1L_Model));

	// Transfer value of projection to both shader's inner variable 'um4p';
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));

	glBindVertexArray(m_shpae[2].vao);
	// Tell openGL to draw the vertex array we had binded before
	glDrawElements(GL_TRIANGLES, m_shpae[2].indexCount, GL_UNSIGNED_INT, 0);

	//-------------------------------------
	//L1L
	bool L1L_flag = FALSE;
	mat4 L1L_Model_after_translate = translate(model, vec3(0.0, -2.5, -0.5));
	mat4 L1L_Model = rotate(L1L_Model_after_translate, deg2rad((my_speed + 60) / 10 % 20 + 20), vec3(0.0, 0.0, -1.0));
	// 沒有作用
	/*if (L1L_flag) {
		L1L_Model = rotate(L1L_Model_after_translate, deg2rad((my_speed + 60) / 10 % 20 + 20), vec3(0.0, 0.0, -1.0));
		L1L_flag = FALSE;
	}
	else {
		L1L_Model = rotate(L1L_Model_after_translate, deg2rad((my_speed + 60) / 10 % 20 + 20), vec3(0.0, 0.0, 1.0));
		L1L_flag = TRUE;
	}*/

	glBindTexture(GL_TEXTURE_2D, m_shpae[2].m_texture);  //加上這行可替photo塗上不同的顏色

	// Transfer value of (view*model) to both shader's inner variable 'um4mv'; 
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * L1L_Model));

	// Transfer value of projection to both shader's inner variable 'um4p';
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));

	glBindVertexArray(m_shpae[2].vao);
	// Tell openGL to draw the vertex array we had binded before
	glDrawElements(GL_TRIANGLES, m_shpae[2].indexCount, GL_UNSIGNED_INT, 0);

	//------------------------------------------
	// L2L
	mat4 L2L_Model_after_translate = translate(L1L_Model, vec3(0.0, -1.0, 0.0));
	mat4 L2L_Model = rotate(L2L_Model_after_translate, deg2rad(0), vec3(0.0, 0.0, 1.0));

	glBindTexture(GL_TEXTURE_2D, m_shpae[3].m_texture);  //加上這行可替photo塗上不同的顏色

	// Transfer value of (view*model) to both shader's inner variable 'um4mv'; 
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * L2L_Model));

	// Transfer value of projection to both shader's inner variable 'um4p';
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));

	glBindVertexArray(m_shpae[3].vao);
	// Tell openGL to draw the vertex array we had binded before
	glDrawElements(GL_TRIANGLES, m_shpae[3].indexCount, GL_UNSIGNED_INT, 0);

	//-------------------------------
	//L2R
	mat4 L2R_Model_after_translate = translate(R1L_Model, vec3(0.0, -1.0, 0.0));
	mat4 L2R_Model = rotate(L2R_Model_after_translate, deg2rad(0), vec3(0.0, 0.0, 1.0));

	glBindTexture(GL_TEXTURE_2D, m_shpae[3].m_texture);  //加上這行可替photo塗上不同的顏色

	// Transfer value of (view*model) to both shader's inner variable 'um4mv'; 
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(view * L2R_Model));

	// Transfer value of projection to both shader's inner variable 'um4p';
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(projection));

	glBindVertexArray(m_shpae[3].vao);
	// Tell openGL to draw the vertex array we had binded before
	glDrawElements(GL_TRIANGLES, m_shpae[3].indexCount, GL_UNSIGNED_INT, 0);

	//-------------mycode-------------
	
	// Change current display buffer to another one (refresh frame) 
    glutSwapBuffers();
}

// Setting up viewing matrix
void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);

	float viewportAspect = (float)width / (float)height;

	// perspective(fov, aspect_ratio, near_plane_distance, far_plane_distance)
	// ps. fov = field of view, it represent how much range(degree) is this camera could see 
	projection = perspective(radians(60.0f), viewportAspect, 0.1f, 1000.0f);

	// lookAt(camera_position, camera_viewing_vector, up_vector)
	// up_vector represent the vector which define the direction of 'up'
	view = lookAt(vec3(-10.0f, 5.0f, 0.0f), vec3(1.0f, 1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
}

void My_Timer(int val)
{
	timer_cnt++;
	glutPostRedisplay();
	if(timer_enabled)
	{
		glutTimerFunc(timer_speed, My_Timer, val);
	}
}

void My_Keyboard(unsigned char key, int x, int y)
{
	printf("Key %c is pressed at (%d, %d)\n", key, x, y);
	if (key == 'd')
	{
		temp = temp + vec3(0, 0, 1);
	}
	else if (key == 'a')
	{
		temp = temp - vec3(0, 0, 1); 
	}
	else if (key == 'w')
	{
		temp = temp + vec3(1, 0, 0);
	}
	else if (key == 's')
	{
		temp = temp - vec3(1, 0, 0);
	}
}

void My_SpecialKeys(int key, int x, int y)
{
	switch(key)
	{
	case GLUT_KEY_F1:
		printf("F1 is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_PAGE_UP:
		printf("Page up is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_LEFT:
		printf("Left arrow is pressed at (%d, %d)\n", x, y);
		break;
	default:
		printf("Other special key is pressed at (%d, %d)\n", x, y);
		break;
	}
}

void My_Menu(int id)
{
	switch(id)
	{
	case MENU_TIMER_START:
		if(!timer_enabled)
		{
			timer_enabled = true;
			glutTimerFunc(timer_speed, My_Timer, 0);
		}
		break;
	case MENU_TIMER_STOP:
		timer_enabled = false;
		break;
	case MENU_EXIT:
		exit(0);
		break;
	case MENU_ORGIN:
		glUseProgram(program);
		break;
	case MENU_PURE_COLOR:
		glUseProgram(program_pure_color);
		break;
	default:
		break;
	}
}

void My_Mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON) 
	{
		if (state == GLUT_DOWN)
		{
			printf("Mouse %d is pressed at (%d, %d)\n", button, x, y);
			mouse_old = vec3(2.0 * x / glutGet(GLUT_WINDOW_WIDTH) - 1, -2.0 * y / glutGet(GLUT_WINDOW_HEIGHT) + 1, 1);
		}
		else if (state == GLUT_UP)
		{
			printf("Mouse %d is released at (%d, %d)\n", button, x, y);
		}
	}
}


// 功能fail
void my_Motion(int x, int y) {
	
	//mouse_orgin = vec3(2.0 * x / glutGet(GLUT_WINDOW_WIDTH) - 1, -2.0 * y / glutGet(GLUT_WINDOW_HEIGHT) + 1, 1); // why?
	vec3 mouse_position = vec3(2.0 * x / glutGet(GLUT_WINDOW_WIDTH) - 1, -2.0 * y / glutGet(GLUT_WINDOW_HEIGHT) + 1, 1);
	vec3 axis = normalize(cross(mouse_old, mouse_position)); // cross得到旋轉軸
	float angleRad = acosf(clamp(dot(mouse_old, mouse_position), -1.0f, 1.0f));  // 由滑鼠座標得到旋轉的角度  這句話的作用?

	cout << angleRad << endl; // output is always zero
	// 越到後期得出的rotation會疊加
	mouse_rotation = rotate(mat4(), angleRad, axis);

	// 得出的degree會是0
	//mouse_old = mouse_position; 

	// 為什麼不會旋轉?
	model = mouse_rotation * model;
	glutPostRedisplay();
}


int main(int argc, char *argv[])
{
#ifdef __APPLE__
    // Change working directory to source code path
    chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(600, 600);
	glutCreateWindow("robot"); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
	dumpInfo();
	My_Init();
	// # renderdoc debug(fail)
	HDC    hdc;
	HGLRC  hglrc;

	// create a rendering context  
	hglrc = wglCreateContext(hdc);

	// make it the calling thread's current rendering context 
	wglMakeCurrent(hdc, hglrc);
	// # end render debug
	int menu_main = glutCreateMenu(My_Menu);
	int menu_timer = glutCreateMenu(My_Menu);
	int menu_new = glutCreateMenu(My_Menu);

	glutSetMenu(menu_main);
	glutAddSubMenu("Timer", menu_timer);
	glutAddSubMenu("Color", menu_new);
	glutAddMenuEntry("Exit", MENU_EXIT);

	glutSetMenu(menu_timer);
	glutAddMenuEntry("Start", MENU_TIMER_START);
	glutAddMenuEntry("Stop", MENU_TIMER_STOP);

	glutSetMenu(menu_new);						// Tell GLUT to design the menu which id==menu_new now
	glutAddMenuEntry("orgin", MENU_ORGIN);		// Add submenu "Hello" in "New"(a menu which index is equal to menu_new)
	glutAddMenuEntry("pure color", MENU_PURE_COLOR);		// Add submenu "Hello" in "New"(a menu which index is equal to menu_new)

	glutSetMenu(menu_main);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// Register GLUT callback functions.
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutKeyboardFunc(My_Keyboard);
	glutSpecialFunc(My_SpecialKeys);
	glutTimerFunc(timer_speed, My_Timer, 0);
	glutMotionFunc(my_Motion);
	// Todo
	// Practice 1 : Register new GLUT event listner here
	// ex. glutXXXXX(my_Func);
	// Remind : you have to implement my_Func
	glutMouseFunc(My_Mouse);

	// Enter main event loop.
	glutMainLoop();

	return 0;
}