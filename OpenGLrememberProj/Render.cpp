#include "Render.h"
#include <string>
#include <sstream>

#include <vector>
#include <windows.h>

#include <GL\gl.h>
#include <GL\glu.h>
#include "GL\glext.h"
#include "MyOGL.h"

#include "Camera.h"
#include "Light.h"
#include "Primitives.h"

#include "MyShaders.h"

#include "ObjLoader.h"
#include "GUItextRectangle.h"

#include "Texture.h"

GuiTextRectangle rec;

bool textureMode = true;
bool lightMode = true;


//небольшой дефайн для упрощения кода
#define POP glPopMatrix()
#define PUSH glPushMatrix()


ObjFile* model;

Texture texture1;
Texture sTex;
Texture rTex;
Texture tBox;

Shader s[10];  //массивчик для десяти шейдеров
Shader frac;
Shader cassini;

struct  Robot
{
	double pozx, pozy, ygl;
	Robot()
	{
		pozx = 0;
		pozy = 0;
		ygl = 0;
	}
};
//класс для настройки камеры
class CustomCamera : public Camera
{
public:
	//дистанция камеры
	double camDist;
	//углы поворота камеры
	double fi1, fi2;


	//значния масеры по умолчанию
	CustomCamera()
	{
		camDist = 15;
		fi1 = 1;
		fi2 = 1;
	}


	//считает позицию камеры, исходя из углов поворота, вызывается движком
	virtual void SetUpCamera()
	{

		lookPoint.setCoords(0, 0, 0);

		pos.setCoords(camDist * cos(fi2) * cos(fi1),
			camDist * cos(fi2) * sin(fi1),
			camDist * sin(fi2));

		if (cos(fi2) <= 0)
			normal.setCoords(0, 0, -1);
		else
			normal.setCoords(0, 0, 1);

		LookAt();
	}

	void CustomCamera::LookAt()
	{
		gluLookAt(pos.X(), pos.Y(), pos.Z(), lookPoint.X(), lookPoint.Y(), lookPoint.Z(), normal.X(), normal.Y(), normal.Z());
	}



}  camera;   //создаем объект камеры


//класс недоделан!
class WASDcamera :public CustomCamera
{
public:

	float camSpeed;

	WASDcamera()
	{
		camSpeed = 0.4;
		pos.setCoords(5, 5, 5);
		lookPoint.setCoords(0, 0, 0);
		normal.setCoords(0, 0, 1);
	}

	virtual void SetUpCamera()
	{

		if (OpenGL::isKeyPressed('W'))
		{
			Vector3 forward = (lookPoint - pos).normolize() * camSpeed;
			pos = pos + forward;
			lookPoint = lookPoint + forward;

		}
		if (OpenGL::isKeyPressed('S'))
		{
			Vector3 forward = (lookPoint - pos).normolize() * (-camSpeed);
			pos = pos + forward;
			lookPoint = lookPoint + forward;

		}

		LookAt();
	}

} WASDcam;


//Класс для настройки света
class CustomLight : public Light
{
public:
	CustomLight()
	{
		//начальная позиция света
		pos = Vector3(1, 1, 3);
	}


	//рисует сферу и линии под источником света, вызывается движком
	void  DrawLightGhismo()
	{

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		Shader::DontUseShaders();
		bool f1 = glIsEnabled(GL_LIGHTING);
		glDisable(GL_LIGHTING);
		bool f2 = glIsEnabled(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_2D);
		bool f3 = glIsEnabled(GL_DEPTH_TEST);

		glDisable(GL_DEPTH_TEST);
		glColor3d(0.9, 0.8, 0);
		Sphere s;
		s.pos = pos;
		s.scale = s.scale * 0.08;
		s.Show();

		if (OpenGL::isKeyPressed('G'))
		{
			glColor3d(0, 0, 0);
			//линия от источника света до окружности
			glBegin(GL_LINES);
			glVertex3d(pos.X(), pos.Y(), pos.Z());
			glVertex3d(pos.X(), pos.Y(), 0);
			glEnd();

			//рисуем окруность
			Circle c;
			c.pos.setCoords(pos.X(), pos.Y(), 0);
			c.scale = c.scale * 1.5;
			c.Show();
		}
		/*
		if (f1)
			glEnable(GL_LIGHTING);
		if (f2)
			glEnable(GL_TEXTURE_2D);
		if (f3)
			glEnable(GL_DEPTH_TEST);
			*/
	}

	void SetUpLight()
	{
		GLfloat amb[] = { 0.2, 0.2, 0.2, 0 };
		GLfloat dif[] = { 1.0, 1.0, 1.0, 0 };
		GLfloat spec[] = { .7, .7, .7, 0 };
		GLfloat position[] = { pos.X(), pos.Y(), pos.Z(), 1. };

		// параметры источника света
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		// характеристики излучаемого света
		// фоновое освещение (рассеянный свет)
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		// диффузная составляющая света
		glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
		// зеркально отражаемая составляющая света
		glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

		glEnable(GL_LIGHT0);
	}


} light;  //создаем источник света



//старые координаты мыши
int mouseX = 0, mouseY = 0;




float offsetX = 0, offsetY = 0;
float zoom = 1;
float Time = 0;
int tick_o = 0;
int tick_n = 0;

//обработчик движения мыши
void mouseEvent(OpenGL* ogl, int mX, int mY)
{
	int dx = mouseX - mX;
	int dy = mouseY - mY;
	mouseX = mX;
	mouseY = mY;

	//меняем углы камеры при нажатой левой кнопке мыши
	if (OpenGL::isKeyPressed(VK_RBUTTON))
	{
		camera.fi1 += 0.01 * dx;
		camera.fi2 += -0.01 * dy;
	}


	if (OpenGL::isKeyPressed(VK_LBUTTON))
	{
		offsetX -= 1.0 * dx / ogl->getWidth() / zoom;
		offsetY += 1.0 * dy / ogl->getHeight() / zoom;
	}



	//двигаем свет по плоскости, в точку где мышь
	if (OpenGL::isKeyPressed('G') && !OpenGL::isKeyPressed(VK_LBUTTON))
	{
		LPPOINT POINT = new tagPOINT();
		GetCursorPos(POINT);
		ScreenToClient(ogl->getHwnd(), POINT);
		POINT->y = ogl->getHeight() - POINT->y;

		Ray r = camera.getLookRay(POINT->x, POINT->y, 60, ogl->aspect);

		double z = light.pos.Z();

		double k = 0, x = 0, y = 0;
		if (r.direction.Z() == 0)
			k = 0;
		else
			k = (z - r.origin.Z()) / r.direction.Z();

		x = k * r.direction.X() + r.origin.X();
		y = k * r.direction.Y() + r.origin.Y();

		light.pos = Vector3(x, y, z);
	}

	if (OpenGL::isKeyPressed('G') && OpenGL::isKeyPressed(VK_LBUTTON))
	{
		light.pos = light.pos + Vector3(0, 0, 0.02 * dy);
	}


}

//обработчик вращения колеса  мыши
void mouseWheelEvent(OpenGL* ogl, int delta)
{


	float _tmpZ = delta * 0.003;
	if (ogl->isKeyPressed('Z'))
		_tmpZ *= 10;
	zoom += 0.2 * zoom * _tmpZ;


	if (delta < 0 && camera.camDist <= 1)
		return;
	if (delta > 0 && camera.camDist >= 100)
		return;

	camera.camDist += 0.01 * delta;
}

//обработчик нажатия кнопок клавиатуры
void keyDownEvent(OpenGL* ogl, int key)
{
	if (key == 'L')
	{
		lightMode = !lightMode;
	}

	if (key == 'T')
	{
		textureMode = !textureMode;
	}

	if (key == 'R')
	{
		camera.fi1 = 1;
		camera.fi2 = 1;
		camera.camDist = 15;

		light.pos = Vector3(1, 1, 3);
	}

	if (key == 'F')
	{
		light.pos = camera.pos;
	}

	if (key == 'S')
	{
		frac.LoadShaderFromFile();
		frac.Compile();

		s[0].LoadShaderFromFile();
		s[0].Compile();

		cassini.LoadShaderFromFile();
		cassini.Compile();
	}

	if (key == 'Q')
		Time = 0;
}

void keyUpEvent(OpenGL* ogl, int key)
{

}


void DrawQuad()
{
	double A[] = { 0,0 };
	double B[] = { 1,0 };
	double C[] = { 1,1 };
	double D[] = { 0,1 };
	glBegin(GL_QUADS);
	glColor3d(.5, 0, 0);
	glNormal3d(0, 0, 1);
	glTexCoord2d(0, 0);
	glVertex2dv(A);
	glTexCoord2d(1, 0);
	glVertex2dv(B);
	glTexCoord2d(1, 1);
	glVertex2dv(C);
	glTexCoord2d(0, 1);
	glVertex2dv(D);
	glEnd();
}


ObjFile objModel, monkey, M,T;

Texture monkeyTex, Tex1, T2, T3;

//выполняется перед первым рендером
void initRender(OpenGL* ogl)
{

	//настройка текстур

	//4 байта на хранение пикселя
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	//настройка режима наложения текстур
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//включаем текстуры
	glEnable(GL_TEXTURE_2D);




	//камеру и свет привязываем к "движку"
	ogl->mainCamera = &camera;
	//ogl->mainCamera = &WASDcam;
	ogl->mainLight = &light;

	// нормализация нормалей : их длины будет равна 1
	glEnable(GL_NORMALIZE);

	// устранение ступенчатости для линий
	glEnable(GL_LINE_SMOOTH);


	//   задать параметры освещения
	//  параметр GL_LIGHT_MODEL_TWO_SIDE - 
	//                0 -  лицевые и изнаночные рисуются одинаково(по умолчанию), 
	//                1 - лицевые и изнаночные обрабатываются разными режимами       
	//                соответственно лицевым и изнаночным свойствам материалов.    
	//  параметр GL_LIGHT_MODEL_AMBIENT - задать фоновое освещение, 
	//                не зависящее от сточников
	// по умолчанию (0.2, 0.2, 0.2, 1.0)

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	/*
	//texture1.loadTextureFromFile("textures\\texture.bmp");   загрузка текстуры из файла
	*/


	frac.VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	frac.FshaderFileName = "shaders\\frac.frag"; //имя файла фрагментного шейдера
	frac.LoadShaderFromFile(); //загружаем шейдеры из файла
	frac.Compile(); //компилируем

	cassini.VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	cassini.FshaderFileName = "shaders\\cassini.frag"; //имя файла фрагментного шейдера
	cassini.LoadShaderFromFile(); //загружаем шейдеры из файла
	cassini.Compile(); //компилируем


	s[0].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	s[0].FshaderFileName = "shaders\\light.frag"; //имя файла фрагментного шейдера
	s[0].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[0].Compile(); //компилируем

	s[1].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	s[1].FshaderFileName = "shaders\\textureShader.frag"; //имя файла фрагментного шейдера
	s[1].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[1].Compile(); //компилируем



	 //так как гит игнорит модели *.obj файлы, так как они совпадают по расширению с объектными файлами, 
	 // создающимися во время компиляции, я переименовал модели в *.obj_m
	loadModel("models\\8.obj", &objModel);

	loadModel("models\\12.obj", &T);
	glActiveTexture(GL_TEXTURE0);
	loadModel("models\\6.obj", &monkey);
	monkeyTex.loadTextureFromFile("textures//7.bmp");
	monkeyTex.bindTexture();
	glActiveTexture(GL_TEXTURE0);
	Tex1.loadTextureFromFile("textures//tex.bmp");
	Tex1.bindTexture();
	T2 = monkeyTex;
	M = monkey;
	tick_n = GetTickCount();
	tick_o = tick_n;

	rec.setSize(300, 100);
	rec.setPosition(10, ogl->getHeight() - 100 - 10);
	rec.setText("1 - Сменить текстуру\n2 -Вернуть исходную текстуру\n3 - Сменить модель\n4-Вернуть исходную модель\nI-Увеличить квадрат\nO-уменьшить квадрат\n ", 0, 0, 0);


}




inline double fact(int n) {
	int r = 1;
	for (int i = 2; i <= n; ++i) {
		r *= i;
	}
	return r;
}
inline double bernshtein(int i, int n, double u) {
	return 1.0 * fact(n) * pow(u, i) * pow(1 - u, n - i) / fact(i) / fact(n - i);
}
Vector3 BCurve(Vector3* points, int n, double t) {
	Vector3 res(0, 0, 0);
	for (int i = 0; i < n; i++) {
		res = res + points[i] * bernshtein(i, n - 1, t);
	}
	return res;
}
void draw() {
	Vector3 a(0, 0, 0), b(1, 0, 0), c(1, 1, 0), d(0, 1, 0);
	glPushMatrix();
	glTranslated(-0.5, -0.5, 0);
	glBegin(GL_QUADS);
	glNormal3d(0, 0, 1);
	glColor3f(1.0, 1.0, 0.0);
	glVertex3dv(a.toArray());
	glVertex3dv(b.toArray());
	glVertex3dv(c.toArray());
	glVertex3dv(d.toArray());
	glEnd();
	glPopMatrix();
}
void Cube() {
	glPushMatrix();
	glTranslated(0, 0, -0.5);
	glRotated(180, 1, 0, 0);
	draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(0, 0, 0.5);
	draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(0, -0.5, 0);
	glRotated(90, 1, 0, 0);
	draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(0, 0.5, 0);
	glRotated(-90, 1, 0, 0);
	draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(0, 0, -0.5);
	glRotated(180, 1, 0, 0);
	draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(0, 0, 0.5);
	draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(0, 0, 0.5);
	draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(0, -0.5, 0);
	glRotated(90, 1, 0, 0);
	draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(0, 0.5, 0);
	glRotated(-90, 1, 0, 0);
	draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(0.5, 0, 0);
	glRotated(90, 0, 1, 0);
	draw();
	glPopMatrix();

	glPushMatrix();
	glTranslated(-0.5, 0, 0);
	glRotated(-90, 0, 1, 0);
	draw();
	glPopMatrix();

}
Robot robot;

double X = 1, Y = 1;
double B = 20;
int u = 1;
void povor()
{
	if (OpenGL::isKeyPressed(VK_UP))
	{


		robot.ygl = 0;
		u = 1;
	}

	if (OpenGL::isKeyPressed(VK_DOWN))
	{
		robot.ygl = 180;
		u = 4;
	}
	if (OpenGL::isKeyPressed(VK_LEFT))
	{


		robot.ygl = 90;
		u = 0;
	}

	if (OpenGL::isKeyPressed(VK_RIGHT))
	{


		robot.ygl = -90;
		u = 3;

	}

}
void dvig()
{

	const double step = 1.0;

	if (OpenGL::isKeyPressed('W') && (robot.pozy < B))
	{
		if (u == 1)
		{
			if (robot.pozy + step <= B)
				robot.pozy += step;
		}
		else if (u == 0)
		{
			if (robot.pozx + step <= B)
				robot.pozx += step;
		}
		else if (u == 3)
		{
			if (robot.pozx - step >= -B)
				robot.pozx -= step;
		}
		else
		{
			if (robot.pozy - step >= -B)
				robot.pozy -= step;
		}
	}

	if (OpenGL::isKeyPressed('S') && robot.pozy > -B)
	{
		if (u == 1)
		{
			if (robot.pozy - step >= -B)
				robot.pozy -= step;
		}
		else if (u == 0)
		{
			if (robot.pozx - step >= -B)
				robot.pozx -= step;
		}
		else if (u == 3)
		{
			if (robot.pozx + step <= B)
				robot.pozx += step;
		}
		else
		{
			if (robot.pozy + step <= B)
				robot.pozy += step;
		}
	}

	if (OpenGL::isKeyPressed('A') && robot.pozx < B)
	{
		if (u == 1)
		{
			if (robot.pozx + step <= B)
				robot.pozx += step;
		}
		else if (u == 0)
		{
			if (robot.pozy - step >= -B)
				robot.pozy -= step;
		}
		else if (u == 3)
		{
			if (robot.pozy + step <= B)
				robot.pozy += step;
		}
		else
		{
			if (robot.pozx - step >= -B)
				robot.pozx -= step;
		}
	}

	if (OpenGL::isKeyPressed('D') && robot.pozx > -B)
	{
		if (u == 1)
		{
			if (robot.pozx - step >= -B)
				robot.pozx -= step;
		}
		else if (u == 0)
		{
			if (robot.pozy + step <= B)
				robot.pozy += step;
		}
		else if (u == 3)
		{
			if (robot.pozy - step >= -B)
				robot.pozy -= step;
		}
		else
		{
			if (robot.pozx + step <= B)
				robot.pozx += step;
		}
	}


}
int A = 1;

void cvet()
{
	if (OpenGL::isKeyPressed('1'))
	{
		monkeyTex = Tex1;
		monkeyTex.bindTexture();
	}

	if (OpenGL::isKeyPressed('2'))
	{
		monkeyTex = T2;
		monkeyTex.bindTexture();
	}
	if (OpenGL::isKeyPressed('3'))
	{
		monkey = objModel;
		A = 0;
	}
	if (OpenGL::isKeyPressed('4'))
	{
		monkey = M;
		A = 1;
	}

}
void quads()
{
	if (OpenGL::isKeyPressed('I'))
	{

		B = B + 1;
	}
	if (OpenGL::isKeyPressed('O'))
	{

		B = B - 1;
	}
}

float anim_speed = 0.001;
double anim_speed_temp = anim_speed;
void Render(OpenGL* ogl)
{
	tick_o = tick_n;
	tick_n = GetTickCount();
	Time += (tick_n - tick_o) / 1000.0;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	glEnable(GL_DEPTH_TEST);
	if (textureMode)
		glEnable(GL_TEXTURE_2D);

	if (lightMode)
		glEnable(GL_LIGHTING);


	s[0].UseShader();

	int location = glGetUniformLocationARB(s[0].program, "light_pos");
	glUniform3fARB(location, light.pos.X(), light.pos.Y(), light.pos.Z());

	location = glGetUniformLocationARB(s[0].program, "Ia");
	glUniform3fARB(location, 0.2, 0.2, 0.2);

	location = glGetUniformLocationARB(s[0].program, "Id");
	glUniform3fARB(location, 1.0, 1.0, 1.0);

	location = glGetUniformLocationARB(s[0].program, "Is");
	glUniform3fARB(location, .7, .7, .7);

	location = glGetUniformLocationARB(s[0].program, "ma");
	glUniform3fARB(location, 0.2, 0.2, 0.1);

	location = glGetUniformLocationARB(s[0].program, "md");
	glUniform3fARB(location, 0.4, 0.65, 0.5);

	location = glGetUniformLocationARB(s[0].program, "ms");
	glUniform4fARB(location, 0.9, 0.8, 0.3, 25.6);

	location = glGetUniformLocationARB(s[0].program, "camera");
	glUniform3fARB(location, camera.pos.X(), camera.pos.Y(), camera.pos.Z());



	s[1].UseShader();
	cvet();
	int l = glGetUniformLocationARB(s[1].program, "7");
	glUniform1iARB(l, 0);
	glPushMatrix();
	glRotated(90, 0, 1, 0);
	glRotated(90, 0, 0, 1);
	dvig();
	povor();
	glTranslated(robot.pozx, 0, robot.pozy);
	glRotated(robot.ygl, 0, 1, 0);
	monkeyTex.bindTexture();
	monkey.DrawObj();
	glPopMatrix();

	if (A == 1)
	{
		s[0].UseShader();
		Vector3 p[4];
		p[0] = Vector3(-1 + robot.pozy, 0 + robot.pozx, 15.5);
		p[1] = Vector3(10 + robot.pozy, 30 + robot.pozx, 20);
		p[2] = Vector3(40 + robot.pozy, 50 + robot.pozx, 30);
		p[3] = Vector3(70 + robot.pozy, 0 + robot.pozx, 0);
		glPushMatrix();
		Vector3 pos = BCurve(p, 4, anim_speed_temp);
		Vector3 pre_pos = BCurve(p, 4, anim_speed_temp - anim_speed);
		Vector3 dir = (pos - pre_pos).normolize();
		Vector3 orig(1, 0, 0);
		Vector3 rotX(dir.X(), dir.Y(), 0);
		rotX = rotX.normolize();
		double cosU = orig.scalarProisvedenie(rotX);
		Vector3 vecProd = orig.vectProisvedenie(rotX);
		double sinSign = vecProd.Z() / fabs(vecProd.Z());
		double U = acos(cosU) * 180.0 / 3.14 * sinSign;
		double cosZU = Vector3(0, 0, 1).scalarProisvedenie(dir);
		double ZU = acos(dir.Z()) * 180.0 / M_PI - 90;
		glTranslated(pos.X(), pos.Y(), pos.Z());
		glRotated(U, 0, 0, 1);
		glRotated(ZU, 0, 1, 0);
		Cube();
		glPopMatrix();
		glDisable(GL_LIGHTING);
		glColor3d(1, 0.3, 0);
		glBegin(GL_LINES);
		glVertex3dv(pos.toArray());
		glVertex3dv((pos + dir.normolize() * 3).toArray());
		glEnd();
		anim_speed_temp += anim_speed;
		if (anim_speed_temp > 1)
			anim_speed = -anim_speed;
		if (anim_speed_temp < 0)
			anim_speed = -anim_speed;
		glBegin(GL_LINE_STRIP);
		glDisable(GL_LIGHTING);
		glColor3f(1, 0, 1);
		for (double t = 0; t <= 1; t += 0.01)
			glVertex3dv(BCurve(p, 4, t).toArray());

		glEnd();
		glPopMatrix();
	}
	s[0].UseShader();

	glPushMatrix();
	quads();
	glBegin(GL_QUADS);
	glColor3f(0.0, 0.0, 1.0);
	glVertex3d(-B, -B, 0);
	glVertex3d(B, -B, 0);
	glVertex3d(B, B, 0);
	glVertex3d(-B, B, 0);
	glEnd();
	glPopMatrix();

	Shader::DontUseShaders();


}



bool gui_init = false;

//рисует интерфейс, вызывется после обычного рендера
void RenderGUI(OpenGL* ogl)
{

	Shader::DontUseShaders();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, ogl->getWidth(), 0, ogl->getHeight(), 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_LIGHTING);


	glActiveTexture(GL_TEXTURE0);
	rec.Draw();

	Shader::DontUseShaders();

}

void resizeEvent(OpenGL* ogl, int newW, int newH)
{
	rec.setPosition(10, newH - 100 - 10);
}

