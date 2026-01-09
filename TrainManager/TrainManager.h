#pragma once
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include "../xml_parser/tinyxml2.h"

// Structure for a stop (station)
struct Station {
    std::string name;
    std::string arrivalTime;   // ex: "10:00" or "-" for the first station
    std::string departureTime; // ex: "10:15" or "-" for the last station
};

struct Train {
    int trainID;
    std::vector<Station> route; 
    int delayMinutes;
    std::string estimate;
};

class TrainManager {
private:
    std::map<int, Train> trains;
    std::mutex mtx;
    std::string dbFileName;
    std::string masterFileName;

    void saveDataToXML();   
    void copyFile(const std::string& src, const std::string& dst);

public:
    void loadDataFromXML(const std::string& liveFile, const std::string& baseFile);
    
    std::string getSchedule(const std::string& from = "", const std::string& to = "");
    
    // These methods traverse the routes
    std::string getDeparturesNextHour(const std::string& stationFilter = "");
    std::string getArrivalsNextHour(const std::string& stationFilter = "");
    std::string getTrainDetails(int id);

    void updateDelay(int trainID, int delayMinutes, const std::string& estimate);
};