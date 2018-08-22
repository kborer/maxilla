

#include "STLRenderContext.h"




typedef struct {
	float nx,ny,nz;
	float x1,y1,z1;
	float x2,y2,z2;
	float x3,y3,z3;
} STLTRI;

STLRenderContext::STLRenderContext()
{
	_scale = 1000.f;
	_currentMatrix = NULL;
}

void STLRenderContext::open(char* path, unsigned int numTriangles)
{	
	_fp = fopen(path, "wb");

	//write header
	char* header[80];
	memset(&header, 0xFF, sizeof(char) * 80);
	fwrite(header, sizeof(char), 80, _fp);

	//tri count
	fwrite(&numTriangles, sizeof(unsigned int), 1, _fp);
}

void STLRenderContext::close()
{
	fclose(_fp);
}


void STLRenderContext::PushMatrix() {
	
	JMatrix m = JMatrix::identity;
	_txStack.push_back(m);	
	_currentMatrix = NULL;
}

void STLRenderContext::PopMatrix() {
	_txStack.pop_back();
	_currentMatrix = NULL;
}

void STLRenderContext::Translatef(float x, float y, float z) {
	JMatrix m = _txStack.back();
	m.Translate(JVector_Create(x,y ,z ));	
	_txStack.pop_back();
	_txStack.push_back(m);
	_currentMatrix = NULL;
}

void STLRenderContext::Rotatef(float angle, float x, float y, float z) {

	float radians = JMath_Deg2Rad(angle);

	JMatrix m = _txStack.back();
	m.Rotate(JVector_Create(x,y,z), radians);	
	_txStack.pop_back();
	_txStack.push_back(m);
	_currentMatrix = NULL;
}

void STLRenderContext::Scalef(float x, float y, float z) {
	JMatrix m = _txStack.back();
	m.Scale(JVector_Create(x,y,z));	
	_txStack.pop_back();
	_txStack.push_back(m);
	_currentMatrix = NULL;
}

void STLRenderContext::Triangle( Point* normal, Point* p1, Point* p2, Point* p3)
{
	if(normal)
		Triangle(JVector_Create(normal->x, normal->y,normal->z),p1,p2,p3);
	else
		TriangleSmooth(p1,p2,p3);
}

void STLRenderContext::Triangle( JVector& normal, Point* p1, Point* p2, Point* p3)
{	
	STLTRI t;
	memset(&t,0,sizeof(STLTRI));

	JVector n = Apply(normal);
	n.Normalize();
	t.nx = n.x; t.ny = n.y; t.nz = n.z;
	
	JVector v1 = Apply(JVector_Create(p1->x,p1->y,p1->z));
	JVector v2 = Apply(JVector_Create(p2->x,p2->y,p2->z));
	JVector v3 = Apply(JVector_Create(p3->x,p3->y,p3->z));

	t.x1 = v1.x * _scale; t.y1 = v1.y * _scale; t.z1 = v1.z * _scale;
	t.x2 = v2.x * _scale; t.y2 = v2.y * _scale; t.z2 = v2.z * _scale;
	t.x3 = v3.x * _scale; t.y3 = v3.y * _scale; t.z3 = v3.z * _scale;

	fwrite(&t, sizeof(STLTRI), 1, _fp);

	//atrributes
	char attr[2];
	attr[0] = 0;
	attr[1] = 0;
	fwrite(attr, sizeof(char), 2, _fp);
}

void STLRenderContext::TriangleSmooth( Point* p1, Point* p2, Point* p3)
{
	JVector normal = JVector_Cross(JVector_Create(p1->x,p1->y,p1->z), JVector_Create(p2->x,p2->y,p2->z));
	Triangle(normal,p1,p2,p3);
}



void STLRenderContext::ReBuildMatrix()
{	
	_matrix = JMatrix::identity;
	_currentMatrix = &_matrix;

	std::vector<JMatrix>::reverse_iterator it = _txStack.rbegin();
	for(;it != _txStack.rend(); it++)
	{
		JMatrix m = *it;
		_currentMatrix->Multiply(m);		
	}
}

JVector STLRenderContext::Apply(const JVector& v)
{
	if(_currentMatrix == NULL)
		ReBuildMatrix();

	JVector c = JMath_ApplyMatrix(v, *_currentMatrix);

	return c;
}


