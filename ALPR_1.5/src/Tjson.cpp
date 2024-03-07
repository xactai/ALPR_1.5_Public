//----------------------------------------------------------------------------------------
//
// Created by Q-engineering 2023/1/2
//
//----------------------------------------------------------------------------------------
#include "Tjson.h"
#include "General.h"
#include <iostream>
#include <fstream>
//----------------------------------------------------------------------------------------
Tjson::Tjson(): UsedStreams(0),
                WorkWidth(WORK_WIDTH),
                WorkHeight(WORK_HEIGHT),
                ThumbWidth(THUMB_WIDTH),
                ThumbHeight(THUMB_HEIGHT),
                MJPEG_Port(0)
{
    Jvalid=false;
}
//----------------------------------------------------------------------------------------
Tjson::~Tjson()
{
    //dtor
}
//----------------------------------------------------------------------------------------
bool Tjson::LoadFromFile(const std::string FileName)
{
    if(std::filesystem::exists(FileName)){
        try{
            std::ifstream f(FileName);
            j = json::parse(f);
            Jvalid=true;
        }
        catch ( ... ){
            std::cout << "parse error in file " << FileName << std::endl;
        }
    }
    else{
        Jvalid=false;
        std::cout << FileName << " file not found!" << std::endl;
    }

    return Jvalid;
}
//----------------------------------------------------------------------------------------
bool Tjson::GetSettings(void)
{
    std::string Jstr, Sstr;
    bool Success=false;

    if(Jvalid){
        if(!GetSetting(j,"STREAMS_NR",UsedStreams))        return Success;

        //video source
        Sstr = "STREAM_1";
        Jstr = j[Sstr].at("VIDEO_INPUT");
        if(Jstr.empty()){
            std::cout << "Error reading VIDEO_INPUT value!" << std::endl;
            return Success;
        }

        //   filter objects
        if(!GetSetting(j,"MIN_PERSON_SIZE",MinPersonSize))          return Success;
        if(!GetSetting(j,"MIN_2_SIZE",MinTwoSize))                  return Success;
        if(!GetSetting(j,"MIN_4_SIZE",MinFourSize))                 return Success;
        //   additional plate parameters.
        if(!GetSetting(j,"PLATE_RATIO",PlateRatio))                 return Success;
        if(!GetSetting(j,"NR_CHARS_PLATE",PlateNrChars))            return Success;
        //   inspection
        if(!GetSetting(j,"INSPECT_TIME",InspectTime))               return Success;
        if(!GetSetting(j,"INSPECT_DISTANCE",InspectDistance))       return Success;
        if(!GetSetting(j,"INSPECT_VELOCITY",InspectSpeed))          return Success;
        //   output
        if(!GetSetting(j,"MQTT_ON",MQTT_On))                        return Success;
        if(!GetSetting(j,"JSON_PORT",JSON_Port))                    return Success;
        if(!GetSetting(j,"MJPEG_PORT",MJPEG_Port))                  return Success;
        if(!GetSetting(j,"MJPEG_WIDTH",MJPEG_Width))                return Success;
        if(!GetSetting(j,"MJPEG_HEIGHT",MJPEG_Height))              return Success;
        if(!GetSetting(j,"PRINT_ON_CLI",PrintOnCli))                return Success;
        if(!GetSetting(j,"PRINT_ON_RENDER",PrintOnRender))          return Success;
        if(!GetSetting(j,"PRINT_PLATE",PrintPlate))                 return Success;

        if(!GetSetting(j,"PORTAL_FREEZE",PortalFreeze))             return Success;
        //   administration
        if(!GetSetting(j,"VERSION",Version))                        return Success;
        //   DNN models
        if(!GetSetting(j,"VEHICLE_MODEL",Cstr))                     return Success;
        if(!GetSetting(j,"LICENSE_MODEL",Lstr))                     return Success;
        if(!GetSetting(j,"OCR_MODEL",Ostr))                         return Success;
        if(!GetSetting(j,"PORTAL_FILE",Pstr))                       return Success;
        if(!GetSetting(j,"THRESHOLD_VERHICLE",ThresCar))            return Success;
        if(!GetSetting(j,"THRESHOLD_PLATE",ThresPlate))             return Success;
        if(!GetSetting(j,"THRESHOLD_OCR",ThresOCR))                 return Success;
        //   folders
        if(!GetSetting(j,"FoI_FOLDER",FoI_Folder))                  return Success;
        if(!GetSetting(j,"VEHICLES_FOLDER",Car_Folder))             return Success;
        if(!GetSetting(j,"PLATES_FOLDER",Plate_Folder))             return Success;
        if(!GetSetting(j,"JSONS_FOLDER",Json_Folder))               return Success;
        if(!GetSetting(j,"RENDERS_FOLDER",Render_Folder))           return Success;
        //optional, not given than the defined values in General.h are used
        if(!GetSetting(j,"WORK_WIDTH",WorkWidth))                   std::cout << "Using default value" << std::endl;
        if(!GetSetting(j,"WORK_HEIGHT",WorkHeight))                 std::cout << "Using default value" << std::endl;
        if(!GetSetting(j,"THUMB_WIDTH",ThumbWidth))                 std::cout << "Using default value" << std::endl;
        if(!GetSetting(j,"THUMB_HEIGHT",ThumbHeight))               std::cout << "Using default value" << std::endl;

        //so far, so good
        Success=true;
    }

    return Success;
}
//----------------------------------------------------------------------------------------
void Tjson::MakeFolders(void)
{
    MakeDir(FoI_Folder);
    MakeDir(Car_Folder);
    MakeDir(Plate_Folder);
    MakeDir(Json_Folder);
    MakeDir(Render_Folder);
}
//----------------------------------------------------------------------------------------
bool Tjson::GetSetting(const json& s,const std::string Key,std::string& Value)
{
    bool Success=false;

    if(Jvalid){
        //read the key
        Value = s.at(Key);
        if(Value.empty()){
            std::cout << "Error reading value of "<< Key <<" in json file!" << std::endl;
        }
        else Success=true;
    }

    return Success;
}
//----------------------------------------------------------------------------------------
bool Tjson::GetSetting(const json& s, const std::string Key,bool& Value)
{
    bool Success=false;

    if(Jvalid){
        //read the key
        try{
            Value = s.at(Key);
            Success=true;
        }
        catch( ... ){
            std::cout << "Error reading value of "<< Key <<" in json file!" << std::endl;
        }
    }

    return Success;
}
//----------------------------------------------------------------------------------------
bool Tjson::GetSetting(const json& s, const std::string Key,int& Value)
{
    bool Success=false;

    if(Jvalid){
        try{
            Value = s.at(Key);
            Success=true;
        }
        catch( ... ){
            std::cout << "Error reading value of "<< Key <<" in json file!" << std::endl;
        }
    }

    return Success;
}
//----------------------------------------------------------------------------------------
bool Tjson::GetSetting(const json& s, const std::string Key,double& Value)
{
    bool Success=false;

    if(Jvalid){
        try{
            Value = s.at(Key);
            Success=true;
        }
        catch( ... ){
            std::cout << "Error reading value of "<< Key <<" in json file!" << std::endl;
        }
    }

    return Success;
}
//----------------------------------------------------------------------------------------

