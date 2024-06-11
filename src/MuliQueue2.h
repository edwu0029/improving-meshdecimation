/*
    Implementation of https://dl-acm-org.myaccess.library.utoronto.ca/doi/10.1145/2755573.2755616
    
*/
#pragma once

#include <atomic>
#include <vector>
#include <queue>
#include <functional>

#include "defines.h"

class MultiQueue2{
public:
    //Quadric Errors have type std::pair<float, int>>
    std::vector<std::priority_queue<std::pair<float, int>>>queues;
    std::vector<double>best_error;
    int num_queues;

    MultiQueue2();

    int insert(int vertexID); //Insert the vertex into the muliqueue
    int pop(); //Returns the vertexID of popped node
}