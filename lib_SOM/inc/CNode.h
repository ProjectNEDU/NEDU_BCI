#ifndef CNode_H_
#define CNode_H_

#include <vector>
#include <string>

#include "utils.h"

using namespace std;

class CNode
{

private:

	vector<double> m_dWeights;
	double m_dPosX;
	double m_dPosY;


public:

	CNode(double posX, double posY, int numWeights):
		m_dPosX(posX),
		m_dPosY(posY)
	{
		/*
		* initialize the weights to small random variables
		*/
		for(int w = 0; w < numWeights; w++)
		{
			m_dWeights.push_back(RandFloat());
		}
	}
	
	/*
	* returns the euclidean distance (squared)
	* between the node's weights and the input vector
	*/
    inline double GetEucDistance(
		const vector<double> &vecInput
	);
	
	/*
	* given a learning rate and a target vector,
	* this function adjusts the node's weights accordingly
	*/
	inline void AdjustWeights(
		const vector<double> &vecTarget,
		const double learningRate,
		const double influence
	);

	double getPosX() const { return m_dPosX; }
	double getPosY() const { return m_dPosY; }

};

#endif