#ifndef K_MAIN
#define K_MAIN

#include "General.h"
//---------------------------------------------------------------------------
struct Point3U : public BGR
{
    int cluster;     // cluster (or count)
    int minDist;     // default infinite dist to nearest cluster

    Point3U() : cluster(-1), minDist(__INT32_MAX__) {}

    inline void operator=(const BGR col)
    {
        b=col.b; g=col.g; r=col.r;
    }

    int distance(Point3U p)
    {
        return (p.b - b) * (p.b - b) + (p.g - g) * (p.g - g) + (p.r - r) * (p.r - r);
    }
};
//---------------------------------------------------------------------------
class Kmain
{
public:
    std::vector<Point3U> centroids;

    Kmain(void);
    Kmain(int NrOfClusters);
    void AddPoint(Point3U &p);
    void Run(void);
    inline size_t GetNumberOfPoints(void){ return points.size(); }
private:
    int Dif;                                //differance between two iterations
    size_t K;                               //number of clusters
    size_t N;                               //number of points
    std::vector<Point3U> points;
    std::vector<Point3U> centroids_t1;

    void InitializeCentroids(void);
    void AssignPointsToClusters(void);
    void UpdateCentroids(void);
};
//---------------------------------------------------------------------------
#endif // GENERAL_H_INCLUDED
