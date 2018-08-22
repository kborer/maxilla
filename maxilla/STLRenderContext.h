

#include "RenderContext.h"
#include <vector>

#include "JMatrix.h"

#ifndef _STLRenderContext_H
#define _STLRenderContext_H

class STLRenderContext : public CRenderContext
{
private:

	float _scale;
	FILE* _fp;

	std::vector<JMatrix> _txStack;

public:
	STLRenderContext();

	void open(char* path, unsigned int numTriangles);
	void close();


	virtual void PushMatrix();
	virtual void PopMatrix();
	virtual void Translatef(float x, float y, float z);
	virtual void Rotatef(float angle, float x, float y, float z);
	virtual void Scalef(float x, float y, float z);
	virtual void Triangle( Point* normal, Point* p1, Point* p2, Point* p3);
	virtual void TriangleSmooth( Point* p1, Point* p2, Point* p3);
	void Triangle( JVector& normal, Point* p1, Point* p2, Point* p3);


protected:

	JVector Apply(const JVector& v);
	void ReBuildMatrix();

	JMatrix _matrix;
	JMatrix* _currentMatrix;

};


#endif