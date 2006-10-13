#ifndef NORMALIZER_H_
#define NORMALIZER_H_

class SetHandler;

class Normalizer
{
public:
	virtual ~Normalizer();
    virtual void setSet(SetHandler *){;}
    virtual void normalize(const double *in,double* out){;}
    virtual void unnormalizeweight(const double *in,double* out){;}
    virtual void normalizeweight(const double *in,double* out){;}
    static Normalizer * getNew();
    static void setType(int type);
	const static int UNI = 0;
	const static int STDV = 1;
protected:
	Normalizer();
	static int subclass_type;
};

#endif /*NORMALIZER_H_*/
