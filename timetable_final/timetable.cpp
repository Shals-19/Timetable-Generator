#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <algorithm>
#include <set>
#include <random>
#include <ctime>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

const int DAYS = 5;
const int SLOTS_PER_DAY = 6;

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

vector<Section> sections;
vector<Class> classes;
vector<Class> combined_classes;

map<string, vector<vector<string>>> timetable;
map<string, map<string, string>> section_professors;

mt19937 rng;

bool canFit(const vector<string>& day, int start, int duration) {
    for (int i = start; i < start + duration && i < SLOTS_PER_DAY; ++i)
        if (!day[i].empty()) return false;
    return true;
}

bool isClassScheduledOnDay(const vector<string>& day, const string& className) {
    for (const string& slot : day) {
        if (slot.find(className) != string::npos)
            return true;
    }
    return false;
}

string selectRandomProfessor(const vector<string>& professors) {
    uniform_int_distribution<int> dist(0, professors.size() - 1);
    return professors[dist(rng)];
}

void preAssignProfessors() {
    for (auto& section : sections) {
        for (auto& c : classes)
            section_professors[section.name][c.name] = selectRandomProfessor(c.professors);
        for (auto& c : combined_classes)
            section_professors[section.name][c.name] = selectRandomProfessor(c.professors);
    }
}

void initTimetables() {
    for (auto& section : sections) {
        timetable[section.name] = vector<vector<string>>(DAYS, vector<string>(SLOTS_PER_DAY, ""));
    }
}

vector<int> getLabSlots() {
    return {0, 2, 4};
}

void assignCombinedClasses() {
    for (auto& c : combined_classes) {
        for (int freq = 0; freq < c.frequency_per_week; ++freq) {
            bool placed = false;
            for (int d = 0; d < DAYS && !placed; ++d) {
                if (isClassScheduledOnDay(timetable[sections[0].name][d], c.name)) continue;

                vector<int> slots_to_try = (c.duration == 2) ? getLabSlots() : vector<int>{0, 1, 2, 3, 4, 5};

                for (int t : slots_to_try) {
                    if (t + c.duration > SLOTS_PER_DAY) continue;

                    bool all_free = true;
                    for (auto& section : sections) {
                        if (!canFit(timetable[section.name][d], t, c.duration)) {
                            all_free = false;
                            break;
                        }
                    }

                    if (all_free) {
                        for (auto& section : sections) {
                            string prof = section_professors[section.name][c.name];
                            for (int k = 0; k < c.duration; ++k)
                                timetable[section.name][d][t + k] = c.name + " (" + prof + ")";
                        }
                        placed = true;
                        break;
                    }
                }
            }
        }
    }
}

vector<int> getRandomizedOrder(int max_val, int seed_offset = 0) {
    vector<int> order(max_val);
    iota(order.begin(), order.end(), 0);
    mt19937 local_rng(time(0) + seed_offset);
    shuffle(order.begin(), order.end(), local_rng);
    return order;
}

void assignIndividualClasses() {
    int section_seed = 0;
    for (auto& section : sections) {
        auto& grid = timetable[section.name];
        vector<Class> section_classes = classes;
        mt19937 local_rng(time(0) + section_seed);

        sort(section_classes.begin(), section_classes.end(), [&](Class& a, Class& b) {
            if (a.is_lab != b.is_lab) return a.is_lab > b.is_lab;
            return a.frequency_per_week > b.frequency_per_week;
        });

        for (auto& c : section_classes) {
            for (int freq = 0; freq < c.frequency_per_week; ++freq) {
                bool placed = false;
                vector<int> day_order = getRandomizedOrder(DAYS, section_seed + freq);

                for (int day_idx : day_order) {
                    if (placed) break;
                    if (isClassScheduledOnDay(grid[day_idx], c.name)) continue;

                    if (c.duration == 2) {
                        vector<int> lab_slots = getLabSlots();
                        shuffle(lab_slots.begin(), lab_slots.end(), local_rng);
                        for (int t : lab_slots) {
                            if (canFit(grid[day_idx], t, c.duration)) {
                                string prof = section_professors[section.name][c.name];
                                for (int k = 0; k < c.duration; ++k)
                                    grid[day_idx][t + k] = c.name + " (" + prof + ")";
                                placed = true;
                                break;
                            }
                        }
                    } else {
                        vector<int> slot_order = getRandomizedOrder(SLOTS_PER_DAY, section_seed + day_idx);
                        for (int t : slot_order) {
                            if (canFit(grid[day_idx], t, c.duration)) {
                                string prof = section_professors[section.name][c.name];
                                grid[day_idx][t] = c.name + " (" + prof + ")";
                                placed = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
        section_seed += 100;
    }
}

void loadInputJSON(const string& filename) {
    ifstream in(filename);
    json j;
    in >> j;

    int numSections = j["numSections"];
    for (int i = 0; i < numSections; ++i) {
        string section_name = "Section_" + to_string(i + 1); // You can change names to A, B, etc. if preferred
        sections.push_back({section_name});
    }

    for (const auto& c : j["courses"]) {
        Class cl;
        cl.name = c["name"];
        cl.frequency_per_week = c["frequency"];
        cl.duration = c["duration"];
        cl.is_lab = (cl.duration == 2);
        cl.is_combined = false;
        for (const auto& prof : c["professors"])
            cl.professors.push_back(prof.get<string>());
        classes.push_back(cl);
    }

    for (const auto& c : j["combinedClasses"]) {
        Class cl;
        cl.name = c["name"];
        cl.frequency_per_week = c["frequency"];
        cl.duration = c["duration"];
        cl.is_lab = (cl.duration == 2);
        cl.is_combined = true;
        for (const auto& prof : c["professors"])
            cl.professors.push_back(prof.get<string>());
        combined_classes.push_back(cl);
    }
}


void writeOutputJSON(const string& filename) {
    json out;
    for (auto& pair : timetable) {
        const string& section = pair.first;
        const auto& grid = pair.second;

        json week;
        for (int d = 0; d < DAYS; ++d) {
            json day;
            for (int t = 0; t < SLOTS_PER_DAY; ++t) {
                day.push_back(grid[d][t]);
            }
            week.push_back(day);
        }

        out[section] = week;
    }

    ofstream o(filename);
    o << setw(4) << out << endl;
}

int main() {
    rng.seed(time(0));

    loadInputJSON("input.json");
    initTimetables();
    preAssignProfessors();
    assignCombinedClasses();
    assignIndividualClasses();
    writeOutputJSON("output.json");

    cout << "Timetable generation completed. Output saved to output.json" << endl;
    return 0;
}
