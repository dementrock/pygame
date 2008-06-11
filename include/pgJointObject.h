#ifndef _PYGAME_PHYSICS_JOINT_
#define _PYGAME_PHYSICS_JOINT_

#include "pgBodyObject.h"

typedef struct _pgJointObject pgJointObject;

typedef struct _pgJointObject{
	PyObject_HEAD

	pgBodyObject*	body1;
	pgBodyObject*	body2;
	int		isCollideConnect;
	void	(*SolveConstraint)(pgJointObject* joint,double stepTime);
	void	(*Destroy)(pgJointObject* joint);
} pgJointObject;

void PG_JointDestroy(pgJointObject* joint);

typedef struct _pgDistanceJoint{
	pgJointObject		joint;

	double		distance;
	pgVector2	anchor1,anchor2;
} pgDistanceJoint;

pgJointObject* PG_DistanceJointNew(pgBodyObject* b1,pgBodyObject* b2,int bCollideConnect,double distance,pgVector2 a1,pgVector2 a2);

#endif //_PYGAME_PHYSICS_JOINT_