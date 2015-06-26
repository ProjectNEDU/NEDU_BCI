#ifndef CONSTANTS_H_
#define CONSTANTS_H_

const int constNumCellsAcross = 40;
const int constNumCellsDown = 40;


//number of weights each node must contain. One for each element of 
//the input vector. In this example it is 3 because a color is
//represented by its red, green and blue components. (RGB)
const int     constSizeOfInputVector   = 3;

//the number of epochs desired for the training
const int    constNumIterations       = 1000;

//the value of the learning rate at the start of training
const double constStartLearningRate   = 0.1;


#endif