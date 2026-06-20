# Port Skeet 3-Matrix Safepoint + Find Loop + NL Backtrack

Port Skeet's `CanSafepoint` 3-matrix intersection check, the `find()` target loop with latest+oldest history hitbox setup, and Neverlose's extended backtrack system into the fatality source.

## Proposed Changes

### Math Utilities — Missing Functions

#### [MODIFY] [math.h](file:///c:/Users/aandr/Desktop/! IMPORTANT/fatality-source-fixed-main/sdk/math.h)

Add 3 missing math functions that Skeet's `CanSafepoint` uses:

```cpp
// Inverse vector transform (world-space → bone-space)
void vector_itransform(const vec3_t& in, const matrix3x4_t& matrix, vec3_t& out);

// Inverse vector rotate (world-space rotation → bone-space rotation)
void vector_irotate(const vec3_t& in, const matrix3x4_t& matrix, vec3_t& out);

// AABB/OBB bounding box intersection test (for non-capsule hitboxes)
bool intersect_bb(const vec3_t& start, const vec3_t& end, const vec3_t& mins, const vec3_t& maxs);
```

Implementations:
- `vector_itransform`: Computes `in - matrix.origin` then dots against each row
- `vector_irotate`: Dots input against each matrix row (no translation)
- `intersect_bb`: Ray-AABB slab intersection test

---

### Aimbot — 3-Matrix Safepoint Check

#### [MODIFY] [hitscan.h](file:///c:/Users/aandr/Desktop/! IMPORTANT/fatality-source-fixed-main/features/hitscan.h)

Add a helper struct for the hitbox data used by the 3-matrix check and update `is_safe_point` signature:

```cpp
struct hitbox_data_t {
    int   hitbox_id;
    bool  is_obb;
    float radius;
    vec3_t mins;
    vec3_t maxs;
    int   bone;
};

// New helper
void calculate_hitbox_data(hitbox_data_t& out, player_t* player, int hitbox, BoneArray* matrix);
```

#### [MODIFY] [hitscan.cpp](file:///c:/Users/aandr/Desktop/! IMPORTANT/fatality-source-fixed-main/features/hitscan.cpp)

**Replace** the existing `is_safe_point()` with Skeet's 3-matrix intersection logic:

1. Add `calculate_hitbox_data()` — transforms a hitbox against a given matrix to get mins/maxs/radius/bone
2. Rewrite `is_safe_point()` to:
   - Calculate eye position and forward direction to the aim point
   - Call `calculate_hitbox_data()` 3 times: once for `m_left_bones`, once for `m_right_bones`, once for `m_center_bones`
   - For OBB hitboxes: use `vector_itransform` + `vector_irotate` to convert to bone space, then `intersect_bb`
   - For capsule hitboxes: use `intersect_capsule`
   - Count hits across all 3 matrices — return `true` only if all 3 intersect (`hits >= 3`)
   - Fallback: if a record doesn't have left/right/center matrices, fall back to single-matrix check

---

### Aimbot — Find Loop with Latest+Oldest + History Hitbox Setup

#### [MODIFY] [aimbot.cpp](file:///c:/Users/aandr/Desktop/! IMPORTANT/fatality-source-fixed-main/features/aimbot.cpp)

Rewrite `find_best_target()` to match Skeet's `find()` structure:

1. **Breaking lag comp path**: If lag comp is broken for a player, only scan the latest record (no oldest)
2. **Normal path**:
   - Get **latest** record → validate → `scan(player, latest, false)` (history=false)
   - Get **oldest** record → validate it's different from latest, not dormant, not immune → `scan(player, oldest, true)` (history=true)
3. **Target sorting**: Use the existing `compare_targets()` with FOV/damage/distance modes
4. **Early break**: If we found a target with damage > 0, break out (matching Skeet's `break` at end of else branch)

---

### Backtrack — Neverlose Extended Backtrack System

#### [MODIFY] [lag_compensation.h](file:///c:/Users/aandr/Desktop/! IMPORTANT/fatality-source-fixed-main/features/lag_compensation.h)

Add extended backtrack methods:

```cpp
struct lag_compensation_t {
    // ... existing ...
    
    // NL extended backtrack
    bool is_record_in_window(float sim_time, float window);
    void build_backtrack_records(int player_index, float window,
                                std::vector<lag_record_t*>& out);
    void filter_extended_backtrack(const std::vector<lag_record_t*>& source,
                                  std::vector<lag_record_t*>& out);
    
    // Extended backtrack toggle
    bool m_extended_enabled;
};
```

#### [MODIFY] [lag_compensation.cpp](file:///c:/Users/aandr/Desktop/! IMPORTANT/fatality-source-fixed-main/features/lag_compensation.cpp)

Implement Neverlose's layered backtrack system:

1. **`is_record_in_window()`** — Port of `IsBacktrackRecordInsideWindow_41400BF0`:
   - Compute latency correction from net channel info (outgoing + incoming lerp)
   - Compute `correction = min(raw_max_unlag, max(0, cap_sum))`
   - Check `abs(sim_time - ref_time + correction) <= window`

2. **`build_backtrack_records()`** — Port of `BuildBacktrackRecordList_414A5310`:
   - Iterate the player's record deque
   - Skip records with certain flags
   - Call `is_record_in_window()` for each
   - Push valid records to output

3. **`filter_extended_backtrack()`** — Port of `FilterOrSelectExtendedBacktrackVector_414AB700`:
   - If source has ≤ 2 records, just copy (no extended work needed)
   - Check if extended backtrack is enabled in config
   - Apply timing gates (interval checks)
   - For large sets: sparse selection keeping newest + records with tick markers

4. **The NL fix**: Use the full `max_backtrack_window` (0.2s) instead of subtracting one tick interval, giving us the full 200ms backtrack window

#### [MODIFY] [aimbot.cpp](file:///c:/Users/aandr/Desktop/! IMPORTANT/fatality-source-fixed-main/features/aimbot.cpp)

Update `find_best_target()` to optionally use the extended backtrack record list when iterating records, instead of just front/back of the deque.

---
HI
## Verification Plan

### Automated Tests
- Build the project: `msbuild internal_hvh.sln /p:Configuration=Prerelease /p:Platform=Win32`
- Verify no compile errors

### Manual Verification
- The 3-matrix safepoint will require in-game testing to verify hits register correctly
- Extended backtrack records should allow hitting further back in time
