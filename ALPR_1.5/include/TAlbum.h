#ifndef TALBUM_H
#define TALBUM_H
#include "TWorker.h"
//----------------------------------------------------------------------------------------
class TAlbum
{
private:
protected:
    TWorker* Ar[ALBUM_SIZE];                        ///< array of pointers to workers
public:
    TAlbum();
    virtual ~TAlbum();

    void Update(TMapObject &Mo,cv::Mat &Img);       ///< add a new picture of a vehicle
    void Execute(TMapObject &Mo);                   ///< get license
    void Remove(TMapObject &Mo);
    void Show(cv::Mat& frame);

    void FreeAll(void);                             ///< free all plates at once (static mode)
};
//----------------------------------------------------------------------------------------
#endif // TALBUM_H
