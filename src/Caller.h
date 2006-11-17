#ifndef CALLER_H_
#define CALLER_H_


class Caller
{
public:
	Caller();
	virtual ~Caller();
    void step(SetHandler & train,double * w, double Cpos, double Cneg, double fdr);
    void trainEm(double * w);
    void xvalidate_step(double *w);
    void xvalidate(double *w);
	static string greeter();
	string extendedGreeter();
    bool parseOptions(int argc, char **argv);
    void printWeights(ostream & weightStream, double * weights);
    int run();
protected:
    Normalizer * pNorm;
    Scores scores;
    string modifiedFN;
    string modifiedShuffledFN;
    string forwardFN;
    string shuffledFN;
    string shuffled2FN;
    string shuffledWC;
    string rocFN;
    string gistFN;
    string weightFN;
    string call;
    bool gistInput;
    double selectedfdr;
    double selectedCpos;
    double selectedCneg;
    int niter;
    time_t startTime;
    clock_t startClock;
    const static unsigned int xval_fold;
    const static double test_fdr;
    static int xv_type; // 0 = None, 1 = intra-itereration, 2 = whole-procedure
    vector<SetHandler> xv_train,xv_test;
    vector<double> xv_fdrs,xv_cposs,xv_cfracs;
    SetHandler trainset,testset;
};

#endif /*CALLER_H_*/
