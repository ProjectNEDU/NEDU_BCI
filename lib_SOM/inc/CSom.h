#ifndef CSOM_H_
#define CSOM_H_

#include <vector>

using namespace std;

#include "CNeuron.h"
#include "constants.h"


class CSom
{

private:

	vector<CNeuron> m_SOM;				// the neurons representing the Self Organizing Map
	CNeuron* m_pWinningNeuron;		    // this holds the address of the BMU of the current iteration
	double m_dMapRadius;				// this is the topological 'radius' of the feature map
	double m_dTimeConstant;				// used in the calculation of the neighbourhood width of influence
	int m_iNumIterations;				// the number of training iterations
	int m_iIterationCount;				// keeps track of what iteration the epoch method has reached
	double m_dNeighbourhoodRadius;		// the current width of the winning node's area of influence
	double m_dInfluence;				// how much the learning rate is adjusted for nodes within the area of influence
	double m_dLearningRate;				// the learning rate
	bool m_bDone;						// set true when training is finished

    /*
    * represents an input vector to each neuron in the network
    * and calculates the Euclidian distance between the vectors
    * of each neuron. it returns a pointer to the BMU
    */
	CNeuron* FindBestMatchingUnit(const vector<double> &vecInput);

    /*
    * gets the gaussian distance between the BMU and
    * another neuron from the neighborhood
    */
	inline double GetGaussianDistance(const double dist, const double sigma);


public:

	CSom():
        m_pWinningNeuron(NULL),
		m_iIterationCount(1),
		m_iNumIterations(0),
		m_dTimeConstant(0),
		m_dMapRadius(0),
		m_dNeighbourhoodRadius(0),
		m_dInfluence(0),
		m_dLearningRate(constStartLearningRate),
		m_bDone(false)
	{}

    /*
    * creates and initializes the network
    */
	void Create(
		int cxClient,
		int cyClient,
		int CellsUp,
		int CellsAcross,
		int NumIterations
	);

    /*
    * taking a vector of input vectors, choses one at random
    * and runs the network through one training set
    */
	bool Learn(const vector<vector<double>> &data);

    
	bool FinishedTraining() const { return m_bDone; }

};

#endif