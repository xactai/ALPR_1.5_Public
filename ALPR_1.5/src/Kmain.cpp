#include <iostream>
#include <vector>
#include <cmath>
#include "Kmain.h"

using namespace cv;
using namespace std;

//---------------------------------------------------------------------------
Kmain::Kmain(void)
{
    K=1;
    points.clear();
    centroids.resize(K);
    centroids_t1.resize(K);
}
//---------------------------------------------------------------------------
Kmain::Kmain(int NrOfClusters)
{
    K=NrOfClusters;
    points.clear();
    centroids.resize(K);
    centroids_t1.resize(K);
}
//---------------------------------------------------------------------------
void Kmain::InitializeCentroids(void)
{
    // For simplicity, randomly select k points from the dataset as initial centroids.
    for(size_t i=0; i<K; i++){
        centroids[i] = points[(i*N)/K];
    }
}
//---------------------------------------------------------------------------
void Kmain::AssignPointsToClusters(void)
{
    int dist;

    for(size_t n=0;n<N;n++){
        for(size_t k=0; k<K; k++){
            dist = points[n].distance(centroids[k]);
            if(dist < points[n].minDist) {
                points[n].cluster = k;
                points[n].minDist = dist;
            }
        }
    }
}
//---------------------------------------------------------------------------
void Kmain::UpdateCentroids(void)
{
    size_t Ct;
    std::vector<double> sumB, sumG, sumR;

    // Initialise with zeroes
    for(size_t k=0; k<K;k++){
        centroids[k].cluster=0;
        sumB.push_back(0.0);
        sumG.push_back(0.0);
        sumR.push_back(0.0);
    }

    // Iterate over points to append data to centroids
    for(Point3U& p : points){
        Ct=p.cluster;
        if(Ct<K){
            centroids[Ct].cluster++;
            sumB[Ct]+=p.b;
            sumG[Ct]+=p.g;
            sumR[Ct]+=p.r;
        }
        p.minDist=__INT32_MAX__;  // reset distance
    }
    // Store the previous centroids
    for(size_t k=0; k<K;k++){
        centroids_t1[k].b = centroids[k].b;
        centroids_t1[k].g = centroids[k].g;
        centroids_t1[k].r = centroids[k].r;
    }
    // Compute the new centroids
    for(size_t k=0; k<K;k++){
        Ct=centroids[k].cluster;
        if(Ct>0){
            centroids[k].b = sumB[k] / Ct;
            centroids[k].g = sumG[k] / Ct;
            centroids[k].r = sumR[k] / Ct;
        }
    }
    // Compute differance
    Dif=0;
    for(size_t k=0; k<K;k++){
        Dif+=centroids_t1[k].distance(centroids[k]);
    }
}
//---------------------------------------------------------------------------
void Kmain::AddPoint(Point3U &p)
{
    points.push_back(p);
}
//---------------------------------------------------------------------------
void Kmain::Run(void)
{
    N=points.size();
    InitializeCentroids();

    for(int i=0; i<250; ++i) {
        AssignPointsToClusters();
        UpdateCentroids();
        if(Dif==0) break;
    }
}
//---------------------------------------------------------------------------

