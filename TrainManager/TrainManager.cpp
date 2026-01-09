#include "TrainManager.h"
#include <sstream>
#include <fstream> 
#include <iostream>
#include <chrono>
#include <iomanip>
#include <cmath>

using namespace std;
using namespace tinyxml2;

// --- Time Helper Functions ---
static int toMinutes(const string& time) {
    if(time == "-" || time.empty()) return -1;
    int h, m;
    if (sscanf(time.c_str(), "%d:%d", &h, &m) != 2) return -1; 
    return (h * 60 + m) % 1440;
}

static string toTime(int minutes) {
    if(minutes < 0) return "--:--";
    minutes = (minutes % 1440 + 1440) % 1440; 
    int h = minutes / 60;
    int m = minutes % 60;
    stringstream ss;
    ss << setw(2) << setfill('0') << h << ":" << setw(2) << setfill('0') << m;
    return ss.str();
}

static string getCurrentTime() {
    using namespace chrono;
    auto now = system_clock::now();
    time_t t = system_clock::to_time_t(now);
    tm lt; localtime_r(&t, &lt); 
    stringstream ss; ss << put_time(&lt, "%H:%M");
    return ss.str();
}

static bool isTimeInNextHour(int targetTime, int nowTime) {
    if(targetTime == -1) return false;
    int oneHourLater = nowTime + 60;
    if (oneHourLater < 1440) return targetTime >= nowTime && targetTime <= oneHourLater;
    else {
        int overflowTime = oneHourLater - 1440;
        return (targetTime >= nowTime) || (targetTime <= overflowTime);
    }
}

// --- PERSISTENCE IMPLEMENTATION (XML + FILE COPY) ---

void TrainManager::copyFile(const string& src, const string& dst) {
    ifstream source(src, ios::binary);
    ofstream dest(dst, ios::binary);
    
    if (!source.is_open() || !dest.is_open()) {
        cerr << "[System Error] Could not copy Template (" << src << ") over Live (" << dst << ")!\n";
        return;
    }
    
    dest << source.rdbuf(); // Effective copy
    
    source.close();
    dest.close();
    cout << "[System] Schedule reset: " << dst << " was overwritten with data from " << src << ".\n";
}

void TrainManager::loadDataFromXML(const string& liveFile, const string& baseFile) {
    lock_guard<mutex> lock(mtx);
    trains.clear();
    
    dbFileName = liveFile;
    masterFileName = baseFile;

    // 1. Reset at start: Copy Base (org) over Live (mod)
    // Thus, we delete old delays from the previous run
    copyFile(masterFileName, dbFileName);

    // 2. Load data from the Live file (now clean)
    XMLDocument doc;
    XMLError eResult = doc.LoadFile(dbFileName.c_str());
    
    if (eResult != XML_SUCCESS) {
        cout << "[XML Error] XML read failure: " << dbFileName << endl;
        return;
    }

    XMLElement* root = doc.FirstChildElement("Trains");
    if (!root) return;

    XMLElement* trainNode = root->FirstChildElement("Train");
    while (trainNode) {
        Train t;
        trainNode->QueryIntAttribute("ID", &t.trainID);
        trainNode->QueryIntAttribute("Delay", &t.delayMinutes);
        const char* est = trainNode->Attribute("Estimate");
        t.estimate = est ? est : "N/A";

        XMLElement* routeNode = trainNode->FirstChildElement("Route");
        if (routeNode) {
            XMLElement* stNode = routeNode->FirstChildElement("Station");
            while (stNode) {
                Station s;
                s.name = stNode->Attribute("Name");
                const char* arr = stNode->Attribute("Arr");
                const char* dep = stNode->Attribute("Dep");
                s.arrivalTime = arr ? arr : "-";
                s.departureTime = dep ? dep : "-";
                t.route.push_back(s);
                stNode = stNode->NextSiblingElement("Station");
            }
        }
        trains[t.trainID] = t;
        trainNode = trainNode->NextSiblingElement("Train");
    }
    cout << "[XML] Data loaded successfully into memory.\n";
}

void TrainManager::saveDataToXML() {
    XMLDocument doc;
    XMLElement* root = doc.NewElement("Trains");
    doc.InsertEndChild(root);

    for (const auto& pair : trains) {
        const Train& t = pair.second;
        XMLElement* trainNode = doc.NewElement("Train");
        trainNode->SetAttribute("ID", t.trainID);
        trainNode->SetAttribute("Delay", t.delayMinutes);
        trainNode->SetAttribute("Estimate", t.estimate.c_str());

        XMLElement* routeNode = doc.NewElement("Route");
        for (const auto& s : t.route) {
            XMLElement* stNode = doc.NewElement("Station");
            stNode->SetAttribute("Name", s.name.c_str());
            stNode->SetAttribute("Arr", s.arrivalTime.c_str());
            stNode->SetAttribute("Dep", s.departureTime.c_str());
            routeNode->InsertEndChild(stNode);
        }
        trainNode->InsertEndChild(routeNode);
        root->InsertEndChild(trainNode);
    }
    // Save to the MODIFIABLE (live) file
    doc.SaveFile(dbFileName.c_str());
    cout << "[Persistence] Changes saved to " << dbFileName << endl;
}

void TrainManager::updateDelay(int trainID, int delayMinutes, const string& estimate) {
    lock_guard<mutex> lock(mtx);
    if(trains.count(trainID)) {
        trains[trainID].delayMinutes = delayMinutes;
        trains[trainID].estimate = estimate;
        // Write to disk immediately
        saveDataToXML();
    }
}

string TrainManager::getSchedule(const string& from, const string& to) {
    lock_guard<mutex> lock(mtx);
    stringstream ss;
    bool found = false;

    if(from.empty() && to.empty()) ss << "--- Train Schedule (Complete) ---\n";
    else ss << "--- Filtered Route: " << (from.empty()?"Any":from) << " -> " << (to.empty()?"Any":to) << " ---\n";

    for(const auto &pair : trains) {
        const Train& t = pair.second;
        int idxFrom = -1, idxTo = -1;

        if(from.empty() && to.empty()) { idxFrom = 0; idxTo = t.route.size()-1; }
        else {
            for(size_t i=0; i<t.route.size(); ++i) {
                if(!from.empty() && t.route[i].name == from) idxFrom = i;
                if(!to.empty() && t.route[i].name == to) idxTo = i;
            }
        }

        bool show = (from.empty() && to.empty()) || 
                    (!from.empty() && to.empty() && idxFrom != -1) ||
                    (!from.empty() && !to.empty() && idxFrom != -1 && idxTo != -1 && idxFrom < idxTo);

        if(show) {
            int end = to.empty() ? t.route.size()-1 : idxTo;
            ss << "Train " << t.trainID << ": " << t.route[idxFrom].name << "(" << t.route[idxFrom].departureTime << ") -> "
               << t.route[end].name << "(" << t.route[end].arrivalTime << ")";
            
            if(t.delayMinutes > 0) ss << " [Delay " << t.delayMinutes << " min]";
            else if(t.delayMinutes < 0) ss << " [Early by " << abs(t.delayMinutes) << " min]";
            else ss << " [On Time]";
            
            ss << "\n";
            found = true;
        }
    }
    if(!found) return "No trains found on this route.\n";
    return ss.str();
}

string TrainManager::getDeparturesNextHour(const string& stationFilter) {
    lock_guard<mutex> lock(mtx);
    int nowMin = toMinutes(getCurrentTime());
    stringstream ss;
    ss << "Departures next hour (" << getCurrentTime() << "):\n";
    
    bool found = false;
    for(const auto &pair : trains) {
        const Train& t = pair.second;
        for(size_t i=0; i<t.route.size()-1; ++i) {
            if(!stationFilter.empty() && t.route[i].name != stationFilter) continue;
            
            int depPlan = toMinutes(t.route[i].departureTime);
            int depReal = (depPlan + t.delayMinutes) % 1440;
            
            if(isTimeInNextHour(depReal, nowMin)) {
                ss << "Train " << t.trainID << " from " << t.route[i].name << " at " << toTime(depReal);
                if(t.delayMinutes != 0) ss << " (Delay: " << t.delayMinutes << ")";
                ss << "\n";
                found = true;
            }
        }
    }
    if(!found) return "No departures soon.\n";
    return ss.str();
}

string TrainManager::getArrivalsNextHour(const string& stationFilter) {
    lock_guard<mutex> lock(mtx);
    int nowMin = toMinutes(getCurrentTime());
    stringstream ss;
    ss << "Arrivals next hour (" << getCurrentTime() << "):\n";

    bool found = false;
    for(const auto &pair : trains) {
        const Train& t = pair.second;
        for(size_t i=1; i<t.route.size(); ++i) {
            if(!stationFilter.empty() && t.route[i].name != stationFilter) continue;

            int arrPlan = toMinutes(t.route[i].arrivalTime);
            if(arrPlan == -1) continue;
            
            int arrReal = (arrPlan + t.delayMinutes) % 1440;
            if(arrReal < 0) arrReal += 1440;

            if(isTimeInNextHour(arrReal, nowMin)) {
                ss << "Train " << t.trainID << " in " << t.route[i].name << " at " << toTime(arrReal);
                if(t.delayMinutes < 0) ss << " (EARLY " << abs(t.delayMinutes) << " min)";
                else if(t.delayMinutes > 0) ss << " (DELAY " << t.delayMinutes << " min)";
                else ss << " (On Time)";
                ss << "\n";
                found = true;
            }
        }
    }
    if(!found) return "No arrivals soon.\n";
    return ss.str();
}

string TrainManager::getTrainDetails(int id) {
    lock_guard<mutex> lock(mtx);
    if (trains.count(id)) {
        Train& t = trains[id];
        string res = "ID: " + to_string(t.trainID) + " | Status: " + t.estimate + " | Delay: " + to_string(t.delayMinutes) + "\nRoute:\n";
        for (auto& s : t.route) res += " - " + s.name + " (Arr:" + s.arrivalTime + ", Dep:" + s.departureTime + ")\n";
        return res;
    }
    return "Train does not exist.\n";
}