#pragma once

#include "Solver.h"


class BM : public Solver
{
public:
    BM(int h, int w, int s, int d);
	~BM();

	virtual void process(Mat &img_l, Mat &img_r);
    virtual void process(Mat &img_l, Mat &img_r, Mat &sky_mask, Mat &sky_mask_beta);

};

