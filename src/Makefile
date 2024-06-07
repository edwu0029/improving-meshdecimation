all: compile
H_FILES = defines.h ErrorMetric.h Examples.h FPPQ.h MeshMetrics.h MultiQueue.h ParallelMesh.h PriorityQueueSingleThreaded.h QuadricErrorMetric.h tiny_obj_loader.h Vertex.h
CPP_FILES = FPPQMesh.cpp MeshSimplification.cpp MultiQueue.cpp ParallelMesh.cpp PriorityQueueSingleThreaded.cpp QuadricErrorMetric.cpp Vertex.cpp
CXXFLAGS = -g -Wall -std=c++2a 
OPEN_MP_FLAGS = -fopenmp

LINKING_FLAGS = -fopenmp -ltbb

compile: setup
	g++ -c ${CXXFLAGS} ${OPEN_MP_FLAGS} FPPQMesh.cpp -o FPPQMesh.o
	g++ -c ${CXXFLAGS} ${OPEN_MP_FLAGS} MeshSimplification.cpp -o MeshSimplification.o
	g++ -c ${CXXFLAGS} ${OPEN_MP_FLAGS} MultiQueue.cpp -o MultiQueue.o
	g++ -c ${CXXFLAGS} ${OPEN_MP_FLAGS} ParallelMesh.cpp -o ParallelMesh.o
	g++ -c ${CXXFLAGS} ${OPEN_MP_FLAGS} PriorityQueueSingleThreaded.cpp -o PriorityQueueSingleThreaded.o
	g++ -c ${CXXFLAGS} ${OPEN_MP_FLAGS} QuadricErrorMetric.cpp -o QuadricErrorMetric.o
	g++ -c ${CXXFLAGS} ${OPEN_MP_FLAGS} Vertex.cpp -o Vertex.o
	
	g++ ${LINKING_FLAGS} -ltbb FPPQMesh.o MeshSimplification.o MultiQueue.o ParallelMesh.o PriorityQueueSingleThreaded.o QuadricErrorMetric.o Vertex.o -o mq_main

setup: ${H_FILES}

clean:
	rm -rf mq_main
_PHONY: hello clean