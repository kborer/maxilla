

#include "Point.h"

#ifndef _RENDER_CONTEXT_H
#define _RENDER_CONTEXT_H


class CRenderContext
{

public:
	virtual void PushMatrix()=0;
	virtual void PopMatrix()=0;

	virtual void Translatef(float x, float y, float z)=0;
	virtual void Rotatef(float angle, float x, float y, float z)=0;
	virtual void Scalef(float x, float y, float z)=0;

	virtual void Triangle( Point* normal, Point* p1, Point* p2, Point* p3) =0;
	virtual void TriangleSmooth( Point* p1, Point* p2, Point* p3) =0;

};

#endif