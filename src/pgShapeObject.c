#include "pgShapeObject.h"
#include "pgBodyObject.h"
#include "pgCollision.h"
#include <string.h>
#include <math.h>
#include <assert.h>
#include <float.h>

//functions of pgShapeObject

void PG_ShapeObjectInit(pgShapeObject* shape)
{
	//TODO: maybe these init methods are not suitable. needing refined.
	memset(&(shape->box), 0, sizeof(shape->box));
	shape->Destroy = NULL;
	shape->Collision = NULL;
}

void PG_ShapeObjectDestroy(pgShapeObject* shape)
{
	if(shape != NULL)
		shape->Destroy(shape);
}



//functions of pgRectShape

void PG_RectShapeUpdateAABB(pgBodyObject* body)
{
	int i;
	pgVector2 gp[4];

	if(body->shape->type == ST_RECT)
	{
		pgRectShape *p = (pgRectShape*)body->shape;
		
		PG_AABBClear(&(p->shape.box));
		for(i = 0; i < 4; ++i)
			gp[i] = PG_GetGlobalPos(body, &(p->point[i]));
		for(i = 0; i < 4; ++i)
			PG_AABBExpandTo(&(p->shape.box), &gp[i]);
	}
}

void PG_RectShapeDestroy(pgShapeObject* rectShape)
{
	PyObject_Free((pgRectShape*)rectShape);
}


int PG_RectShapeCollision(pgBodyObject* selfBody, pgBodyObject* incidBody, PyObject* contactList);


pgShapeObject*	PG_RectShapeNew(pgBodyObject* body, double width, double height, double seta)
{
	int i;
	pgRectShape* p = (pgRectShape*)PyObject_MALLOC(sizeof(pgRectShape));

	PG_ShapeObjectInit(&(p->shape));
	p->shape.Destroy = PG_RectShapeDestroy;
	p->shape.UpdateAABB = PG_RectShapeUpdateAABB;
	p->shape.Collision = PG_RectShapeCollision;
	p->shape.type = ST_RECT;
	p->shape.rInertia = body->fMass*(width*width + height*height)/12; // I = M(a^2 + b^2)/12

	PG_Set_Vector2(p->bottomLeft, -width/2, -height/2);
	PG_Set_Vector2(p->bottomRight, width/2, -height/2);
	PG_Set_Vector2(p->topRight, width/2, height/2);
	PG_Set_Vector2(p->topLeft, -width/2, height/2);
	for(i = 0; i < 4; ++i)
		c_rotate(&(p->point[i]), seta);
	
	return (pgShapeObject*)p;
}

//-------------box's collision test------------------
//TEST: these functions have been partly tested.

//we use a simple SAT to select the contactNormal:
//Supposing the relative velocity between selfBody and incidBody in
//two frame is "small", the face(actually is an edge in 2D) with minimum 
//average penetrating depth is considered to be contact face, then we
//get the contact normal.
//note: this method is not available in CCD(continue collision detection)
//since the velocity is not small.
//static double _SAT_GetContactNormal1(pgAABBBox* clipBox, PyObject* contactList,
//								  int from, int to)
//{
//	int i;
//	int id;
//	double deps[4], min_dep;
//	pgContact* p;
//	pgVector2 normal;
//		
//	memset(deps, 0, sizeof(deps));
//	for(i = from; i <= to; ++i)
//	{
//		p = (pgContact*)PyList_GetItem(contactList, i);
//		deps[0] += p->pos.real - clipBox->left; //left
//		deps[1] += p->pos.imag - clipBox->bottom; //bottom
//		deps[2] += clipBox->right - p->pos.real; //right
//		deps[3] += clipBox->top - p->pos.imag; //top
//	}
//	
//	//find min penetrating face
//	id = 0;
//	min_dep = deps[0];
//	for(i = 1; i < 4; ++i)
//	{
//		if(min_dep > deps[i])
//		{
//			min_dep = deps[i];
//			id = i;
//		}
//	}
//	PG_Set_Vector2(normal, 0, 0);
//	//generate contactNormal
//	switch(id)
//	{
//	case 0://left
//		PG_Set_Vector2(normal, -1, 0);
//		break;
//	case 1://bottom
//		PG_Set_Vector2(normal, 0, -1);
//		break;
//	case 2://right
//		PG_Set_Vector2(normal, 1, 0);
//		break;
//	case 3://top
//		PG_Set_Vector2(normal, 0, 1);
//		break;
//	}
//
//    for(i = from; i <= to; ++i)
//    {
//        p = (pgContact*)PyList_GetItem(contactList, i);
//        p->normal = normal;
//    }
//
//	return min_dep;
//}
//
//#define _swap(a, b, t) {(t) = (a); (a) = (b); (b) = (t);}
//
////TODO: now just detect Box-Box collision, later add Box-Circle
//int PG_RectShapeCollision1(pgBodyObject* selfBody, pgBodyObject* incidBody, PyObject* contactList)
//{
//	int i, i1, k;
//	int from, to, _from[2], _to[2];
//	int apart;
//	pgVector2 ip[4];
//	int has_ip[4]; //use it to prevent from duplication
//	pgVector2 pf, pt;
//	pgRectShape *self, *incid, *tmp;
//	pgBodyObject * tmpBody;
//	pgAABBBox clipBox;
//	pgContact* contact;
//	pgVector2* pAcc;
//	PyObject* list[2];
//	double dist[2];
//
//	list[0] = PyList_New(0);
//	list[1] = PyList_New(0);
//
//	self = (pgRectShape*)selfBody->shape;
//	incid = (pgRectShape*)incidBody->shape;
//
//	apart = 1;	
//	for(k = 0; k < 2; ++k)
//	{
//		//transform incidBody's coordinate according to selfBody's coordinate
//		for(i = 0; i < 4; ++i)
//			ip[i] = PG_GetRelativePos(selfBody, incidBody, &(incid->point[i]));
//		//clip incidBody by selfBody
//		clipBox = PG_GenAABB(self->bottomLeft.real, self->topRight.real,
//			self->bottomLeft.imag, self->topRight.imag);
//		memset(has_ip, 0, sizeof(has_ip));
//		_from[k] = PyList_Size(list[k]);
//		
//		for(i = 0; i < 4; ++i)
//		{
//			i1 = (i+1)%4;
//			if(PG_LiangBarskey(&clipBox, &ip[i], &ip[i1], &pf, &pt))
//			{
//				apart = 0;
//				if(pf.real == ip[i].real && pf.imag == ip[i].imag)
//				{
//					has_ip[i] = 1;
//				}
//				else
//				{
//					contact = (pgContact*)PG_ContactNew(selfBody, incidBody);
//					contact->pos = pf;
//					PyList_Append(list[k], (PyObject*)contact);
//				}
//				
//				if(pt.real == ip[i1].real && pt.imag == ip[i1].imag)
//				{	
//					has_ip[i1] = 1;
//				}
//				else
//				{
//					contact = (pgContact*)PG_ContactNew(selfBody, incidBody);
//					contact->pos = pt;
//					PyList_Append(list[k], (PyObject*)contact);
//				}
//
//			}
//		}
//
//		if(apart)
//			goto END;
//
//		for(i = 0; i < 4; ++i)
//		{
//			if(has_ip[i])
//			{
//				contact = (pgContact*)PG_ContactNew(selfBody, incidBody);
//				contact->pos = ip[i];
//				PyList_Append(list[k], (PyObject*)contact);
//			}
//		}
//		//now all the contact points are added to list
//		_to[k] = PyList_Size(list[k]) - 1;
//		dist[k] = _SAT_GetContactNormal(&clipBox, list[k], _from[k], _to[k]);
//
//		//now swap refBody and incBody, and do it again
//		_swap(selfBody, incidBody, tmpBody);
//		_swap(self, incid, tmp);
//	}
//
//	from = PyList_Size(contactList);
//	for(i = 0; i < 2; ++i)
//	{
//		i1 = (i+1)%2;
//		if(dist[i] <= dist[i1])
//		{
//			for(k = _from[i]; k <= _to[i]; ++k)
//			{
//				contact = (pgContact*)PyList_GetItem(list[i], k);
//				PyList_Append(contactList, (PyObject*)contact);
//				Py_XINCREF((PyObject*)contact);
//			}
//			break;
//		}
//		_swap(selfBody, incidBody, tmpBody);
//	}
//	to = PyList_Size(contactList) - 1;
//
//	pAcc = PyObject_Malloc(sizeof(pgVector2));
//	pAcc->real = pAcc->imag = 0;
//	for(i = from; i <= to; ++i)
//	{
//		contact = (pgContact*)PyList_GetItem(contactList, i);
//
//		c_rotate(&(contact->pos), selfBody->fRotation);
//		contact->pos = c_sum(contact->pos, selfBody->vecPosition);
//		c_rotate(&(contact->normal), selfBody->fRotation);
//
//		contact->ppAccMoment = PyObject_Malloc(sizeof(pgVector2*));
//		*(contact->ppAccMoment) = pAcc;
//
//		contact->weight = (to - from)+1;
//	}
//
//
//END:
//	Py_XDECREF(list[0]);
//	Py_XDECREF(list[1]);
//	return 1;
//}


////all the points are in refbox's locate coordinate system
//static double _SAT_GetContactNormal(pgAABBBox* clipBox, pgVector2* clist, 
//									int csize, pgVector2* normal)
//{	
//	int i;
//	int id;
//	double deps[4], min_dep;
//	pgVector2* p;
//		
//	memset(deps, 0, sizeof(deps));
//	for(i = 0; i < csize; ++i)
//	{
//		p = &clist[i];
//		deps[0] += fabs(p->real - clipBox->left); //left
//		deps[1] += fabs(p->imag - clipBox->bottom); //bottom
//		deps[2] += fabs(clipBox->right - p->real); //right
//		deps[3] += fabs(clipBox->top - p->imag); //top
//	}
//	
//	//find min penetrating face
//	id = 0;
//	min_dep = deps[0];
//	for(i = 1; i < 4; ++i)
//	{
//		if(min_dep > deps[i])
//		{
//			min_dep = deps[i];
//			id = i;
//		}
//	}
//	PG_Set_Vector2(*normal, 0, 0);
//	//generate contactNormal
//	switch(id)
//	{
//	case 0://left
//		PG_Set_Vector2(*normal, -1, 0);
//		break;
//	case 1://bottom
//		PG_Set_Vector2(*normal, 0, -1);
//		break;
//	case 2://right
//		PG_Set_Vector2(*normal, 1, 0);
//		break;
//	case 3://top
//		PG_Set_Vector2(*normal, 0, 1);
//		break;
//	default:
//		assert(0);
//		break;
//	}
//
//	return min_dep;
//}
//
//
//static int _PG_RectShapeCollision(pgBodyObject* selfBody, pgBodyObject* incidBody,
//								  pgVector2* clist, int* csize, pgVector2 *normal, double* depth)
//{
//	pgRectShape * self, * incid;
//	pgVector2 gp[4], ans_p1, ans_p2;
//	int has_ip[4];
//	pgAABBBox clipBox;
//	int i, i1;
//	int axis;
//	int valid_p1, valid_p2;
//	pgVector2 cpoints[2][10], n[2];
//	int csizes[2];
//	double dep[2];
//	int id;
//	int apart[2];
//
//	self = (pgRectShape*)selfBody->shape;
//	incid = (pgRectShape*)incidBody->shape;
//
//	clipBox = PG_GenAABB(self->bottomLeft.real, self->topRight.real,
//		self->bottomLeft.imag, self->topRight.imag);
//
//	for(i = 0; i < 4; ++i)
//		gp[i] = PG_GetRelativePos(selfBody, incidBody, &(incid->point[i]));
//	
//	for(axis = CA_X; axis <= CA_Y; ++axis)
//	{
//		memset(has_ip, 0, sizeof(has_ip));
//		apart[axis] = 1;
//		csizes[axis] = 0;
//		for(i = 0; i < 4; ++i)
//		{
//			i1 = (i+1)%4;
//			if(PG_PartlyLB(&clipBox, &gp[i], &gp[i1], axis, 
//				&ans_p1, &ans_p2, &valid_p1, &valid_p2))
//			{
//				apart[axis] = 0;
//				if(valid_p1)
//				{
//					if(c_equal(&ans_p1, &gp[i]))
//						has_ip[i] = 1;
//					else
//						cpoints[axis][csizes[axis]++] = ans_p1;
//				}
//				if(valid_p2)
//				{
//					if(c_equal(&ans_p2, &gp[i1]))
//						has_ip[i1] = 1;
//					else
//						cpoints[axis][csizes[axis]++] = ans_p2;
//				}
//			}
//		}
//
//		for(i = 0; i < 4; ++i)
//			if(has_ip[i])
//				cpoints[axis][csizes[axis]++] = gp[i];
//		dep[axis] = _SAT_GetContactNormal(&clipBox, cpoints[axis], csizes[axis], &n[axis]);
//	}
//
//	if(apart[CA_X] && apart[CA_Y]) return 0;
//	//assert(csizes[0] > 0 && csizes[1] > 0);
//	
//	id = dep[CA_X] < dep[CA_Y] ? CA_X : CA_Y;
//	if(csizes[id] == 0)
//		id = (id + 1)%2;
//	*csize = csizes[id];
//	for(i = 0; i < csizes[id]; ++i)
//		clist[i] = cpoints[id][i];
//	*normal = n[id];
//	*depth = dep[id];
//
//	return 1;
//}
//
//int PG_RectShapeCollision(pgBodyObject* selfBody, pgBodyObject* incidBody, 
//						  PyObject* contactList)
//{
//	pgVector2 clist1[10], clist2[10], *clist, n1, n2, *n;
//	int csize1, csize2, *csize;
//	double dep1, dep2;
//	int is_collided;
//	pgBodyObject *self, *incid;
//	int i;
//	pgContact* contact;
//	pgVector2* pAcc;
//
//	is_collided = _PG_RectShapeCollision(selfBody, incidBody, clist1, 
//				  &csize1, &n1, &dep1);
//	if(!is_collided) return 0;
//	is_collided = _PG_RectShapeCollision(incidBody, selfBody, clist2, 
//				  &csize2, &n2, &dep2);
//	assert(is_collided);
//
//	if(dep1 < dep2)
//	{
//		clist = clist1;
//		csize = &csize1;
//		n = &n1;
//		self = selfBody;
//		incid = incidBody;
//	}
//	else
//	{
//		clist = clist2;
//		csize = &csize2;
//		n = &n2;
//		self = incidBody;
//		incid = selfBody;
//	}
//	assert(*csize > 0);
//
//
//	pAcc = PyObject_Malloc(sizeof(pgVector2));
//	pAcc->real = pAcc->imag = 0;
//	for(i = 0; i < *csize; ++i)
//	{
//		contact = (pgContact*)PG_ContactNew(self, incid);
//		contact->pos = clist[i];
//		contact->normal = *n;
//		c_rotate(&(contact->pos), self->fRotation);
//		contact->pos = c_sum(contact->pos, self->vecPosition);
//		c_rotate(&(contact->normal), self->fRotation);
//
//		contact->ppAccMoment = PyObject_Malloc(sizeof(pgVector2*));
//		*(contact->ppAccMoment) = pAcc;
//		contact->weight = *csize;
//		PyList_Append(contactList, (PyObject*)contact);
//	}
//
//	return 1;
//}


static int _Get_Depth(pgBodyObject* refBody, pgBodyObject* incBody,
					   int* faceId, double* min_dep, pgVector2* gp_in_ref, 
					   pgAABBBox* clipBox)
{
	int i, apart;
	pgRectShape *ref, *inc;
	double deps[4];

	ref = (pgRectShape*)refBody->shape;
	inc = (pgRectShape*)incBody->shape;
	memset(gp_in_ref, 0, sizeof(gp_in_ref));
	memset(deps, 0, sizeof(deps));
	for(i = 0; i < 4; ++i)
		gp_in_ref[i] = PG_GetRelativePos(refBody, incBody, &(inc->point[i]));

	*clipBox = PG_GenAABB(ref->bottomLeft.real, ref->topRight.real,
		ref->bottomLeft.imag, ref->topRight.imag);

	apart = 1;
	for(i = 0; i < 4; ++i)
		if(PG_IsIn(&gp_in_ref[i], clipBox))
		{
			apart = 0;
			deps[CF_LEFT] += fabs(gp_in_ref[i].real - clipBox->left);
			deps[CF_RIGHT] += fabs(clipBox->right - gp_in_ref[i].real);
			deps[CF_BOTTOM] += fabs(gp_in_ref[i].imag - clipBox->bottom);
			deps[CF_TOP] += fabs(clipBox->top - gp_in_ref[i].imag);
		}

	if(apart) return 0;

	*min_dep = deps[0];
	*faceId = 0;

	for(i = 1; i < 4; ++i)
		if(deps[i] < *min_dep)
		{
			*min_dep = deps[i];
			*faceId = i;
		}

	return 1;
}

static int _SAT_Select(pgBodyObject* body1, pgBodyObject* body2,
					   pgBodyObject** refBody, pgBodyObject** incBody,
					   int* face_id, pgVector2* gp_in_ref, pgAABBBox* clipBox)
{
	double min_dep[2];
	int id[2];
	pgVector2 gp[2][4];
	pgAABBBox cb[2];
	int is_in[2];
	int i;
	
	min_dep[0] = min_dep[1] = DBL_MAX;
	is_in[0] = _Get_Depth(body1, body2, &id[0], &min_dep[0], gp[0], &cb[0]);
	is_in[1] = _Get_Depth(body2, body1, &id[1], &min_dep[1], gp[1], &cb[1]);

	if(!is_in[0] && !is_in[1]) return 0;

	if(min_dep[0] < min_dep[1])
	{
		*refBody = body1;
		*incBody = body2;
		*face_id = id[0];
		for(i = 0; i < 4; ++i)
			gp_in_ref[i] = gp[0][i];
		*clipBox = cb[0];
	}
	else
	{
		*refBody = body2;
		*incBody = body1;
		*face_id = id[1];
		for(i = 0; i < 4; ++i)
			gp_in_ref[i] = gp[1][i];
		*clipBox = cb[1];
	}

	return 1;
}

int _Build_Contacts(pgVector2* gp, pgAABBBox* clipBox, int axis,
					 pgVector2 contacts[], int* size)
{
	int i, i1;
	int apart = 1;
	int has_ip[4];
	pgVector2 pf, pt;
	int valid_pf, valid_pt;

	*size = 0;
	memset(has_ip, 0, sizeof(has_ip));
	for(i = 0; i < 4; ++i)
	{
		i1 = (i+1)%4;
		if(PG_PartlyLB(clipBox, &gp[i], &gp[i1], axis, 
			&pf, &pt, &valid_pf, &valid_pt))
		{
			apart = 0;
			if(valid_pf)
			{
				if(c_equal(&pf, &gp[i]))
					has_ip[i] = 1;
				else
					contacts[(*size)++] = pf;
			}
			if(valid_pt)
			{
				if(c_equal(&pt, &gp[i1]))
					has_ip[i1] = 1;
				else
					contacts[(*size)++] = pt;
			}
		}
	}
	for(i = 0; i < 4; ++i)
		if(has_ip[i])
			contacts[(*size)++] = gp[i];

	return !apart;
}

int PG_RectShapeCollision(pgBodyObject* selfBody, pgBodyObject* incidBody, 
						  PyObject* contactList)
{
#define MAX_CONTACTS 10

	pgBodyObject* ref = NULL, *inc = NULL;
	int face_id;
	pgVector2 gp[4];
	pgVector2 contacts[MAX_CONTACTS];
	int csize;
	pgVector2 normal;
	pgAABBBox clipBox;
	int overlap;
	pgVector2* pAcc;
	pgContact* contact;
	int i;


	overlap = _SAT_Select(selfBody, incidBody,
						  &ref, &inc,
						  &face_id, gp, &clipBox);

	if(!overlap) return 0;

	switch(face_id)
	{
	case CF_LEFT:
		PG_Set_Vector2(normal, -1, 0);
		assert(_Build_Contacts(gp, &clipBox, CA_X, contacts, &csize)); 
		break;
	case CF_BOTTOM:
		PG_Set_Vector2(normal, 0, -1);
		assert(_Build_Contacts(gp, &clipBox, CA_Y, contacts, &csize));
		break;
	case CF_RIGHT:
		PG_Set_Vector2(normal, 1, 0);
		assert(_Build_Contacts(gp, &clipBox, CA_X, contacts, &csize));
		break;
	case CF_TOP:
		PG_Set_Vector2(normal, 0, 1);
		assert(_Build_Contacts(gp, &clipBox, CA_Y, contacts, &csize));
		break;
	default:
		assert(0);
		break;
	}

	pAcc = PyObject_Malloc(sizeof(pgVector2));
	pAcc->real = pAcc->imag = 0;
	for(i = 0; i < csize; ++i)
	{
		contact = (pgContact*)PG_ContactNew(ref, inc);
		contact->pos = contacts[i];
		contact->normal = normal;
		c_rotate(&(contact->pos), ref->fRotation);
		contact->pos = c_sum(contact->pos, ref->vecPosition);
		c_rotate(&(contact->normal), ref->fRotation);

		contact->ppAccMoment = PyObject_Malloc(sizeof(pgVector2*));
		*(contact->ppAccMoment) = pAcc;
		contact->weight = csize;
		PyList_Append(contactList, (PyObject*)contact);
	}

	return 1;

}
