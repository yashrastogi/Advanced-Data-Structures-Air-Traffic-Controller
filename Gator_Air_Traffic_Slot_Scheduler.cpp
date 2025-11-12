#include <climits>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "binary_heap.hpp"
#include "pairing_heap.hpp"

using namespace std;

// Global output stream for all program output
stringstream ss;

// Flight lifecycle states
enum FlightState { PENDING, SCHEDULED, IN_PROGRESS, COMPLETED };

// Initial flight submission data
struct FlightRequest {
  int flightId;
  int airlineId;
  int submitTime;
  int priority;
  int duration;
  FlightRequest(int flightId, int airlineId, int submitTime, int priority,
                int duration)
      : flightId(flightId), airlineId(airlineId), submitTime(submitTime),
        priority(priority), duration(duration) {}
};

// Flight data while in pending queue
struct PendingFlight {
  int priority;
  int submitTime;
  int flightId;
  FlightRequest flightRequest;

  PendingFlight(int priority, int submitTime, int flightId,
                FlightRequest flightRequest)
      : priority(priority), submitTime(submitTime), flightId(flightId),
        flightRequest(flightRequest) {}
};

// Flight data after scheduling
struct ActiveFlightData {
  int runwayId;
  int startTime;
  int ETA;
  FlightRequest flightRequest;

  ActiveFlightData()
      : runwayId(0), startTime(0), ETA(0), flightRequest(0, 0, 0, 0, 0) {}

  ActiveFlightData(int runwayId, int startTime, int ETA,
                   FlightRequest flightRequest)
      : runwayId(runwayId), startTime(startTime), ETA(ETA),
        flightRequest(flightRequest) {}
};

// Entry in the time table for tracking completion times
struct TimeTableEntry {
  int ETA;
  int flightId;
  int runwayId;
  TimeTableEntry(int ETA, int flightId, int runwayId)
      : ETA(ETA), flightId(flightId), runwayId(runwayId) {}
  bool operator==(const TimeTableEntry &other) const {
    return flightId == other.flightId && ETA == other.ETA &&
           runwayId == other.runwayId;
  }
};

// Comparator for pending flight priority queue
struct CompPendingFlight {
  bool operator()(const PendingFlight &a, const PendingFlight &b) const {
    if (a.priority != b.priority) {
      return a.priority > b.priority;
    } else if (a.submitTime != b.submitTime) {
      return a.submitTime < b.submitTime;
    }
    return a.flightId < b.flightId;
  }
};

// Handle entry for tracking flight state and references
struct HandlesEntry {
  FlightState state;
  PairingHeapNode<PendingFlight> *pendingNode;
  int submitTime;
  TimeTableEntry timeTableEntry;

  HandlesEntry()
      : state(PENDING), pendingNode(nullptr), submitTime(0),
        timeTableEntry(TimeTableEntry(0, 0, 0)) {}

  HandlesEntry(FlightState state, PairingHeapNode<PendingFlight> *node,
               int submitTime, TimeTableEntry timeTableEntry)
      : state(state), pendingNode(node), submitTime(submitTime),
        timeTableEntry(timeTableEntry) {}
};

// Comparator for time table entries
struct CompTimeTableEntry {
  bool operator()(const TimeTableEntry &a, const TimeTableEntry &b) const {
    if (a.ETA != b.ETA) {
      return a.ETA < b.ETA;
    }
    return a.flightId < b.flightId;
  }
};

class GatorAirTrafficSlotScheduler {
public:
  // Tracks runways by next available time
  // Ensures flights are assigned to earliest free runway
  BinaryHeap<pair<int, int>, less<pair<int, int>>> runwayPool;

  // Priority queue for new flight requests
  // Orders by priority, submit time, and flight ID
  PairingHeap<PendingFlight, CompPendingFlight> pendingFlights;

  // Maps flight ID to active flight data
  // Enables quick lookup for cancellation and reprioritization
  unordered_map<int, ActiveFlightData> activeFlights;

  // Sorted list of scheduled flights by completion time
  // Enables efficient completion processing on time advancement
  BinaryHeap<TimeTableEntry, CompTimeTableEntry> timeTable;

  // Groups flights by airline for unsatisfied flights
  // Supports efficient airline-wide operations
  unordered_map<int, unordered_set<int>> airlineIndex;

  // Central map storing references to flight locations
  // Ensures quick and consistent updates across data structures
  unordered_map<int, HandlesEntry> handles;

  int currentTime;

  // Initialize the system with specified number of runways
  void initialize(int runwayCount) {
    if (runwayCount <= 0) {
      ss << "Invalid input" << "\n";
    }
    for (int i = 0; i < runwayCount; i++) {
      runwayPool.push({0, i + 1});
    }
    currentTime = 0;
    ss << runwayCount << " Runways are now available" << "\n";
  }

  // Submit a new flight request to the system
  void submitFlight(int flightId, int airlineId, int submitTime, int priority,
                    int duration) {
    tick(submitTime);
    if (handles.count(flightId)) {
      ss << "Duplicate FlightID" << "\n";
      return;
    }

    auto pendingFlightHeapNode = pendingFlights.push(PendingFlight(
        priority, submitTime, flightId,
        FlightRequest(flightId, airlineId, submitTime, priority, duration)));
    airlineIndex[airlineId].insert(flightId);
    handles[flightId] = HandlesEntry(PENDING, pendingFlightHeapNode, submitTime,
                                     TimeTableEntry(0, 0, 0));

    tick(submitTime);
  }

  // Advance system time and process completions and rescheduling
  void tick(int currentTime) {
    this->currentTime = currentTime;

    // Phase 1: Settle completions
    // Find all flights with ETA at or before current time
    // Mark them completed and remove from data structures
    // Print in ascending ETA order with flight ID as tiebreaker

    // Pair of ETA and Flight ID
    auto comp = [](const pair<int, int> &a, const pair<int, int> &b) {
      return a.first == b.first ? a.second < b.second : a.first < b.first;
    };

    BinaryHeap<pair<int, int>, decltype(comp)> completed(comp);

    // Process completed flights and clean up data structures
    while (!timeTable.empty() && timeTable.top().ETA <= currentTime) {
      int flightId = timeTable.top().flightId;
      completed.push({timeTable.top().ETA, flightId});
      int airlineId = activeFlights[flightId].flightRequest.airlineId;
      airlineIndex[airlineId].erase(flightId);
      activeFlights.erase(flightId);
      handles[flightId].state = COMPLETED;
      timeTable.pop();
    }

    // Print completed flights in ascending order
    while (!completed.empty()) {
      ss << "Flight " << completed.top().second << " has landed at time "
         << completed.top().first << "\n";
      completed.pop();
    }

    // Promotion step between phases
    // Mark flights with start time at or before current time as in progress
    // These flights are non-preemptive and excluded from rescheduling
    for (auto it = activeFlights.begin(); it != activeFlights.end(); it++) {
      if (it->second.startTime <= currentTime) {
        handles[it->first].state = IN_PROGRESS;
        airlineIndex[it->second.flightRequest.airlineId].erase(it->first);
      }
    }

    // Phase 2: Reschedule unsatisfied flights
    // Track runways currently in use
    unordered_map<int, int> inUseRunways;
    // Track flights whose ETA changed during rescheduling
    unordered_map<int, int> rescheduleETAChanged;
    // Convert all scheduled flights back to pending for rescheduling
    for (auto it = activeFlights.begin(); it != activeFlights.end(); it++) {
      auto &e = (*it);
      // Unschedule flights that are scheduled but not yet in progress
      if (handles[e.first].state == SCHEDULED) {
        handles[e.first].pendingNode = pendingFlights.push(PendingFlight(
            e.second.flightRequest.priority, handles[e.first].submitTime,
            e.first, e.second.flightRequest));
        handles[e.first].state = PENDING;
        rescheduleETAChanged[e.first] = e.second.ETA;
        timeTable.eraseOne(handles[e.first].timeTableEntry);
        // Unset scheduling fields but keep in active flights
        e.second.startTime = -1;
        e.second.ETA = -1;
        e.second.runwayId = -1;
      }
      // Track runways being used by in-progress flights
      else if (handles[e.first].state == IN_PROGRESS) {
        inUseRunways[e.second.runwayId] = e.second.ETA;
      }
    }

    // Rebuild runway pool with current availability
    int runwayCount = runwayPool.size();
    runwayPool.clear();
    for (int i = 0; i < runwayCount; i++) {
      int runwayId = i + 1;
      if (inUseRunways.count(runwayId) == 0) {
        runwayPool.push({currentTime, runwayId});
      } else {
        runwayPool.push({inUseRunways[runwayId], runwayId});
      }
    }

    // Schedule all pending flights
    while (!pendingFlights.empty()) {
      auto pendingFlight = pendingFlights.pop();
      auto runway = runwayPool.pop();
      int startTime = max(currentTime, runway.first);
      int ETA = startTime + pendingFlight.flightRequest.duration;
      // Return runway to pool with updated availability
      runwayPool.push({ETA, runway.second});

      // Add flight to time table and active flights
      auto timeTableEntry =
          TimeTableEntry(ETA, pendingFlight.flightId, runway.second);
      timeTable.push(timeTableEntry);

      activeFlights[pendingFlight.flightId] = ActiveFlightData(
          runway.second, startTime, ETA, pendingFlight.flightRequest);
      handles[pendingFlight.flightId] = HandlesEntry(
          SCHEDULED, nullptr, pendingFlight.submitTime, timeTableEntry);

      // Track ETA changes or mark as new scheduling
      if (rescheduleETAChanged.count(pendingFlight.flightId) &&
          rescheduleETAChanged[pendingFlight.flightId] != ETA) {
        rescheduleETAChanged[pendingFlight.flightId] = ETA;
      } else if (!rescheduleETAChanged.count(pendingFlight.flightId)) {
        ss << "Flight " << pendingFlight.flightId << " scheduled - ETA: " << ETA
           << "\n";
      } else {
        rescheduleETAChanged.erase(pendingFlight.flightId);
      }
    }

    // Print rescheduled flights with updated ETAs
    BinaryHeap<pair<int, int>, less<pair<int, int>>> rescheduled;
    for (const auto &entry : rescheduleETAChanged) {
      rescheduled.push({entry.first, entry.second});
    }
    if (!rescheduled.empty()) {
      ss << "Updated ETAs: [";
      bool first = true;
      while (!rescheduled.empty()) {
        auto entry = rescheduled.pop();
        if (!first) {
          ss << ", ";
        }
        ss << entry.first << ": " << entry.second;
        first = false;
      }
      ss << "]" << "\n";
    }
  }

  // Print all scheduled flights within a time range
  void printSchedule(int t1, int t2) {
    PairingHeap<pair<int, string>, less<pair<int, string>>> schedulePrintHeap;
    for (const auto &entry : activeFlights) {
      if (handles[entry.first].state != SCHEDULED ||
          entry.second.startTime <= currentTime) {
        continue;
      }
      const ActiveFlightData &data = entry.second;
      if (data.ETA >= t1 && data.ETA <= t2) {
        schedulePrintHeap.push({data.ETA, "[" + to_string(entry.first) + "]"});
      }
    }

    if (schedulePrintHeap.empty()) {
      ss << "There are no flights in that time period" << "\n";
    }

    while (!schedulePrintHeap.empty()) {
      ss << schedulePrintHeap.pop().second << "\n";
    }
  }

  // Print all active flights
  void printActive() {
    PairingHeap<pair<int, string>, less<pair<int, string>>> activePrintHeap;
    for (const auto &entry : activeFlights) {
      const ActiveFlightData &data = entry.second;
      activePrintHeap.push(
          {entry.first, "[flight" + to_string(entry.first) + ", airline" +
                            to_string(data.flightRequest.airlineId) +
                            ", runway" + to_string(data.runwayId) + ", start" +
                            to_string(data.startTime) + ", ETA" +
                            to_string(data.ETA) + "]"});
    }
    while (!activePrintHeap.empty()) {
      ss << activePrintHeap.top().second << "\n";
      activePrintHeap.pop();
    }
  }

  // Ground all flights for airlines in specified range
  void groundHold(int airlineLow, int airlineHigh, int currentTime) {
    tick(currentTime);
    if (airlineHigh < airlineLow) {
      ss << "Invalid input. Please provide a valid airline range." << "\n";
      return;
    }

    for (int airlineId = airlineLow; airlineId <= airlineHigh; airlineId++) {
      if (airlineIndex.count(airlineId)) {
        // Copy flight IDs to avoid iterator invalidation during removal
        vector<int> flightsToGround(airlineIndex[airlineId].begin(),
                                    airlineIndex[airlineId].end());
        for (int flightId : flightsToGround) {
          if (handles[flightId].state == PENDING) {
            pendingFlights.eraseOne(handles[flightId].pendingNode);
          } else if (handles[flightId].state == SCHEDULED) {
            timeTable.eraseOne(handles[flightId].timeTableEntry);
          }
          activeFlights.erase(flightId);
          handles.erase(flightId);
          airlineIndex[airlineId].erase(flightId);
        }
      }
    }
    ss << "Flights of the airlines in the range [" << airlineLow << ", "
       << airlineHigh << "] have been grounded" << "\n";
    tick(currentTime);
  }

  // Add additional runways to the system
  void addRunways(int count, int currentTime) {
    tick(currentTime);
    if (count <= 0) {
      ss << "Invalid input. Please provide a valid number of runways."
         << "\n";
      return;
    }
    int existingRunways = runwayPool.size();
    for (int i = 0; i < count; i++) {
      runwayPool.push({currentTime, existingRunways + i + 1});
    }
    ss << "Additional " << count << " Runways are now available" << "\n";
    tick(currentTime);
  }

  // Update priority of a pending or scheduled flight
  void reprioritize(int flightId, int currentTime, int newPriority) {
    tick(currentTime);
    if (!handles.count(flightId)) {
      ss << "Flight " << flightId << " not found" << "\n";
      return;
    } else if (handles[flightId].state == IN_PROGRESS ||
               handles[flightId].state == COMPLETED) {
      ss << "Cannot reprioritize. Flight " << flightId
         << " has already departed" << "\n";
      return;
    }

    if (handles[flightId].state == PENDING) {
      // Update priority in pending flights queue
      auto flightRequest = handles[flightId].pendingNode->value.flightRequest;
      handles[flightId].pendingNode = pendingFlights.changeKey(
          handles[flightId].pendingNode,
          PendingFlight(newPriority, handles[flightId].submitTime, flightId,
                        flightRequest));
    } else {
      // Update priority in active flights
      activeFlights[flightId].flightRequest.priority = newPriority;
    }
    ss << "Priority of Flight " << flightId << " has been updated to "
       << newPriority << "\n";
    tick(currentTime);
  }

  // Cancel a pending or scheduled flight
  void cancelFlight(int flightId, int currentTime) {
    tick(currentTime);
    if (!handles.count(flightId)) {
      ss << "Flight " << flightId << " does not exist" << "\n";
      return;
    }
    if (handles[flightId].state == IN_PROGRESS ||
        handles[flightId].state == COMPLETED) {
      ss << "Cannot cancel: Flight " << flightId << " has already departed"
         << "\n";
      return;
    }
    // Clean up from all data structures
    if (handles[flightId].state == SCHEDULED) {
      timeTable.eraseOne(handles[flightId].timeTableEntry);
      airlineIndex[activeFlights[flightId].flightRequest.airlineId].erase(
          flightId);
    }
    if (handles[flightId].state == PENDING) {
      airlineIndex[handles[flightId].pendingNode->value.flightRequest.airlineId]
          .erase(flightId);
      pendingFlights.eraseOne(handles[flightId].pendingNode);
    }
    activeFlights.erase(flightId);
    handles.erase(flightId);
    ss << "Flight " << flightId << " has been canceled" << "\n";
    tick(currentTime);
  }
};

// Quit program and write output to file
void quit(ifstream &inputFile, char *argv[]) {
  ss << "Program Terminated!!" << "\n";
  inputFile.close();
  string inputFileName =
      string(argv[1]).substr(0, string(argv[1]).find_last_of('.'));
  ofstream outfile(inputFileName + "_output_file.txt");
  if (!outfile.is_open()) {
    cerr << "Failed to open output file for writing" << "\n";
    exit(1);
  }
  outfile << ss.str();
  ss.flush();
  outfile.flush();
  outfile.close();
  exit(0);
}

// Main program entry point
int main(int argc, char *argv[]) {
  if (argc != 2)
    throw std::invalid_argument("Invalid number of arguments");
  ifstream inputFile(argv[1]);
  if (!inputFile.is_open()) {
    cerr << "Failed to open input file" << "\n";
    return 1;
  }

  GatorAirTrafficSlotScheduler scheduler;

  string line;
  // Process each command from input file
  while (getline(inputFile, line)) {
    if (line == "Quit()") {
      quit(inputFile, argv);
    } else if (line.find("Tick") != string::npos) {
      int start = line.find("(");
      int end = line.find(")");
      string arg1 = line.substr(start + 1, end - start - 1);
      scheduler.tick(stoi(arg1));
    } else if (line.find("PrintSchedule") != string::npos) {
      int start = line.find("(");
      int end1 = line.find(",");
      int end = line.find(")");
      string arg1 = line.substr(start + 1, end1 - start - 1);
      string arg2 = line.substr(end1 + 1, end - end1 - 1);
      scheduler.printSchedule(stoi(arg1), stoi(arg2));
    } else if (line.find("PrintActive") != string::npos) {
      scheduler.printActive();
    } else if (line.find("GroundHold") != string::npos) {
      int start = line.find("(");
      int end1 = line.find(",");
      int end2 = line.find(",", end1 + 1);
      int end = line.find(")");
      string arg1 = line.substr(start + 1, end1 - start - 1);
      string arg2 = line.substr(end1 + 1, end2 - end1 - 1);
      string arg3 = line.substr(end2 + 1, end - end2 - 1);
      scheduler.groundHold(stoi(arg1), stoi(arg2), stoi(arg3));
    } else if (line.find("AddRunways") != string::npos) {
      int start = line.find("(");
      int end1 = line.find(",");
      int end = line.find(")");
      string arg1 = line.substr(start + 1, end1 - start - 1);
      string arg2 = line.substr(end1 + 1, end - end1 - 1);
      scheduler.addRunways(stoi(arg1), stoi(arg2));
    } else if (line.find("Reprioritize") != string::npos) {
      int start = line.find("(");
      int end1 = line.find(",");
      int end2 = line.find(",", end1 + 1);
      int end = line.find(")");
      string arg1 = line.substr(start + 1, end1 - start - 1);
      string arg2 = line.substr(end1 + 1, end2 - end1 - 1);
      string arg3 = line.substr(end2 + 1, end - end2 - 1);
      scheduler.reprioritize(stoi(arg1), stoi(arg2), stoi(arg3));
    } else if (line.find("CancelFlight") != string::npos) {
      int start = line.find("(");
      int end1 = line.find(",");
      int end = line.find(")");
      string arg1 = line.substr(start + 1, end1 - start - 1);
      string arg2 = line.substr(end1 + 1, end - end1 - 1);
      scheduler.cancelFlight(stoi(arg1), stoi(arg2));
    } else if (line.find("Initialize") != string::npos) {
      int start = line.find("(");
      int end = line.find(")");
      string arg1 = line.substr(start + 1, end - start - 1);
      scheduler.initialize(stoi(arg1));
    } else if (line.find("SubmitFlight") != string::npos) {
      int start = line.find("(");
      int end1 = line.find(",");
      int end2 = line.find(",", end1 + 1);
      int end3 = line.find(",", end2 + 1);
      int end4 = line.find(",", end3 + 1);
      int end = line.find(")");
      string arg1 = line.substr(start + 1, end1 - start - 1);
      string arg2 = line.substr(end1 + 1, end2 - end1 - 1);
      string arg3 = line.substr(end2 + 1, end3 - end2 - 1);
      string arg4 = line.substr(end3 + 1, end4 - end3 - 1);
      string arg5 = line.substr(end4 + 1, end - end4 - 1);
      scheduler.submitFlight(stoi(arg1), stoi(arg2), stoi(arg3), stoi(arg4),
                             stoi(arg5));
    } else {
      throw runtime_error("Invalid command: " + line);
    }
  }
  return 0;
}
