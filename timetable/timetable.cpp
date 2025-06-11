#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <random>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

const int DAYS = 5;
const int SLOTS_PER_DAY = 6;
const vector<string> DAY_NAMES = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};
const vector<string> TIME_SLOTS = {"9:00-10:00", "10:00-11:00", "11:00-12:00", "12:00-1:00", "1:00-2:00", "2:00-3:00"};

struct Class {
    string name;
    int frequency_per_week;
    bool is_lab;
    int duration;
    bool is_combined;
    vector<string> professors;
};

struct Section {
    string name;
};

int num_sections;
vector<Section> sections;
vector<Class> classes;
vector<Class> combined_classes;

map<string, vector<vector<string>>> timetable;
map<string, map<string, string>> section_professors;

mt19937 rng;

bool canFit(const vector<string>& day, int start, int duration) {
    if (start + duration > SLOTS_PER_DAY) return false;
    for (int i = start; i < start + duration; ++i) {
        if (!day[i].empty()) return false;
    }
    return true;
}

bool isClassScheduledOnDay(const vector<string>& day, const string& className) {
    for (const string& slot : day) {
        if (!slot.empty() && slot.find(className) != string::npos) {
            return true;
        }
    }
    return false;
}

string selectRandomProfessor(const vector<string>& professors) {
    if (professors.empty()) return "TBD";
    uniform_int_distribution<int> dist(0, professors.size() - 1);
    return professors[dist(rng)];
}

void preAssignProfessors() {
    cout << "Pre-assigning professors..." << endl;
    for (const auto& sec : sections) {
        string secName = sec.name;
        for (const auto& c : classes) {
            section_professors[secName][c.name] = selectRandomProfessor(c.professors);
            cout << "  Section " << secName << ": " << c.name << " -> " << section_professors[secName][c.name] << endl;
        }
        for (const auto& c : combined_classes) {
            section_professors[secName][c.name] = selectRandomProfessor(c.professors);
            cout << "  Section " << secName << ": " << c.name << " (combined) -> " << section_professors[secName][c.name] << endl;
        }
    }
}

void loadInputFromJson(const string& filename) {
    ifstream inFile(filename);
    if (!inFile.is_open()) {
        cerr << "Error: Failed to open " << filename << endl;
        exit(1);
    }

    json j;
    try {
        inFile >> j;
    } catch (const json::exception& e) {
        cerr << "Error parsing JSON: " << e.what() << endl;
        exit(1);
    }

    if (!j.contains("numSections") || !j.contains("courses")) {
        cerr << "Error: Missing required fields in input.json" << endl;
        exit(1);
    }

    num_sections = j["numSections"];
    
    // Create sections
    sections.clear();
    for (int i = 0; i < num_sections; ++i) {
        Section s;
        s.name = string(1, 'A' + i);
        sections.push_back(s);
    }

    // Load courses
    classes.clear();
    for (const auto& item : j["courses"]) {
        Class c;
        c.name = item["name"];
        c.frequency_per_week = item["frequency"];
        c.duration = item["duration"];
        c.is_lab = (c.duration == 2);
        c.is_combined = false;
        if (item.contains("professors") && item["professors"].is_array()) {
            c.professors = item["professors"].get<vector<string>>();
        }
        classes.push_back(c);
        cout << "Loaded class: " << c.name << " (freq: " << c.frequency_per_week << ", duration: " << c.duration << ")" << endl;
    }

    // Load combined classes
    combined_classes.clear();
    if (j.contains("combinedClasses") && j["combinedClasses"].is_array()) {
        for (const auto& item : j["combinedClasses"]) {
            Class c;
            c.name = item["name"];
            c.frequency_per_week = item["frequency"];
            c.duration = item["duration"];
            c.is_lab = (c.duration == 2);
            c.is_combined = true;
            if (item.contains("professors") && item["professors"].is_array()) {
                c.professors = item["professors"].get<vector<string>>();
            }
            combined_classes.push_back(c);
            cout << "Loaded combined class: " << c.name << " (freq: " << c.frequency_per_week << ", duration: " << c.duration << ")" << endl;
        }
    }

    cout << "Successfully loaded " << num_sections << " sections, " 
         << classes.size() << " classes, " << combined_classes.size() << " combined classes." << endl;
}

void initTimetables() {
    timetable.clear();
    for (const auto& sec : sections) {
        timetable[sec.name] = vector<vector<string>>(DAYS, vector<string>(SLOTS_PER_DAY, ""));
    }
    cout << "Initialized timetables for " << sections.size() << " sections" << endl;
}

vector<int> getLabSlots() {
    return {0, 2, 4}; // Start slots for 2-hour labs
}

void assignCombinedClasses() {
    cout << "Assigning " << combined_classes.size() << " combined classes..." << endl;
    for (const auto& c : combined_classes) {
        cout << "Processing combined class: " << c.name << endl;
        for (int freq = 0; freq < c.frequency_per_week; ++freq) {
            bool placed = false;
            cout << "  Attempting to place occurrence " << (freq + 1) << "/" << c.frequency_per_week << endl;
            
            for (int d = 0; d < DAYS && !placed; ++d) {
                // Check if already scheduled on this day
                if (isClassScheduledOnDay(timetable[sections[0].name][d], c.name)) {
                    cout << "    Day " << d << ": Already scheduled" << endl;
                    continue;
                }

                vector<int> slots_to_try = (c.duration == 2) ? getLabSlots() : vector<int>{0, 1, 2, 3, 4, 5};

                for (int t : slots_to_try) {
                    if (t + c.duration > SLOTS_PER_DAY) continue;

                    bool all_free = true;
                    for (const auto& sec : sections) {
                        if (!canFit(timetable[sec.name][d], t, c.duration)) {
                            all_free = false;
                            break;
                        }
                    }

                    if (all_free) {
                        for (const auto& sec : sections) {
                            string prof = section_professors[sec.name][c.name];
                            for (int k = 0; k < c.duration; ++k) {
                                timetable[sec.name][d][t + k] = c.name + " (" + prof + ")";
                            }
                        }
                        placed = true;
                        cout << "    Placed on " << DAY_NAMES[d] << " at slot " << t << endl;
                        break;
                    }
                }
            }
            if (!placed) {
                cout << "    WARNING: Could not place occurrence " << (freq + 1) << endl;
            }
        }
    }
}

void assignIndividualClasses() {
    cout << "Assigning individual classes..." << endl;
    
    for (const auto& sec : sections) {
        cout << "Processing section " << sec.name << endl;
        auto& grid = timetable[sec.name];
        
        // Sort classes: labs first, then by frequency (descending)
        vector<Class> section_classes = classes;
        sort(section_classes.begin(), section_classes.end(), [](const Class& a, const Class& b) {
            if (a.is_lab != b.is_lab) return a.is_lab > b.is_lab;
            return a.frequency_per_week > b.frequency_per_week;
        });

        for (const auto& c : section_classes) {
            cout << "  Assigning " << c.name << " (freq: " << c.frequency_per_week 
                 << ", duration: " << c.duration << ")" << endl;
            
            for (int freq = 0; freq < c.frequency_per_week; ++freq) {
                bool placed = false;
                cout << "    Placing occurrence " << (freq + 1) << "/" << c.frequency_per_week << endl;
                
                for (int d = 0; d < DAYS && !placed; ++d) {
                    // Skip if already scheduled on this day
                    if (isClassScheduledOnDay(grid[d], c.name)) {
                        continue;
                    }

                    if (c.duration == 2) {
                        // Lab scheduling
                        vector<int> lab_slots = getLabSlots();
                        for (int t : lab_slots) {
                            if (canFit(grid[d], t, c.duration)) {
                                string prof = section_professors[sec.name][c.name];
                                for (int k = 0; k < c.duration; ++k) {
                                    grid[d][t + k] = c.name + " (" + prof + ")";
                                }
                                placed = true;
                                cout << "      Placed lab on " << DAY_NAMES[d] << " at slots " << t << "-" << (t+1) << endl;
                                break;
                            }
                        }
                    } else {
                        // Regular class scheduling
                        for (int t = 0; t < SLOTS_PER_DAY; ++t) {
                            if (canFit(grid[d], t, c.duration)) {
                                string prof = section_professors[sec.name][c.name];
                                grid[d][t] = c.name + " (" + prof + ")";
                                placed = true;
                                cout << "      Placed on " << DAY_NAMES[d] << " at slot " << t << " (" << TIME_SLOTS[t] << ")" << endl;
                                break;
                            }
                        }
                    }
                    if (placed) break;
                }
                if (!placed) {
                    cout << "      WARNING: Could not place occurrence " << (freq + 1) << endl;
                }
            }
        }
    }
}

void printTimetable() {
    cout << "\n=== Generated Timetables ===" << endl;
    for (const auto& sec : sections) {
        cout << "\nSection " << sec.name << ":" << endl;
        cout << "Time Slot    ";
        for (int d = 0; d < DAYS; ++d) {
            cout << setw(20) << DAY_NAMES[d];
        }
        cout << endl;
        
        for (int t = 0; t < SLOTS_PER_DAY; ++t) {
            cout << setw(12) << TIME_SLOTS[t] << " ";
            for (int d = 0; d < DAYS; ++d) {
                string slot = timetable[sec.name][d][t];
                if (slot.empty()) slot = "FREE";
                cout << setw(20) << slot.substr(0, 19);
            }
            cout << endl;
        }
    }
}

void saveOutputToJson(const string& filename) {
    json output;

    // Add metadata
    output["metadata"]["generatedAt"] = time(0);
    output["metadata"]["numSections"] = num_sections;
    output["metadata"]["daysPerWeek"] = DAYS;
    output["metadata"]["slotsPerDay"] = SLOTS_PER_DAY;
    output["metadata"]["dayNames"] = DAY_NAMES;
    output["metadata"]["timeSlots"] = TIME_SLOTS;

    // Add timetables - ensure no nulls
    for (const auto& sec : sections) {
        json timetableArray;
        for (int d = 0; d < DAYS; ++d) {
            json day;
            for (int t = 0; t < SLOTS_PER_DAY; ++t) {
                string slot = timetable[sec.name][d][t];
                // Replace empty slots with "FREE" to avoid nulls
                day.push_back(slot.empty() ? "FREE" : slot);
            }
            timetableArray.push_back(day);
        }
        output["sections"][sec.name] = timetableArray;
    }

    try {
        ofstream outFile(filename);
        if (!outFile.is_open()) {
            cerr << "Error: Could not create " << filename << endl;
            return;
        }
        outFile << output.dump(4);
        cout << "\nTimetable successfully saved to " << filename << "!" << endl;
    } catch (const exception& e) {
        cerr << "Error saving output: " << e.what() << endl;
    }
}

int main() {
    rng.seed(time(0));

    cout << "=== Timetable Generator ===" << endl;
    cout << "Reading input from input.json..." << endl;
    
    loadInputFromJson("input.json");
    initTimetables();
    preAssignProfessors();
    
    if (!combined_classes.empty()) {
        assignCombinedClasses();
    }
    
    assignIndividualClasses();
    
    printTimetable();
    saveOutputToJson("output.json");
    
    cout << "\n=== Generation Complete ===" << endl;
    return 0;
}