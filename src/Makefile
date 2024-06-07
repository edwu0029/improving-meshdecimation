all: compile
H_FILES = defines.h ErrorMetric.h Examples.h FPPQ.h MeshMetrics.h MultiQueue.h ParallelMesh.h PriorityQueueSingleThreaded.h QuadricErrorMetric.h tiny_obj_loader.h Vertex.h
CPP_FILES = FPPQMesh.cpp MeshSimplification.cpp MultiQueue.cpp ParallelMesh.cpp PriorityQueueSingleThreaded.cpp QuadricErrorMetric.cpp Vertex.cpp

compile: setup
	g++ -c FPPQMesh.cpp -o FPPQMesh.o
	g++ -c MeshSimplification.cpp -o MeshSimplification.o
	g++ -c MultiQueue.cpp -o MultiQueue.o
	g++ -c ParallelMesh.cpp -o ParallelMesh.o
	g++ -c PriorityQueueSingleThreaded.cpp -o PriorityQueueSingleThreaded.o
	g++ -c QuadricErrorMetric.cpp -o QuadricErrorMetric.o
	g++ -c Vertex.cpp -o Vertex.o
	
	g++ FPPQMesh.o MeshSimplification.o MultiQueue.o ParallelMesh.o PriorityQueueSingleThreaded.o QuadricErrorMetric.o Vertex.o -o mq_main

setup: ${H_FILES}

clean:
	rm -rf mq_main
_PHONY: hello clean