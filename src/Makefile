all: mq_main
H_FILES = defines.h ErrorMetric.h Examples.h FPPQ.h MeshMetrics.h MultiQueue.h ParallelMesh.h PriorityQueueSingleThreaded.h QuadricErrorMetric.h tiny_obj_loader.h Vertex.h UpdateMultiQueue.h
CPP_FILES = FPPQMesh.cpp MeshSimplification.cpp MultiQueue.cpp ParallelMesh.cpp PriorityQueueSingleThreaded.cpp QuadricErrorMetric.cpp Vertex.cpp UpdateMultiQueue.cpp 
CXXFLAGS = -g -Wall -std=c++2a 
OPEN_MP_FLAGS = -fopenmp
LINKING_FLAGS = -fopenmp -ltbb

OBJECTS = FPPQMesh.o MeshSimplification.o MultiQueue.o ParallelMesh.o PriorityQueueSingleThreaded.o QuadricErrorMetric.o Vertex.o UpdateMultiQueue.o

%.o: %.cpp
	g++ -c ${CXXFLAGS} ${OPEN_MP_FLAGS} $< -o $@

mq_main: ${OBJECTS}
	g++ ${OBJECTS} -o mq_main ${LINKING_FLAGS}

clean:
	rm -rf mq_main ${OBJECTS}
_PHONY: hello clean
