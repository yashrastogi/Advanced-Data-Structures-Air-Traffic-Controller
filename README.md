# Gator Air Traffic Slot Scheduler

## Student Information
**Name:** Yash Rastogi  
**UFID:** 15231895  
**UF Email:** yash.rastogi@ufl.edu  

---

## Project Overview

This project implements an advanced air traffic control slot scheduling system for managing flights at an airport with multiple runways. The system maintains flight requests, assigns runways to flights based on priority and availability, handles cancellations and reprioritization, and manages time-based scheduling and rescheduling operations.

The implementation uses custom data structures including:
- **Max Pairing Heap** (two-pass scheme) for pending flights priority queue
- **Binary Min Heap** for runway pool and time-based scheduling
- **Hash Tables** for efficient flight lookup and airline indexing (C++ STL `unordered_map`)

---

## Data Structures Implemented

### 1. Pairing Heap (Max Heap)
A max pairing heap implementation using the two-pass scheme for efficient priority queue operations.

**File:** `pairing_heap.hpp`

**Key Components:**
```cpp
template <typename T> 
struct PairingHeapNode {
    T value;
    PairingHeapNode<T> *child;
    PairingHeapNode<T> *leftSibling;
    PairingHeapNode<T> *rightSibling;
};

template <typename T, typename Compare = std::greater<T>> 
class PairingHeap {
private:
    Compare comp_;
    std::size_t totalNodes;
    PairingHeapNode<T> *root_;
    
public:
    // Core operations
    PairingHeapNode<T>* push(const T &value);
    T pop();
    T top() const;
    bool empty() const;
    size_type size() const;
    
    // Advanced operations
    PairingHeapNode<T>* changeKey(PairingHeapNode<T> *theNode, T newValue);
    bool eraseOne(PairingHeapNode<T> *theNode);
    
    // Helper methods
    PairingHeapNode<T>* meld(PairingHeapNode<T> *a, PairingHeapNode<T> *b);
    void detachNode(PairingHeapNode<T> *theNode);
};
```

**Time Complexity:**
- `push`: O(1)
- `pop`: O(log n) amortized
- `changeKey`: O(log n) amortized
- `eraseOne`: O(log n) amortized

---

### 2. Binary Heap (Min/Max Heap)
A binary heap implementation supporting both min and max heap configurations through custom comparators.

**File:** `binary_heap.hpp`

**Key Components:**
```cpp
template <typename T, typename Compare = std::greater<T>> 
class BinaryHeap {
private:
    std::vector<T> data_;
    Compare comp_;
    
    int parent(int index) const;
    size_t leftChild(int index) const;
    size_t rightChild(int index) const;
    size_t bubbleUp(size_t i);
    size_t bubbleDown(size_t i);
    void swap(T *a, T *b);
    
public:
    // Core operations
    void push(const T &value);
    T pop();
    T top();
    bool empty() const;
    size_type size() const;
    
    // Advanced operations
    bool changeKey(T value, T newValue);
    bool eraseOne(T &value);
    void clear();
};
```

**Time Complexity:**
- `push`: O(log n)
- `pop`: O(log n)
- `top`: O(1)
- `changeKey`: O(n) [search] + O(log n) [restructure]
- `eraseOne`: O(n)

---

## Core System Components

### Main Scheduler Class

**File:** `Gator_Air_Traffic_Slot_Scheduler.cpp`

```cpp
class GatorAirTrafficSlotScheduler {
private:
    // Runway management - min heap by (nextFreeTime, runwayID)
    BinaryHeap<pair<int, int>, less<pair<int, int>>> runwayPool;
    
    // Pending flights - max pairing heap by (priority, -submitTime, -flightID)
    PairingHeap<PendingFlight, CompPendingFlight> pendingFlights;
    
    // Active flights lookup table
    unordered_map<int, ActiveFlightData> activeFlights;
    
    // Completion tracking - min heap by (ETA, flightID)
    BinaryHeap<TimeTableEntry, CompTimeTableEntry> timeTable;
    
    // Airline index for grouping flights
    unordered_map<int, unordered_set<int>> airlineIndex;
    
    // Central handles map for cross-references
    unordered_map<int, HandlesEntry> handles;
    
    int currentTime;

public:
    // System initialization
    void initialize(int runwayCount);
    
    // Flight operations
    void submitFlight(int flightId, int airlineId, int submitTime, 
                     int priority, int duration);
    void cancelFlight(int flightId, int currentTime);
    void reprioritize(int flightId, int currentTime, int newPriority);
    
    // Runway operations
    void addRunways(int count, int currentTime);
    
    // Airline operations
    void groundHold(int airlineLow, int airlineHigh, int currentTime);
    
    // Time management
    void tick(int currentTime);
    
    // Query operations
    void printActive();
    void printSchedule(int t1, int t2);
};
```

---

## Data Structures and Their Roles

### Supporting Structures

```cpp
// Flight lifecycle states
enum FlightState { 
    PENDING,      // In pending queue, not yet scheduled
    SCHEDULED,    // Assigned runway and time, not started
    IN_PROGRESS,  // Currently using runway
    COMPLETED     // Finished and removed
};

// Initial flight submission data
struct FlightRequest {
    int flightId;
    int airlineId;
    int submitTime;
    int priority;
    int duration;
};

// Flight data in pending queue
struct PendingFlight {
    int priority;
    int submitTime;
    int flightId;
    FlightRequest flightRequest;
};

// Flight data after scheduling
struct ActiveFlightData {
    int runwayId;
    int startTime;
    int ETA;
    FlightRequest flightRequest;
};

// Entry in completion tracking table
struct TimeTableEntry {
    int ETA;
    int flightId;
    int runwayId;
};

// Handle entry for state tracking
struct HandlesEntry {
    FlightState state;
    PairingHeapNode<PendingFlight> *pendingNode;
    int submitTime;
    TimeTableEntry timeTableEntry;
};
```

### Comparators

```cpp
// Pending flight comparator (max heap)
// Priority: higher priority > earlier submitTime > smaller flightID
struct CompPendingFlight {
    bool operator()(const PendingFlight &a, const PendingFlight &b) const {
        if (a.priority != b.priority)
            return a.priority > b.priority;
        else if (a.submitTime != b.submitTime)
            return a.submitTime < b.submitTime;
        return a.flightId < b.flightId;
    }
};

// Time table comparator (min heap)
// Priority: earlier ETA > smaller flightID
struct CompTimeTableEntry {
    bool operator()(const TimeTableEntry &a, const TimeTableEntry &b) const {
        if (a.ETA != b.ETA)
            return a.ETA < b.ETA;
        return a.flightId < b.flightId;
    }
};
```

---

## Function Prototypes and Descriptions

### 1. System Initialization
```cpp
void initialize(int runwayCount);
```
**Purpose:** Initialize the system with specified number of runways  
**Parameters:** `runwayCount` - number of runways to create  
**Algorithm:**
- Validates input (runwayCount > 0)
- Creates runways with IDs 1 to runwayCount
- Sets all runways' nextFreeTime to 0
- Sets currentTime to 0

---

### 2. Flight Submission
```cpp
void submitFlight(int flightId, int airlineId, int submitTime, 
                  int priority, int duration);
```
**Purpose:** Submit a new flight request to the system  
**Parameters:**
- `flightId` - unique flight identifier
- `airlineId` - airline identifier
- `submitTime` - time of submission
- `priority` - flight priority (higher = more urgent)
- `duration` - runway usage duration

**Algorithm:**
1. Advance time to submitTime (trigger Phase 1 & 2)
2. Check for duplicate flightId
3. Insert flight into pending queue
4. Update airline index
5. Reschedule all unsatisfied flights
6. Print new flight's ETA and any updated ETAs

---

### 3. Time Advancement
```cpp
void tick(int currentTime);
```
**Purpose:** Advance system time and process all state changes  
**Algorithm:**

**Phase 1 - Settle Completions:**
1. Find all flights with ETA ≤ currentTime
2. Remove from all data structures
3. Print completions in (ETA, flightID) order

**Promotion Step:**
1. Mark flights with startTime ≤ currentTime as IN_PROGRESS
2. Exclude from rescheduling (non-preemptive)

**Phase 2 - Reschedule Unsatisfied Flights:**
1. Collect all PENDING and SCHEDULED-not-started flights
2. Convert scheduled flights back to pending
3. Rebuild runway pool with current availability
4. Apply greedy scheduling:
   - Pick highest priority flight
   - Assign to earliest free runway
   - Update runway availability
5. Track and print ETA changes

---

### 4. Flight Cancellation
```cpp
void cancelFlight(int flightId, int currentTime);
```
**Purpose:** Cancel a flight that hasn't started  
**Algorithm:**
1. Advance time to currentTime
2. Validate flight exists and is cancellable
3. Remove from all data structures
4. Reschedule remaining unsatisfied flights
5. Print cancellation confirmation and updated ETAs

---

### 5. Priority Update
```cpp
void reprioritize(int flightId, int currentTime, int newPriority);
```
**Purpose:** Change priority of a pending/scheduled flight  
**Algorithm:**
1. Advance time to currentTime
2. Validate flight exists and hasn't departed
3. Update priority:
   - For PENDING: use changeKey on pairing heap
   - For SCHEDULED: update in active flights
4. Reschedule all unsatisfied flights
5. Print confirmation and updated ETAs

---

### 6. Add Runways
```cpp
void addRunways(int count, int currentTime);
```
**Purpose:** Add additional runways to increase capacity  
**Algorithm:**
1. Advance time to currentTime
2. Validate count > 0
3. Create new runways with consecutive IDs
4. Set nextFreeTime = currentTime
5. Add to runway pool
6. Reschedule all unsatisfied flights
7. Print confirmation and updated ETAs

---

### 7. Ground Hold
```cpp
void groundHold(int airlineLow, int airlineHigh, int currentTime);
```
**Purpose:** Ground all unsatisfied flights for airlines in range  
**Algorithm:**
1. Advance time to currentTime
2. Validate range (airlineHigh ≥ airlineLow)
3. For each airline in [airlineLow, airlineHigh]:
   - Remove all unsatisfied flights
   - Keep IN_PROGRESS flights running
4. Reschedule remaining unsatisfied flights
5. Print confirmation and updated ETAs

---

### 8. Print Active Flights
```cpp
void printActive();
```
**Purpose:** Display all active flights (pending, scheduled, in-progress)  
**Algorithm:**
1. Collect all flights from active flights table
2. Sort by flightID
3. Print each flight's details
4. Use -1 for unassigned times (pending flights)

---

### 9. Print Schedule
```cpp
void printSchedule(int t1, int t2);
```
**Purpose:** Show scheduled-but-not-started flights with ETA in [t1, t2]  
**Algorithm:**
1. Filter flights where:
   - state == SCHEDULED
   - startTime > currentTime
   - ETA ∈ [t1, t2]
2. Sort by (ETA, flightID)
3. Print flight IDs

---

## Program Structure

### Main Entry Point
```cpp
int main(int argc, char *argv[]);
```
**Flow:**
1. Validate command-line arguments
2. Open input file
3. Create scheduler instance
4. Parse and execute commands line by line
5. Handle Quit() command to write output

### Command Parser
The main function includes inline parsing for each command type:
- `Initialize(runwayCount)`
- `SubmitFlight(flightID, airlineID, submitTime, priority, duration)`
- `CancelFlight(flightID, currentTime)`
- `Reprioritize(flightID, currentTime, newPriority)`
- `AddRunways(count, currentTime)`
- `GroundHold(airlineLow, airlineHigh, currentTime)`
- `Tick(t)`
- `PrintActive()`
- `PrintSchedule(t1, t2)`
- `Quit()`

### Output Management
```cpp
void quit(ifstream &inputFile, char *argv[]);
```
**Purpose:** Write all output to file and terminate  
**Algorithm:**
1. Append termination message
2. Close input file
3. Extract base filename
4. Write output to `<filename>_output_file.txt`
5. Exit program

---

## Scheduling Algorithm

### Greedy Policy (Deterministic)

**For each flight assignment:**

1. **Select next flight:**
   - Highest priority
   - Tie-breaker: earliest submitTime
   - Tie-breaker: smallest flightID

2. **Select runway:**
   - Earliest nextFreeTime
   - Tie-breaker: smallest runwayID

3. **Assign times:**
   ```cpp
   startTime = max(currentTime, runway.nextFreeTime)
   ETA = startTime + duration
   ```

4. **Update runway:**
   ```cpp
   runway.nextFreeTime = ETA
   ```

This ensures deterministic output across all correct implementations.

---

## Complexity Analysis

### Time Complexities

| Operation | Worst Case | Explanation |
|-----------|-----------|-------------|
| Initialize | O(r) | r runways inserted into heap |
| SubmitFlight | O(n log n) | Rescheduling n flights |
| CancelFlight | O(n log n) | Rescheduling after removal |
| Reprioritize | O(n log n) | Priority update + reschedule |
| AddRunways | O(n log n) | Rescheduling with new runways |
| GroundHold | O(n log n) | Remove flights + reschedule |
| Tick | O(k log n + m log m) | k completions, m reschedules |
| PrintActive | O(n log n) | Sort n active flights |
| PrintSchedule | O(n log n) | Filter and sort flights |

where:
- n = number of active flights
- r = number of runways
- k = number of completions
- m = number of unsatisfied flights

### Space Complexity
**O(n + r)** where n is total flights and r is runways

---

## Build and Execution

### Compilation
```bash
make
```
This creates the executable `gatorAirTrafficScheduler`

### Execution
```bash
./gatorAirTrafficScheduler input_file.txt
```

### Output
Results written to: `input_file_output_file.txt`

---

## Implementation Notes

1. **Non-preemptive Scheduling:** Once a flight starts (startTime ≤ currentTime), it cannot be cancelled or rescheduled.

2. **Two-Phase Update:** Every time advancement triggers:
   - Phase 1: Settle completions
   - Phase 2: Reschedule unsatisfied flights

3. **ETA Updates:** Only printed when rescheduling changes a previously assigned ETA.

4. **Custom Data Structures:** Both pairing heap and binary heap implemented from scratch as required.

5. **Deterministic Behavior:** All tie-breaking rules ensure consistent output.

---

## Key Design Decisions

1. **Pairing Heap for Pending Flights:** Provides O(1) insertion and efficient decrease-key operations for priority updates.

2. **Binary Heap for Runways:** Simple and efficient for repeatedly finding earliest available runway.

3. **Hash Tables for Lookups:** Enable O(1) average-case lookup by flightID and airlineID.

4. **Handles Map:** Central registry maintaining pointers to all data structures for efficient cross-updates.

5. **State Tracking:** Explicit state enum prevents invalid operations on flights.

---

## References

- Course: COP 5536 Advanced Data Structures, Fall 2025
- Institution: University of Florida
- Platform: thunder.cise.ufl.edu

---

## Acknowledgments

This project implements the Gator Air Traffic Slot Scheduler as specified in the COP 5536 course requirements, demonstrating advanced data structure concepts including pairing heaps, binary heaps, and efficient scheduling algorithms.
