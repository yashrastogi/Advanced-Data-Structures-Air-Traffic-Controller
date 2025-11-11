
# Gator Air Traffic Slot Scheduler — Project Specification (COP 5536 Fall 2025)*

## Problem Description

Imagine building a system to manage flights at an airport with several runways. Every flight sends a request to the system when it wants a slot to take off or land. The request contains:

* `flightID` (unique integer)
* `airlineID` (integer)
* `submitTime` (integer)
* `priority` (integer; larger = more urgent)
* `duration` (integer minutes needed on a runway)

Once a flight starts using a runway, it cannot be stopped or moved until it finishes (non-preemptive). At any moment flights may be:

* waiting for assignment,
* already reserved for a future start time,
* currently using a runway,
* already finished.

The goal: keep track of flights, decide ordering on runways, and update the plan whenever new requests arrive or time moves forward.

---

## Terms (defined before use)

* **System time (`currentTime`)**: the scheduler's current time. It only changes when an operation explicitly provides a time parameter:

  * flight submission with `submitTime`, or
  * `Tick(t)` where `t` is integer minutes.

* **Flight request fields**:

  * `flightID` (unique integer identifier)
  * `airlineID` (integer)
  * `submitTime` (integer)
  * `priority` (integer; larger = more urgent)
  * `duration` (integer minutes needed on a runway)

* **Start time (`startTime`)**: when a flight begins using its assigned runway (integer minutes).

* **End time / ETA**: `ETA = startTime + duration`.

* **Non-preemptive**: once a flight starts on a runway, it cannot be paused, moved, or shortened.

* **Unsatisfied flights**: flights that have **not** yet started (includes both **Pending** and **Scheduled (not started)** states).

---

## Flight Lifecycle

1. **Pending** — flight is in the request pool and not yet assigned a start time or runway.
2. **Scheduled (not started)** — flight has an assigned runway, `startTime`, and `ETA`, but `currentTime < startTime`.
3. **InProgress** — `startTime ≤ currentTime < ETA`.
4. **Landed** — flight completed at its `ETA` and is removed from the system.

---

## How Time Advances (Two-Phase Update)

Whenever `currentTime` changes (an operation supplies a time), the scheduler performs **two phases** **in order** before the requested operation:

### Phase 1 — Settle completions

* Find all flights with `ETA ≤ currentTime`.
* Mark them **Completed**, remove them from data structures, and print them in ascending `ETA` order; break ties by smaller `flightID`.

### Promotion step (between phases)

* Any flight with `startTime ≤ currentTime` is marked **InProgress** and excluded from rescheduling (non-preemptive).

### Phase 2 — Reschedule Unsatisfied Flights

* Consider all **unsatisfied flights** (Pending or Scheduled-not-started).
* **Clear** previous assignments for unsatisfied flights and **rebuild** the schedule from `currentTime` because the situation may have changed (completions, new requests, runway availability).
* **Seeding runway availability**: use a fresh runway pool seeded at `currentTime`. For in-progress runways set `nextFreeTime` to that flight’s `ETA` so occupied runways stay blocked.
* **Greedy scheduling policy (deterministic & fair)**:

  1. Choose the **next flight**: highest priority; tie-breakers: earlier `submitTime`, then smaller `flightID`. Implement via **max pairing heap** with key `(priority, -submitTime, -flightID)`.
  2. Choose the **runway**: earliest `nextFreeTime`; tie-breaker: smaller `runwayID`. Implement via **binary min-heap** keyed by `(nextFreeTime, runwayID)`.
  3. Assign times:

     * `startTime = max(currentTime, runway.nextFreeTime)`
     * `ETA = startTime + duration`
  4. Update runway availability: `runway.nextFreeTime = ETA`
  5. Repeat until all unsatisfied flights are assigned.
* **Updated ETAs**:

  * If any not-yet-started flight’s `ETA` changes compared to its previously assigned value, print exactly one line:

    ```
    Updated ETAs: [flightID1: ETA1, flightID2: ETA2, …]
    ```

    Include only flights whose ETAs changed, sorted by increasing `flightID`.
* After Phases 1 & 2 are complete, perform the requested operation (add runway, submit flight, reprioritize, cancel, ground-hold, etc.).
* If the requested operation changes the set of unsatisfied flights, run Phase 2 **again** immediately after and print Updated ETAs if applicable.

---

## Data Structures (required; some must be implemented from scratch)

> **Note:** Max pairing heap and binary min-heap must be implemented from scratch. Hash tables may use standard library implementations.

1. ### Pending Flights Queue — **Max Pairing Heap (Two-Pass Scheme)**

   * **Key**: `(priority, -submitTime, -flightID)` (max-heap over the triple).
   * **Payload**: pointer to flight record.
   * **Ops required**: `push`, `pop` (extract best), `increase-key` (when priority increases), `erase` by handle (for cancel or ground-hold).
   * **Implementation note**: use the two-pass max pairing heap scheme and store a handle/pointer to each node in the per-flight record for O(log n) updates.

2. ### Runway Pool — **Binary Min Heap**

   * **Key**: `(nextFreeTime, runwayID)`
   * **Payload**: `{ runwayID, nextFreeTime }`
   * Always pick earliest free runway for assignment, update `nextFreeTime` and push back.

3. ### Active Flights — **Hash Table (by flightID)**

   * **Key**: `flightID`
   * **Payload**: `{ airlineID, priority, duration, runwayID, startTime, ETA }`
   * Used for lookups by ID for cancellation, reprioritize, and printing. For printing, collect from the hash table and sort by `flightID`.

4. ### Timetable (Completions Queue) — **Binary Min Heap**

   * **Key**: `(ETA, flightID)`
   * **Payload**: `{ runwayID }` (other fields via Active Flights)
   * Supports: `insert`, `pop all with ETA ≤ currentTime`

5. ### Airline Index — **Hashtable**

   * **Role**: group unsatisfied flights by `airlineID`, efficient airline-wide operations (like GroundHold).
   * **Key → Value**: `airlineID → set/list of flightIDs`
   * Maintain as state changes; iterate set when applying a hold.

6. ### Handles — **HashTable**

   * **Role**: central map with references where each flight lives in other structures for coordinated, O(1) lookup and O(log n) updates.
   * **Key → Value**: `flightID → { state, pairingNode?, ... }`
   * At minimum, store pairing-heap node handle for pending.

---

## System Rules — Summary

1. **State transitions**

   * A flight enters **InProgress** when `currentTime = startTime`.
   * A flight becomes **Completed** when `currentTime ≥ ETA`.

2. **Unsatisfied flights**

   * `Unsatisfied = Pending ∪ Scheduled (not started)`.
   * Only unsatisfied flights are rescheduled in Phase 2.

3. **Completions (Phase 1 output)**

   * Print flights that complete (`ETA ≤ currentTime`) in ascending `ETA` order, tie by smaller `flightID`.

4. **Updated ETAs (Phase 2 output)**

   * Print only if a flight’s `ETA` changed after rescheduling; include only those flights, sorted by `flightID`.

5. **Order of scheduling & operations**

   * Always run Phase 1 (settle completions) and Phase 2 (reschedule) before applying the requested operation.
   * If the operation changes unsatisfied flights, run Phase 2 again and print Updated ETAs if needed.

6. **Determinism**

   * Ties between flights: earlier `submitTime`, then smaller `flightID`.
   * Ties between runways: smaller `runwayID`.
   * Ensures identical outputs for correct implementations.

---

## Operations

> **Important rule:** If an operation has a time parameter, advance (settle completions + reschedule + possibly Updated ETAs) **before** executing the operation itself. If the operation modifies unsatisfied flights, run Phase 2 again and print Updated ETAs if needed.

### 1. `Initialize(runwayCount)`

**Purpose:** Start system with `runwayCount` runways.

**Behavior:**

* If `runwayCount ≤ 0` → print `Invalid input.`
* Else:

  * Create runways with IDs `1 ... runwayCount`.
  * Set each runway `nextFreeTime = 0`.
  * Set `currentTime = 0`.

**Outputs:**

* Valid:

  ```
  <runwayCount> Runways are now available
  ```
* Invalid:

  ```
  Invalid input. Please provide a valid number of runways.
  ```

---

### 2. `SubmitFlight(flightID, airlineID, submitTime, priority, duration)`

**Purpose:** Add a new flight request at `submitTime`.

**Behavior:**

* Advance time to `submitTime` (settle completions + reschedule).
* If `flightID` already exists → print `Duplicate flight` and stop.
* Else:

  * Insert flight into Pending Flights Queue.
  * Reschedule all unsatisfied flights using the greedy policy.
  * Print the new flight's assigned `ETA`.
* If rescheduling causes other unsatisfied flights to get new ETAs → print `Updated ETAs: [...]` listing only those flights sorted by `flightID`.

**Outputs:**

* On success (after reschedule):

  ```
  Flight <flightID> scheduled - ETA: <ETA>
  ```
* If duplicate:

  ```
  Duplicate FlightID
  ```
* If other ETAs changed:

  ```
  Updated ETAs: [<flightID1>: <ETA1>, ...]
  ```

**Example walkthrough** is in the document (omitted here for brevity).

---

### 3. `CancelFlight(flightID, currentTime)`

**Purpose:** Remove a flight that has not started yet.

**Behavior:**

* Advance time to `currentTime` → settle completions → reschedule unsatisfied.
* Lookup `flightID`.

  * If not found → print `Flight <flightID> does not exist`.
  * If In Progress or Completed → print `Cannot cancel: Flight <id> has already departed.`
  * Else (Pending / Scheduled-not-started):

    * Remove it from Pending / Active / Timetable / Airline index / Handles.
    * Reschedule remaining unsatisfied flights from `currentTime`.
    * Print: `Flight <id> has been canceled.`
    * If other ETAs changed → print `Updated ETAs: [...]`.

**Outputs:**

* Removed (pending or not started):

  ```
  Flight <flightID> has been canceled
  ```
* Already in progress or landed:

  ```
  Cannot cancel. Flight <flightID> has already departed
  ```
* Not found:

  ```
  Flight <flightID> does not exist
  ```
* If ETAs changed after reschedule:

  ```
  Updated ETAs: [<flightID1>: <ETA1>, ...]
  ```

---

### 4. `Reprioritize(flightID, currentTime, newPriority)`

**Purpose:** Change a flight’s priority before it starts and rebuild schedule.

**Behavior:**

* Advance time to `currentTime` → settle completions → reschedule unsatisfied.
* Lookup `flightID`.

  * If not found → print `Flight <id> does not exist.`
  * If In Progress or Completed → print `Cannot reprioritize: Flight <id> has already departed.`
  * Else (Pending / Scheduled-not-started):

    * Update priority:

      * If priority increases: pairing heap `increase-key`.
      * If priority decreases: erase + insert.
    * Reschedule all unsatisfied flights from `currentTime`.
    * Print: `Priority of Flight <id> has been updated to <newPriority>.`
    * If other flights’ ETAs changed → print single `Updated ETAs: [...]`.

**Outputs:**

* Updated:

  ```
  Priority of Flight <flightID> has been updated to <newPriority>
  ```
* Already in progress or landed:

  ```
  Cannot reprioritize. Flight <flightID> has already departed
  ```
* Not found:

  ```
  Flight <flightID> not found
  ```
* If ETAs changed:

  ```
  Updated ETAs: [<flightID1>: <ETA1>, ...]
  ```

---

### 5. `AddRunways(count, currentTime)`

**Purpose:** Add `count` new runways available at `currentTime`, then reschedule.

**Behavior:**

* Advance time to `currentTime` → settle completions → reschedule unsatisfied.
* If `count ≤ 0` → print `Invalid input.`
* Else:

  * Create `count` new runways with consecutive IDs; set each `nextFreeTime = currentTime`; push into runway heap.
  * Reschedule unsatisfied flights from `currentTime`.
  * Print confirmation and, if any ETAs changed, print `Updated ETAs: [...]`.

**Outputs:**

* Valid:

  ```
  Additional <count> Runways are now available
  ```
* Invalid:

  ```
  Invalid input. Please provide a valid number of runways.
  ```
* If ETAs changed:

  ```
  Updated ETAs: [<flightID1>: <ETA1>, ...]
  ```

---

### 6. `GroundHold(airlineLow, airlineHigh, currentTime)`

**Purpose:** Temporarily block unsatisfied flights whose `airlineID` ∈ `[airlineLow, airlineHigh]` from being scheduled.

**Behavior:**

* Advance time to `currentTime` → settle completions → reschedule unsatisfied.
* If `airlineHigh < airlineLow` → print `Invalid input. Please provide a valid airline range.`
* Else:

  * Remove all unsatisfied flights with `airlineID` in range from all structures (pending/active/timetable/airline-index/handles).

    * Note: flights **In Progress** are **unaffected**.
  * Reschedule remaining unsatisfied flights from `currentTime`.
  * Print: `Flights of the airlines in the range [low, high] have been grounded`
  * If any ETAs changed → print `Updated ETAs: [...]`.

**Outputs:**

* Valid range:

  ```
  Flights of the airlines in the range [<airlineLow>, <airlineHigh>] have been grounded
  ```
* Invalid range:

  ```
  Invalid input. Please provide a valid airline range.
  ```
* If ETAs changed:

  ```
  Updated ETAs: [<flightID1>: <ETA1>, ...]
  ```

---

### 7. `PrintActive()`

**Purpose:** Show all flights still in the system (pending, scheduled-not-started, or in-progress).

**Behavior:**

* Do **not** change `currentTime`.
* Gather flights from Active Flights hash table, sort by `flightID`.
* Print one line per flight:

  ```
  [flight<flightID>, airline<airlineID>, runway<runwayID>, start<startTime>, ETA<ETA>]
  ```

  * For **Pending** flights with no assigned times, print `startTime = -1` and `ETA = -1`.

**Outputs:**

* If flights exist (ordered by `flightID`):

  ```
  [flight<flightID>, airline<airlineID>, runway<runwayID>, start<startTime>, ETA<ETA>]
  ```
* If none:

  ```
  No active flights
  ```

---

### 8. `PrintSchedule(t1, t2)`

**Purpose:** Show **only unsatisfied** flights (Scheduled-but-not-started) whose ETA ∈ `[t1, t2]`.

**Behavior:**

* Do **not** change `currentTime`.
* Gather flights from Active Flights table.
* Filter flights that:

  * `state == SCHEDULED`
  * `startTime > currentTime` (not started yet)
  * `ETA ∈ [t1, t2]`
* Ignore Pending and InProgress flights.
* Sort results by `(ETA, flightID)` before printing.

**Outputs:**

* If flights exist (ordered by ETA, then flightID), print each flightID on its own line:

  ```
  [<flightID>]
  ```
* If none:

  ```
  There are no flights in that time period
  ```

---

### 9. `Tick(t)`

**Purpose:** Advance system clock to `t`, land flights that finish by `t`, then reschedule.

**Behavior:**

* Advance `currentTime` → `t`.
* Phase 1: complete & print flights with `ETA ≤ t` in `(ETA, flightID)` order; remove them from structures.
* Phase 2: reschedule unsatisfied from `t`.
* If any ETAs changed → print `Updated ETAs: [...]`.

**Outputs:**

* For each landed flight (ordered by ETA, tie by flightID):

  ```
  Flight <flightID> has landed at time <ETA>
  ```
* If none landed and no ETAs changed: print nothing.
* If ETAs changed after reschedule:

  ```
  Updated ETAs: [<flightID1>: <ETA1>, ...]
  ```

---

### 10. `Quit()`

**Purpose:** End processing immediately.

**Behavior:** Print termination line.

**Output:**

```
Program Terminated!!
```

---

## Programming Environment & Execution

* Acceptable languages: **Java, C++, or Python**.
* Program will be tested on `thunder.cise.ufl.edu` server.
* Submission must include a **Makefile** that creates an executable named `gatorAirTrafficScheduler`.
* Execution commands:

  * C/C++: `./gatorAirTrafficScheduler file_name`
  * Java: `java gatorAirTrafficScheduler file_name`
  * Python: `python3 gatorAirTrafficScheduler file_name`
* `file_name` is the input test data file.

---

## Input and Output Requirements

* Read input from a text file specified as a **command-line argument**.
* Write output to a text file named: `input_filename_output_file.txt`
  (e.g., `test1.txt` → `test1_output_file.txt`).
* Program should terminate when the operation encountered is `Quit()`.

---

## Examples (Input / Output)

### Example 1 — Input

```
Initialize(2)
SubmitFlight(201, 1, 0, 5, 4)
SubmitFlight(202, 2, 0, 6, 4)
SubmitFlight(203, 1, 0, 4, 5)
PrintSchedule(1, 10)
SubmitFlight(205, 4, 2, 7, 2)
PrintSchedule(4, 6)
Reprioritize(203, 3, 9)
GroundHold(5, 4, 3)
GroundHold(4, 4, 3)
AddRunways(1, 3)
SubmitFlight(206, 6, 3, 6, 4)
CancelFlight(206, 3)
Tick(4)
SubmitFlight(204, 3, 4, 8, 2)
AddRunways(0, 5)
PrintActive()
PrintSchedule(5, 10)
Quit()
```

### Example 1 — Output

```
2 Runways are now available
Flight 201 scheduled - ETA: 4
Flight 202 scheduled - ETA: 4
Flight 203 scheduled - ETA: 9
[203]
Flight 205 scheduled - ETA: 6
[205]

Priority of Flight 203 has been updated to 9
Invalid input. Please provide a valid airline range.
Flights of the airlines in the range [4, 4] have been grounded
Additional 1 Runways are now available
Updated ETAs: [203: 8]
Flight 206 scheduled - ETA: 8
Flight 206 has been canceled
Flight 201 has landed at time 4
Flight 202 has landed at time 4
Flight 204 scheduled - ETA: 6
Invalid input. Please provide a valid number of runways.
[flight203, airline1, runway3, start3, ETA8]
[flight204, airline3, runway1, start4, ETA6]
There are no flights in that time period
Program Terminated!!
```

---

### Example 2 — Input

```
Initialize(2)
SubmitFlight(301, 5, 0, 7, 4)
SubmitFlight(302, 6, 0, 6, 5)
SubmitFlight(303, 7, 0, 5, 3)
PrintSchedule(5, 8)
GroundHold(6, 6, 0)
SubmitFlight(301, 5, 0, 7, 4)
SubmitFlight(304, 8, 1, 9, 4)
Reprioritize(304, 1, 10)
PrintSchedule(4, 8)
AddRunways(1, 1)
PrintSchedule(4, 8)
SubmitFlight(305, 9, 2, 6, 3)
CancelFlight(305, 2)
AddRunways(0, 2)
Tick(3)
PrintActive()
PrintSchedule(3, 7)
Tick(4)
Quit()
```

### Example 2 — Output

```
2 Runways are now available
Flight 301 scheduled - ETA: 4
Flight 302 scheduled - ETA: 5
Flight 303 scheduled - ETA: 7

[303]
Flights of the airlines in the range [6, 6] have been grounded
Duplicate FlightID
Flight 304 scheduled - ETA: 8
Updated ETAs: [303: 8]
Priority of Flight 304 has been updated to 10
[303]
[304]
Additional 1 Runways are now available
Updated ETAs: [303: 7, 304: 5]
[303]
Flight 305 scheduled - ETA: 7
Updated ETAs: [303: 8]
Flight 305 has been canceled
Updated ETAs: [303: 7]
Invalid input. Please provide a valid number of runways.
[flight301, airline5, runway1, start0, ETA4]
[flight302, airline6, runway2, start0, ETA5]
[flight303, airline7, runway1, start4, ETA7]
[flight304, airline8, runway3, start1, ETA5]
[303]
Flight 301 has landed at time 4
Program Terminated!!
```

---

## Submission Requirements

* **Makefile** that compiles source and produces an executable `gatorAirTrafficScheduler`.
* **Source program** (C++/Java/Python), well-commented.
* **Report (PDF)** with project details, function prototypes, and explanations:

  * Include student info: Name, UFID, UF Email.
  * Present function prototypes and program structure.
* Submit a single ZIP named `LastName_FirstName.zip` with all files in the top-level directory (no nested directories).
* Submit on Canvas (do not email TA). Late submissions not accepted.

---

## Grading Policy

* Correct implementation and execution: **25 pts**
* Comments and readability: **15 pts**
* Report: **20 pts**
* Testcases: **40 pts**

Penalty deductions include (examples):

* Source files not in single directory after unzip: `-5`
* Incorrect output file name: `-5`
* Error in Makefile: `-5`
* Makefile does not produce executable with required commands: `-5`
* Hard-coded input file name instead of argument: `-5`
* Not following output format: `-5`
* Other input/output or submission requirement violations: `-3`

Programs may be checked for correctness and runtime performance. Plagiarism detection will be used.

---

## Miscellaneous

* Implement **Pairing Heap** and **Binary min-heap** from scratch (do not use built-in heap libraries).
* Work individually (discussion allowed) — plagiarism checks will be performed.