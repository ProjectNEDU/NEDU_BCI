#include "CNode.h"

double CNode::GetEucDistance(const vector<double> &vecInput)
{
	double distance = 0;
  
	for(int i = 0; i < m_dWeights.size(); i++)
	{
		distance += (vecInput[i] - m_dWeights[i]) * (vecInput[i] - m_dWeights[i]);
	}

	return sqrt(distance);
}

void CNode::AdjustWeights(
	const vector<double> &vecTarget,
    const double learningRate,
    const double influence
) {
	for(int w = 0; w < vecTarget.size(); w++)
	{
		m_dWeights[w] += learningRate * influence * (vecTarget[w] - m_dWeights[w]);
	}
}