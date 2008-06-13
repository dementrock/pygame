#include <GL/glut.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>

#define HEIGHT 600
#define WIDTH 600
#define MAX_STR_LEN 1000

/*-------------------------------���Թ���----------------------------*/

#include "pgPhysicsRenderer.h"

static pgWorldObject* s_world = NULL;
static double s_updateTime = 0.0;

//�ܱ�
static double s_time;
static int s_first = 1;

void watch_start()
{
	s_time = (double)clock();
}

//������ܱ�򿪵�ֹͣ�������˶�����
double watch_stop()
{
	return ((double)clock() - s_time)/CLOCKS_PER_SEC;
}

//��Ⱦ����, ����Ӣ�ģ�(x, y)�����־������Ͻ�����
//ÿ���ַ��ַ���9���أ���15����
void draw_text(int x, int y, char *str)
{
	int len, i, w, h;

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	w = glutGet(GLUT_WINDOW_WIDTH);
	h = glutGet(GLUT_WINDOW_HEIGHT);
	gluOrtho2D(0, w, h, 0);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glColor3f(0.6f, 0.8f, 0.6f);
	glRasterPos2i(x, y + 15);
	len = (int) strlen(str);
	for (i = 0; i < len; i++)
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, str[i]);

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

//��ӡ���ֵ���Ļ������printf���Ƽ�
void glprintf(int x, int y, const char* fmt, ...)
{
	static char buf[MAX_STR_LEN];
	va_list p;
	va_start(p, fmt);
	memset(buf, 0, sizeof(buf));
	vsprintf(buf, fmt, p);
	draw_text(x, y, buf);
}





/*-------------------------------���Ժ���----------------------------*/

//��Ⱦ����
void do_render()
{
	//glprintf(0, 0, "Your RP value is: %d", 0);
	glColor3f(1.f, 1.f, 1.f);
	/*glBegin(GL_LINES);
	glVertex2f(-10, 0);
	glVertex2f(10, 0);
	glEnd();*/
	PGT_RenderWorld(s_world);

}



//keyboard������Ӧ
void keyboard (unsigned char key, int x, int y)
{
	switch(key)
	{
	case 27:
		PG_WorldDestroy(s_world);
		exit(0);
		break;
	}
}





/*-------------------------------���ú���----------------------------*/


void InitGL()
{
	glShadeModel(GL_SMOOTH);	
	glPointSize(3.f);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
	glHint(GL_POINT_SMOOTH_HINT, GL_DONT_CARE);
	glLineWidth(2.5f);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-WIDTH/2, WIDTH/2, -HEIGHT/2, HEIGHT/2, -1.f, 1.f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


void display(void)
{
	double dt;
	//watch_start();

	

	if(s_first)
	{
		watch_start();
		s_first = 0;
		return;
	}
	dt = watch_stop();
	watch_start();
	/*s_updateTime += dt;
	if(s_updateTime >= s_world->fStepTime)
	{
		PG_Update(s_world,s_updateTime);
		s_updateTime = 0.0;

		glClear(GL_COLOR_BUFFER_BIT);
		glLoadIdentity();
		do_render();
		glutSwapBuffers();
	}*/
	PG_Update(s_world,dt);
	
	glClear(GL_COLOR_BUFFER_BIT);
	glLoadIdentity();
	do_render();
	glutSwapBuffers();

	
	
}

//�������һ��ʼ�ͻᱻ���ã���gluPerspective����û��Ҫ��initGL����display���������
void reshape (int width , int height)
{
	if(height == 0)										
		height = 1;										

	glViewport(0,0,width,height);						
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-width/2, width/2, -height/2, height/2, -1.f, 1.f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();									
}

//============================================

void TestBasic1Init()
{
	pgBodyObject* body;
	pgJointObject* joint;
	pgVector2 a1,a2;
	PG_Set_Vector2(a1,0,0);
	PG_Set_Vector2(a2,0,100);

	s_world = PG_WorldNew();
	s_world->fStepTime = 0.03;
	body = PG_BodyNew();
	PG_Set_Vector2(body->vecPosition,0,0)
	PG_Set_Vector2(body->vecLinearVelocity,40,0)
	PG_AddBodyToWorld(s_world,body);
	
	
	joint = PG_DistanceJointNew(body,NULL,0,100,a1,a2);
	PG_AddJointToWorld(s_world,joint);
}

void TestBasic2Init()
{
	pgBodyObject* body;
	s_world = PG_WorldNew();
	s_world->fStepTime = 0.03;
	body = PG_BodyNew();
	PG_Set_Vector2(body->vecPosition,0,0)
	PG_Set_Vector2(body->vecLinearVelocity,0,30)
	PG_AddBodyToWorld(s_world,body);
}

void TestBasic3Init()
{
	pgBodyObject* body1,*body2;
	pgJointObject* joint;
	pgVector2 a1,a2;
	PG_Set_Vector2(a1,0,0);
	PG_Set_Vector2(a2,0,0);

	s_world = PG_WorldNew();
	s_world->fStepTime = 0.03;
	body1 = PG_BodyNew();
	PG_Set_Vector2(body1->vecPosition,0,0)
	PG_Set_Vector2(body1->vecLinearVelocity,10,0)
	PG_AddBodyToWorld(s_world,body1);

	body2 = PG_BodyNew();
	PG_Set_Vector2(body2->vecPosition,0,100)
	PG_Set_Vector2(body2->vecLinearVelocity,0,0)
	PG_AddBodyToWorld(s_world,body2);


	joint = PG_DistanceJointNew(body1,body2,0,100,a1,a2);
	PG_AddJointToWorld(s_world,joint);
}

void TestBasic4Init()
{
	#define BODY_NUM 3

	int i;
	pgBodyObject* body[BODY_NUM + 1];
	pgJointObject* joint[BODY_NUM];
	pgVector2 a1,a2;
	PG_Set_Vector2(a1,0,0);
	PG_Set_Vector2(a2,0,100);

	s_world = PG_WorldNew();
	s_world->fStepTime = 0.03;

	body[0] = NULL;
	for (i = 1;i < BODY_NUM + 1;i++)
	{
		body[i] = PG_BodyNew();
		PG_Set_Vector2(body[i]->vecPosition,0,(-i*50 + 100))
		PG_Set_Vector2(body[i]->vecLinearVelocity,0,0)
		PG_AddBodyToWorld(s_world,body[i]);
	}

	PG_Set_Vector2(body[BODY_NUM]->vecLinearVelocity,50,0)

	i = 0;
	joint[i] = PG_DistanceJointNew(body[i+1],body[i],0,50,a1,a2);
	PG_AddJointToWorld(s_world,joint[i]);
	for (i = 1;i < BODY_NUM;i++)
	{
		joint[i] = PG_DistanceJointNew(body[i],body[i+1],0,50,a1,a2);
		PG_AddJointToWorld(s_world,joint[i]);
	}


}

//===============================================

void InitWorld()
{
	TestBasic4Init();
}

int main (int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(WIDTH, HEIGHT);
	glutCreateWindow("test physics");
	InitWorld();
	InitGL();
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutIdleFunc(display);
	glutMainLoop();
	return 0;
}

