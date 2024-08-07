
// #include <windows.h>
#include <chrono>
#include <algorithm>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <omp.h>
#include <mutex>

#include "defines.h"
#include "MeshMetrics.h"
#include "ParallelMesh.h"
#include "FPPQ.h"
#include "Examples.h"
// ----------------------------------------------------------------------------

typedef std::chrono::high_resolution_clock Clock;


void exportObj(std::string rootPath, std::vector<glm::vec3> vertices, std::vector<glm::ivec3> faces, std::string ModelName, std::string suffix) {
    std::filesystem::create_directory(rootPath + "/Models");
    std::filesystem::create_directory(rootPath + "/Models/" + ModelName);
    std::ofstream outFile(rootPath + "/Models/" + ModelName + "/dec_" + suffix +".obj");

    outFile << "o " + suffix << std::endl;

    for (auto& vertex : vertices) {
        outFile << "v " + std::to_string(vertex.x) + " " + std::to_string(vertex.y) + " " + std::to_string(vertex.z) << std::endl;
    }
    for (auto& face : faces) {
        outFile << "f " + std::to_string(1 + face.x) + " " + std::to_string(1 + face.y) + " " + std::to_string(1 + face.z) << std::endl;
    }

    outFile.close();
}


int main()
{
    //FPPQExample();
    //meshDecimationExample();

    std::string methodName = typeid(PRIORITY_STRUCTURE).name();
    methodName = methodName.substr(methodName.find_last_of(" ") + 1);
    std::string meshName = MESH_PATH;
    meshName = meshName.substr(meshName.find_last_of("/") + 1);
    meshName = meshName.substr(0, meshName.find_last_of("."));

    std::string benchmarkName = methodName + "_" + meshName;

    int totalVerticies; //Total Verticies at load

#ifndef BENCHMARK
    auto t0 = Clock::now();
    std::cout << "Starting Load" << std::endl;
    auto& pMesh = ParallelMesh::getInstance();
    if(!pMesh.loadFromObj(std::filesystem::path(MESH_PATH))) {
        std::cout << "Could not load mesh: "
            << MESH_PATH << std::endl;
        return -1;
    }
   
#ifndef NO_HAUSDORFF
    std::vector<glm::vec3> originVertices;
    std::vector<glm::ivec3> originFaces;
    originVertices = pMesh.getVertices();
    originFaces = pMesh.getFaces();
#endif
    

    auto t1 = Clock::now();

    int threads;
#ifdef SINGLE_THREADED
    threads = 1;
#else
    threads = NUM_THREADS;
#endif // SINGLE_THREADED
    //{0.75f, .25f, .05f, .01f, .001f}
    float decimation = .001f;
    pMesh.computeQuadricErrorMatrices(threads);
    int finalVertexCount = pMesh.reduceVerticesTo(decimation, threads);
    pMesh.reduceVertices(finalVertexCount, threads);
    pMesh.deletePriorityStructure();

    std::cout << "Final Vertex Count: " << finalVertexCount << std::endl;

    auto t2 = Clock::now();
    std::cout << "Mesh decimation: "
        << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
        << " ms" << std::endl;
    std::pair<std::vector<glm::vec3>, std::vector<glm::vec3>> data = pMesh.getData();
    std::vector<glm::vec3> vertices = data.first;
    std::vector<glm::vec3> normals = data.second;

    auto t3 = Clock::now();
    std::cout << "Mesh decimation including load: "
        << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t0).count()
        << " ms" << std::endl;
#ifndef NO_HAUSDORFF
    {
        std::vector<glm::vec3> newVertices;
        std::vector<glm::ivec3> newFaces;
        newVertices = pMesh.getVertices();
        newFaces = pMesh.getFaces();
        double hausdorffMean, hausdorffMax;
        MeshMetrics::computeHausdorff(originVertices, originFaces, newVertices, newFaces, hausdorffMean, hausdorffMax);
        std::cout << "Hausdorff RMSE: "
            << hausdorffMean << std::endl;
        std::cout << "Hausdorff Max: "
            << hausdorffMax << std::endl;
        
        std::string hausdorff;
        {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(12) << hausdorffMean;

            hausdorff = ss.str();
            std::replace(hausdorff.begin(), hausdorff.end(), '.', ',');

        }

        std::string suffix = std::to_string(decimation) + "_" + std::to_string(threads) + "_" + methodName + "_HD"+ hausdorff;
        std::string percent;

        if (decimation >= 0.01f) {
            percent = std::to_string(int(decimation * 100)) + "%";
        }
        else {
            percent = "0," + std::to_string(int(decimation * 1000)) + "%";
        }
        bool refFound = pMesh.loadFromObj(OUTPUT_DIRECTORY + "/Models/" + meshName + "/" + meshName + percent + ".obj");
        std::vector<glm::vec3> refVertices;
        std::vector<glm::ivec3> refFaces;
        if (refFound) {
            refVertices = pMesh.getVertices();
            refFaces = pMesh.getFaces();
        }
        else {
            std::cout << "No reference for mesh similarity found: " << percent << std::endl;
        }
        double vRatio, fRatio;
        MeshMetrics::computeSimilarity(refVertices, refFaces, newVertices, newFaces, vRatio, fRatio);
        std::cout << "vRatio: "
            << vRatio << std::endl;
        std::cout << "fRatio: "
            << fRatio << std::endl;

        std::string vR;
        {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(12) << vRatio;

            vR = ss.str();
            std::replace(vR.begin(), vR.end(), '.', ',');

        }

        std::string fR;
        {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(12) << fRatio;

            fR = ss.str();
            std::replace(fR.begin(), fR.end(), '.', ',');

        }

        suffix = suffix + "_VR_" + vR + "_FR_" + fR;
#ifdef EXPORT
        exportObj(OUTPUT_DIRECTORY, newVertices, newFaces, meshName, suffix);
#endif // EXPORT
    }


#endif // !SINGLE_THREADED_EXTRAS
#else
#ifndef BENCHMARK_DEBUG
    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS); // Needs to run in admin mode otherwise gets only high_priority
#endif // !BENCHMARK_DEBUG
    //SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS); // Needs to run in admin mode otherwise gets only high_priority


    // unsigned int dwPriClass = GetPriorityClass(GetCurrentProcess());
    // std::cout << dwPriClass << std::endl;

    std::vector<std::chrono::microseconds> iterationTimesInit(countIterations);
    std::vector<std::chrono::microseconds> iterationTimesDecimation(countIterations);
    std::vector<std::chrono::microseconds> iterationTimesDelete(countIterations);
    std::vector<std::chrono::microseconds> iterationTimesComplete(countIterations);
    std::vector<double> hausdorffMean(countIterations);
    std::vector<double> hausdorffMax(countIterations);
    std::vector<double> vSim(countIterations);
    std::vector<double> fSim(countIterations);


    std::cout << "Starting Benchmark: " << benchmarkName
        << " Iterations: " << countIterations
#ifndef BENCHMARK_DEBUG
#else
        << " Desired Vertex Count: " << (int)(totalVerticies*(0.01*DECIMATION_PERCENT))
#endif
        << std::endl;
    auto t0 = Clock::now();
    auto& pMesh = ParallelMesh::getInstance();
    if(!pMesh.loadFromObj(std::filesystem::path(MESH_PATH))) {
        std::cout << "Could not load mesh: "
            << MESH_PATH << std::endl;
        return -1;
    }
    totalVerticies = pMesh.getVertices().size();
    std::cout << "Num of Verticies:" << pMesh.getVertices().size() << std::endl;
    std::cout << "Num of Faces:" << pMesh.getFaces().size() << std::endl;
    
    auto t1 = Clock::now();
    std::vector<glm::vec3> originVertices;
    std::vector<glm::ivec3> originFaces;
    originVertices = pMesh.getVertices();
    originFaces = pMesh.getFaces();

    std::vector<Vertex> verticesCopy;
    std::vector<int> adjacentVerticesIndicesCopy;
    std::vector<int> faceIndicesCopy;
    std::vector<glm::ivec3> facesCopy;
    pMesh.exportData(verticesCopy, adjacentVerticesIndicesCopy, faceIndicesCopy, facesCopy);

    auto t2 = Clock::now();
#ifdef SINGLE_THREADED
    std::vector<int> threads = { { 1 } };
#else
#ifdef BENCHMARK_DEBUG
    std::vector<int> threads = { { NUM_THREADS } };
#else
    //std::vector<int> threads = { {1, 2, 4, 8, 12, 16, 20, 24, 28, 30, 32 } };
    std::vector<int> threads = { { NUM_THREADS } };
    //std::vector<int> threads = { { 4 } };
#endif
#endif // SINGLE_THREADED
// #ifndef NO_HAUSDORFF
    std::vector<glm::vec3 > decimatedVertices;
    std::vector<glm::ivec3 > decimatedFaces;

//#endif // !NO_HAUSDORFF

#ifndef BENCHMARK_DEBUG
    std::vector<float> percentiles;
    if(meshName == "dragon")
        percentiles = { {.25f, .05f, .01f} };
        //percentiles = { {.05f, .04f, .03f, .02f, .01f} };
        //percentiles = { {.01f} };
    else
        percentiles = { {.05f, .01f, .001f} };
        //percentiles = { {.001f} };
    std::vector<std::chrono::microseconds> iterationTimesInitTotal(countIterations * threads.size() * percentiles.size());
    std::vector<std::chrono::microseconds> iterationTimesDecimationTotal(countIterations * threads.size() * percentiles.size());
    std::vector<std::chrono::microseconds> iterationTimesDeleteTotal(countIterations * threads.size() * percentiles.size());
    std::vector<std::chrono::microseconds> iterationTimesCompleteTotal(countIterations * threads.size() * percentiles.size());
    std::vector<double> hausdorffMeanTotal(countIterations * threads.size() * percentiles.size());
    std::vector<double> hausdorffMaxTotal(countIterations * threads.size() * percentiles.size());
    std::vector<double> vSimTotal(countIterations * threads.size() * percentiles.size());
    std::vector<double> fSimTotal(countIterations * threads.size() * percentiles.size());

    std::atomic<int> barrierCounter;

    for (int k = 0; k < percentiles.size(); k++)
#endif
    {
        // Similarity reference
        std::string percent;
#ifdef BENCHMARK_DEBUG
        int decimation = (int)(totalVerticies *(0.01*DECIMATION_PERCENT));
        percent = std::to_string(int(decimation));
#else
        if(percentiles[k] >= 0.01f) {
            percent = std::to_string(int(percentiles[k] * 100)) + "%";
        } else {
            percent = "0," + std::to_string(int(percentiles[k] * 1000)) + "%";
        }
#endif
        bool refFound = pMesh.loadFromObj(OUTPUT_DIRECTORY + "/Models/" + meshName + "/" + meshName + percent + ".obj");
        std::vector<glm::vec3> refVertices;
        std::vector<glm::ivec3> refFaces;
        if(refFound) {
            refVertices = pMesh.getVertices();
            refFaces = pMesh.getFaces();
        } else {
            std::cout << "No reference for mesh similarity found: "  << percent << std::endl;
        }
        //std::cout << "Number of threads: " << threads.size() << std::endl;
        for (int j = 0; j < threads.size(); j++) {
            for (int i = 0; i < countIterations; i++) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                pMesh.loadFromData(verticesCopy, adjacentVerticesIndicesCopy, faceIndicesCopy, facesCopy);
                //barrierCounter++;
                auto tx0 = Clock::now();
                pMesh.computeQuadricErrorMatrices(threads[j]);
                //barrierCounter++;
                auto tx1 = Clock::now();
#ifndef BENCHMARK_DEBUG
                int finalVertexCount = pMesh.reduceVerticesTo(percentiles[k], threads[j]);
#else
                
                int finalVertexCount = pMesh.reduceVerticesTo(decimation, threads[j]);
#endif
                // std::cout << finalVertexCount << std::endl;
                
                //barrierCounter++;
                auto tx2 = Clock::now();
                //pMesh.reduceVertices(finalVertexCount, threads[j]);
                pMesh.deletePriorityStructure();
                //barrierCounter++;
                auto tx3 = Clock::now();
                //barrierCounter++;
                iterationTimesInit[i] = std::chrono::duration_cast<std::chrono::microseconds>(tx1 - tx0);
                iterationTimesDecimation[i] = std::chrono::duration_cast<std::chrono::microseconds>(tx2 - tx1);
                iterationTimesDelete[i] = std::chrono::duration_cast<std::chrono::microseconds>(tx3 - tx2);
                iterationTimesComplete[i] = std::chrono::duration_cast<std::chrono::microseconds>(tx3 - tx0);


#if defined NO_HAUSDORFF
                hausdorffMean[i] = 0;
                hausdorffMax[i] = 0;
                
                //Required for exporting
                decimatedVertices = pMesh.getVertices();
                decimatedFaces = pMesh.getFaces();
                
                
#else
                decimatedVertices = pMesh.getVertices();
                decimatedFaces = pMesh.getFaces();
                double mean, max;
                MeshMetrics::computeHausdorff(originVertices, originFaces, decimatedVertices, decimatedFaces, mean, max);
                hausdorffMean[i] = mean;
                hausdorffMax[i] = max;
                double vRatio = 0, fRatio = 0;
                if(refFound)
                    MeshMetrics::computeSimilarity(refVertices, refFaces, decimatedVertices, decimatedFaces, vRatio, fRatio);
                vSim[i] = vRatio;
                fSim[i] = fRatio;
                //geometricDeviation(originVertices, originFaces, decimatedVertices, decimatedFaces);
#endif // !SINGLE_THREADED_EXTRAS

                std::cout << "Iteration: "
                    << i << ": Decimation: "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(iterationTimesDecimation[i]).count()
                    << " ms Complete: "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(tx3 - tx0).count() << " ms" <<
                    // " Hd RSME: " << hausdorffMean[i] <<
                    // " Hd Max: " << hausdorffMax[i] <<
                    // " vSim: " << vSim[i] <<
                    // " fSim: " << fSim[i] <<
                    std::endl;


#ifdef EXPORT
                //if(hausdorffMean[i] < 0.0000185 *1.005 && hausdorffMean[i] > 0.0000185 *0.995)
                {
                    std::stringstream ss;
                    //ss << std::fixed << std::setprecision(12) << hausdorffMean[i];

                    std::string hausdorff = ss.str();
                    std::replace(hausdorff.begin(), hausdorff.end(), '.', ',');


                    std::string suffix = methodName+"_thr"+std::to_string(threads[j])+"_"+std::to_string((int)DECIMATION_PERCENT)+"_"+meshName;

                    exportObj(OUTPUT_DIRECTORY, decimatedVertices, decimatedFaces, meshName, suffix);
                    std::cout << "Exported" << std::endl;

                }
#endif



            }
            auto t3 = Clock::now();
            std::string percent;
#ifdef BENCHMARK_DEBUG
            percent = std::to_string(DECIMATION_PERCENT);
#endif
            // std::cout << benchmarkName << std::endl << "Total Time: "
            //     << std::chrono::duration_cast<std::chrono::seconds>(t3 - t0).count() << "s"
            //     << " Count Iterations: " << countIterations
            //     << " Desired Vertex Count: " << percent
            //     << std::endl;
            double summedHausdorffMean = 0;
            double summedHausdorffMax = 0;
            double summedvSim = 0;
            double summedfSim = 0;
            std::chrono::microseconds summedDecimation = std::chrono::microseconds(0);
            std::chrono::microseconds summedComplete = std::chrono::microseconds(0);
            std::chrono::microseconds summedInit = std::chrono::microseconds(0);
            std::chrono::microseconds summedDelete = std::chrono::microseconds(0);
            for (int i = 0; i < countIterations; i++) {
                summedHausdorffMean += hausdorffMean[i];
                summedHausdorffMax += hausdorffMax[i];
                summedvSim += vSim[i];
                summedfSim += fSim[i];
                summedDecimation += iterationTimesDecimation[i];
                summedComplete += iterationTimesComplete[i];
                summedInit += iterationTimesInit[i];
                summedDelete += iterationTimesDelete[i];
            }
            std::sort(hausdorffMean.begin(), hausdorffMean.end());
            std::sort(hausdorffMax.begin(), hausdorffMax.end());
            std::sort(iterationTimesDecimation.begin(), iterationTimesDecimation.end());
            std::sort(iterationTimesComplete.begin(), iterationTimesComplete.end());
            std::sort(iterationTimesInit.begin(), iterationTimesInit.end());
            std::sort(iterationTimesDelete.begin(), iterationTimesDelete.end());

            int median = countIterations / 2;
            int tenth = countIterations / 10;

            // std::cout << "Hausdorf RSME: "
            //     << "min: " << hausdorffMean[0]
            //     << " max: " << hausdorffMean[countIterations - 1]
            //     << " med: " << hausdorffMean[median]
            //     << " avg: " << summedHausdorffMean / countIterations
            //     << " 10%: " << hausdorffMean[tenth]
            //     << " 90%: " << hausdorffMean[countIterations - 1 - tenth]
            //     << std::endl;
            // std::cout << "Hausdorf Max: "
            //     << "min: " << hausdorffMax[0]
            //     << " max: " << hausdorffMax[countIterations - 1]
            //     << " med: " << hausdorffMax[median]
            //     << " avg: " << summedHausdorffMax / countIterations
            //     << " 10%: " << hausdorffMax[tenth]
            //     << " 90%: " << hausdorffMax[countIterations - 1 - tenth]
            //     << std::endl;
            // std::cout << "vSim: "
            //     << "min: " << vSim[0]
            //     << " max: " << vSim[countIterations - 1]
            //     << " med: " << vSim[median]
            //     << " avg: " << summedvSim / countIterations
            //     << " 10%: " << vSim[tenth]
            //     << " 90%: " << vSim[countIterations - 1 - tenth]
            //     << std::endl;
            // std::cout << "fSim: "
            //     << "min: " << fSim[0]
            //     << " max: " << fSim[countIterations - 1]
            //     << " med: " << fSim[median]
            //     << " avg: " << summedfSim / countIterations
            //     << " 10%: " << fSim[tenth]
            //     << " 90%: " << fSim[countIterations - 1 - tenth]
            //     << std::endl;
            // std::cout << "Init Time: "
            //     << "min: " << iterationTimesInit[0].count() / 1000
            //     << " max: " << iterationTimesInit[countIterations - 1].count() / 1000
            //     << " med: " << iterationTimesInit[median].count() / 1000
            //     << " avg: " << (summedInit / countIterations).count() / 1000
            //     << " 10%: " << iterationTimesInit[tenth].count() / 1000
            //     << " 90%: " << iterationTimesInit[countIterations - 1 - tenth].count() / 1000
            //     << std::endl;
            // std::cout << "Decimation Time: "
            //     << "min: " << iterationTimesDecimation[0].count() / 1000
            //     << " max: " << iterationTimesDecimation[countIterations - 1].count() / 1000
            //     << " med: " << iterationTimesDecimation[median].count() / 1000
            //     << " avg: " << (summedDecimation / countIterations).count() / 1000
            //     << " 10%: " << iterationTimesDecimation[tenth].count() / 1000
            //     << " 90%: " << iterationTimesDecimation[countIterations - 1 - tenth].count() / 1000
            //     << std::endl;
            // std::cout << "Delete Time: "
            //     << "min: " << iterationTimesDelete[0].count() / 1000
            //     << " max: " << iterationTimesDelete[countIterations - 1].count() / 1000
            //     << " med: " << iterationTimesDelete[median].count() / 1000
            //     << " avg: " << (summedDelete / countIterations).count() / 1000
            //     << " 10%: " << iterationTimesDelete[tenth].count() / 1000
            //     << " 90%: " << iterationTimesDelete[countIterations - 1 - tenth].count() / 1000
            //     << std::endl;
            // std::cout << "Complete Time: "
            //     << "min: " << iterationTimesComplete[0].count() / 1000
            //     << " max: " << iterationTimesComplete[countIterations - 1].count() / 1000
            //     << " med: " << iterationTimesComplete[median].count() / 1000
            //     << " avg: " << (summedComplete / countIterations).count() / 1000
            //     << " 10%: " << iterationTimesComplete[tenth].count() / 1000
            //     << " 90%: " << iterationTimesComplete[countIterations - 1 - tenth].count() / 1000
            //     << std::endl;

            // std::string benchmarkToken = benchmarkName + "_" + std::to_string(threads[j]);
            // benchmarkToken += "_" + percent;
            // benchmarkToken += "_" + std::to_string(countIterations) + "it";

            
            // std::filesystem::create_directory(OUTPUT_DIRECTORY);
            // std::filesystem::create_directory(OUTPUT_DIRECTORY + benchmarkName);
            // int retryCount = 0;
            // std::ofstream resultFile(OUTPUT_DIRECTORY + benchmarkName + "/" + benchmarkToken + "_result.txt");
            // while (!resultFile.is_open())
            // {
            //     retryCount++;
            //     resultFile.open(OUTPUT_DIRECTORY + "/Results/" + benchmarkName + "/" + benchmarkToken + "_result_" + std::to_string(retryCount) + ".txt");
            // }
            // {
                std::cout << std::endl << "Iterations: " << std::to_string(countIterations) << std::endl;
#ifdef BENCHMARK_DEBUG
                std::cout << "Desired Vertex Count: " << (int)(totalVerticies *(0.01*DECIMATION_PERCENT)) << std::endl;
                std::cout << "Decimation Percent: " << DECIMATION_PERCENT << std::endl;
#endif
                // resultFile << "Hausdorf RMSE: "
                //     << "min: " << hausdorffMean[0]
                //     << " max: " << hausdorffMean[countIterations - 1]
                //     << " med: " << hausdorffMean[median]
                //     << " avg: " << summedHausdorffMean / countIterations
                //     << " 10%: " << hausdorffMean[tenth]
                //     << " 90%: " << hausdorffMean[countIterations - 1 - tenth]
                //     << std::endl;
                // resultFile << "Hausdorf Max: "
                //     << "min: " << hausdorffMax[0]
                //     << " max: " << hausdorffMax[countIterations - 1]
                //     << " med: " << hausdorffMax[median]
                //     << " avg: " << summedHausdorffMax / countIterations
                //     << " 10%: " << hausdorffMax[tenth]
                //     << " 90%: " << hausdorffMax[countIterations - 1 - tenth]
                //     << std::endl;
                // resultFile << "vSim: "
                //     << "min: " << vSim[0]
                //     << " max: " << vSim[countIterations - 1]
                //     << " med: " << vSim[median]
                //     << " avg: " << summedvSim / countIterations
                //     << " 10%: " << vSim[tenth]
                //     << " 90%: " << vSim[countIterations - 1 - tenth]
                //     << std::endl;
                // resultFile << "fSim: "
                //     << "min: " << fSim[0]
                //     << " max: " << fSim[countIterations - 1]
                //     << " med: " << fSim[median]
                //     << " avg: " << summedfSim / countIterations
                //     << " 10%: " << fSim[tenth]
                //     << " 90%: " << fSim[countIterations - 1 - tenth]
                //     << std::endl;
                std::cout << "Decimation Time: (ms)"
                    << "min: " << iterationTimesDecimation[0].count()/1000
                    << " max: " << iterationTimesDecimation[countIterations - 1].count()/1000
                    << " med: " << iterationTimesDecimation[median].count()/1000
                    << " avg: " << (summedDecimation / countIterations).count()/1000
                    << " 10%: " << iterationTimesDecimation[tenth].count()/1000
                    << " 90%: " << iterationTimesDecimation[countIterations - 1 - tenth].count()/1000
                    << std::endl;
                std::cout << "Complete Time: (microseconds)"
                    << "min: " << iterationTimesComplete[0].count()/1000
                    << " max: " << iterationTimesComplete[countIterations - 1].count()/1000
                    << " med: " << iterationTimesComplete[median].count()/1000
                    << " avg: " << (summedComplete / countIterations).count()/1000
                    << " 10%: " << iterationTimesComplete[tenth].count()/1000
                    << " 90%: " << iterationTimesComplete[countIterations - 1 - tenth].count()/1000
                    << std::endl;
            }
            

#ifndef BENCHMARK_DEBUG
            for(int i = 0; i < countIterations; i++) {
                iterationTimesInitTotal[k * threads.size() * countIterations + j * countIterations + i] = iterationTimesInit[i];
                iterationTimesDecimationTotal[k * threads.size() * countIterations + j * countIterations + i] = iterationTimesDecimation[i];
                iterationTimesDeleteTotal[k * threads.size() * countIterations + j * countIterations + i] = iterationTimesDelete[i];
                iterationTimesCompleteTotal[k * threads.size() * countIterations + j * countIterations + i] = iterationTimesComplete[i];
                hausdorffMeanTotal[k * threads.size() * countIterations + j * countIterations + i] = hausdorffMean[i];
                hausdorffMaxTotal[k * threads.size() * countIterations + j * countIterations + i] = hausdorffMax[i];
                vSimTotal[k * threads.size() * countIterations + j * countIterations + i] = vSim[i];
                fSimTotal[k * threads.size() * countIterations + j * countIterations + i] = fSim[i];
            }
#endif
        //     resultFile.close();
        // }

    }
    std::pair<std::vector<glm::vec3>, std::vector<glm::vec3>> data = pMesh.getData();
    std::vector<glm::vec3> vertices = data.first;
    std::vector<glm::vec3> normals = data.second;

#endif // !BENCHMARK
    std::vector<glm::vec3> newVertices;
    std::vector<glm::ivec3> newFaces;
    newVertices = pMesh.getVertices();
    newFaces = pMesh.getFaces();



    return 0;
}