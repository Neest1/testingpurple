#include <cstdint>
#include <cmath>

static constexpr uintptr_t G_HISTORY_BASE_PTR        = 0x424B004C;
static constexpr uintptr_t G_GLOBALS_PTR             = 0x4223CA04;
static constexpr uintptr_t G_MAX_BACKTRACK_WINDOW    = 0x4205A9FC; // float 0.200000003
static constexpr uintptr_t G_EXTENDED_CFG_BASE_PTR   = 0x424F639C;
static constexpr uintptr_t G_CURRENT_TICK_PTR        = 0x424F4B7C;
static constexpr uintptr_t G_RECORD_AGE_LIMIT_PTR    = 0x4223DC24;

static constexpr uintptr_t G_EXT_FILTER_STATE_PTR    = 0x424F4B74;
static constexpr uintptr_t G_EXT_FILTER_TIME_PTR     = 0x424B040C;
static constexpr uintptr_t G_FLOAT_001               = 0x4205AA00; // float 0.001
static constexpr uintptr_t G_FLOAT_HALF              = 0x4205A518; // float 0.5
static constexpr uintptr_t G_FLOAT_VEC_HALF          = 0x4205A520; // vector {0.5,0.5,...}
static constexpr uintptr_t G_FLOAT_ONE               = 0x4205A554; // float 1.0 in observed normalization checks

static constexpr uintptr_t G_VIEW_OR_AIM_ANGLE       = 0x424AFD14;
static constexpr uintptr_t G_ARRAY_A_42504EDC        = 0x42504EDC;
static constexpr uintptr_t G_ARRAY_B_42504EEC        = 0x42504EEC;

static constexpr uintptr_t G_BYTE_425009DC           = 0x425009DC;
static constexpr uintptr_t G_MODE_COUNT_42504F3C     = 0x42504F3C;
static constexpr uintptr_t G_COUNT_42504F1C          = 0x42504F1C;
static constexpr uintptr_t G_TICK_A_4223E328         = 0x4223E328;
static constexpr uintptr_t G_TICK_B_42241A3C         = 0x42241A3C;
static constexpr uintptr_t G_PTR_4223F428            = 0x4223F428;
static constexpr uintptr_t G_ORDER_A_425007F8        = 0x425007F8;
static constexpr uintptr_t G_ORDER_B_4223FE28        = 0x4223FE28;
static constexpr uintptr_t G_FLOAT_42077FC8          = 0x42077FC8;


// pseudocode helpers.
// in this reconstruction, Read<T>(addr) represents a direct memory read in the
// target process.

template <typename T>
T Read(uintptr_t address);

template <typename T>
void Write(uintptr_t address, T value);

int32_t FallbackCurrentTick();
float ComputeFovOrAngleLikeValueFromDirection(float x, float y, float z);
bool SomeGlobalFastPath_413F6DB0();
bool Helper_414F21D0();

struct Vector3 {
    float x;
    float y;
    float z;
};

static inline Vector3 operator+(const Vector3& a, const Vector3& b) {
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

static inline Vector3 operator-(const Vector3& a, const Vector3& b) {
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

static inline Vector3 operator*(const Vector3& a, float s) {
    return { a.x * s, a.y * s, a.z * s };
}

static inline float Length(const Vector3& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

/*
[4223CA04] points to this or an engine global like structure.

+0x0C  timeish value used by 414AB700 extended filter
+0x10  curtime like value
+0x14  interval/frame like value, observed 0.015625
+0x20  interval_per_tick, observed 0.015625
*/

struct GlobalsLike {
    uint8_t pad_00[0x0C];

    float time_or_fraction_0C;     // +0x0C, used in Extended Backtrack gate
    float curtime_like_10;         // +0x10, observed incrementing by interval
    float interval_like_14;        // +0x14, observed 0.015625

    uint8_t pad_18[0x08];

    float interval_per_tick_20;    // +0x20, observed 0.015625
};


// This is the record stored in the player history ring and later passed into the
// shot candidate builder.
struct BacktrackRecord {
    // +0x00
    Vector3 origin;

    // +0x0C..+0x23
    // often zero in the simple standing traces. could hold velocity/angles/etc
    uint8_t unknown_0C[0x18];

    // +0x24
    // collision bbox mins
    // observed normal values:
    //   {-16, -16, 0}
    Vector3 mins;

    // +0x30
    // collision bbox maxs
    // observed normal values:
    //   {16, 16, 72}
    Vector3 maxs;

    // +0x3C
    int32_t unknown_3C;

    // +0x40
    // small state like value, observed 3 or 4
    int32_t sequence_or_state_40;

    // +0x44
    // often 1.0f as float in records. exact semantic unknown
    int32_t unknown_44;

    // +0x48
    int32_t unknown_48;

    // +0x4C
    // older tick like value used by some helper calls
    int32_t older_tick_like_4C;

    uint8_t unknown_50[0x14];

    // +0x64..+0x78
    // angle/animation/pose-ish floats. exact names unknown
    float angle_or_pose_64;
    float angle_or_pose_68;
    float angle_or_pose_6C;
    float angle_or_pose_70;
    float angle_or_pose_74;
    float angle_or_pose_78;

    int32_t unknown_7C;

    // +0x80
    // observed 0x301 frequently
    int32_t flags_or_state_80;

    // +0x84
    int32_t sim_tick;

    // +0x88
    int32_t sim_tick_duplicate;

    uint8_t unknown_8C[0x08];

    // +0x94
    float simulation_time;

    // +0x98
    int32_t unknown_98;

    // +0x9C
    float fraction_or_anim_9C;

    // +0xA0
    // Observed 64.0
    float constant_64_A0;

    // +0xA4
    // 41400BF0 writes/uses byte at this address as a validity byte.
    // dumps often print the dword here as 0x01000001 because adjacent bytes
    // are also populated; the actual byte used is +0xA4.
    uint8_t valid_byte_A4;

    uint8_t unknown_A5;
    uint8_t unknown_A6;
    uint8_t unknown_A7;

    uint8_t unknown_A8[0x08];

    // +0xB0
    //
    // known behavior:
    //   bit 0x08:
    //      builder stops scanning when this is set.
    //
    //   bit 0x800:
    //      builder does an age-limit check:
    //        age = current_tick - rec->sim_tick
    //        continue only if age <= half_limit
    //
    //   bit 0x10:
    //      4149C000 status tail stores:
    //        item+0x16 = (record_flags >> 4) & 1
    //
    //   mask 0x81:
    //      4149C000 candidate type:
    //        type = (((record_flags & 0x81) == 0x80) ? 1 : 0) + 3
    //
    // observed common values:
    //   0x20
    //   0x100
    //   0x820
    //   0x420
	//   0x421
    int32_t record_flags;

    // +0xB4
    // tick like field. often -1 before selection, later sometimes filled with
    // a future/current shot tick value by the vector filter/consumer path.
    int32_t linked_or_selected_tick_B4;

    // +0xB8
    // tick-like field. often -1 before selection, later sometimes filled with
    // source/base tick.
    int32_t linked_or_selected_tick_B8;

    // +0xBC
    int32_t unknown_BC;

    // +0xC0...
    // matrix/bones/animation/state data begins here. not fully documented.
};

/*
standard three-pointer vector layout used repeatedly:

  begin
  end
  capacity

for vectors of BacktrackRecord*:
  count = (end - begin) / 4
*/
template <typename T>
struct PointerVector {
    T* begin;
    T* end;
    T* capacity;

    int32_t Count() const {
        return int32_t(end - begin);
    }

    bool Empty() const {
        return begin == end;
    }
};

/*
4149C000 builds one of these from a BacktrackRecord*.

caller stores these in a vector with stride 0x170 bytes:

  4149BCA0 - lea ecx,[eax+00000170]
  4149BCA6 - mov [edx+04],ecx

fields:
  +0x00 target/entity pointer was already present before 4149C000.
  +0x04 record pointer written by 4149C000.
  +0x08 center x.
  +0x0C center y.
  +0x10 center z.
  +0x14 status: timing-window result from 41400BF0 in slow branch,
        or forced 1 in fast/simple branch.
  +0x15 validity byte written by 41400BF0 in slow branch,
        or forced 1 in fast/simple branch.
  +0x16 record flag bit:
        (record_flags >> 4) & 1 in slow branch,
        or forced 0 in fast/simple branch.
  +0x17 special link byte set near 4149C3E7 under an extra helper condition.
  +0x18 / +0x1D / +0x1E are also set/copied by the caller around 4149BC10,
        not primarily by 4149C000.
  +0xD0 distance.
  +0xD4 angular/FOV-like value.
  +0xD8 candidate type.
*/
struct ShotCandidateItem {
    void* target;                    // +0x00, pre-filled by caller
    BacktrackRecord* record;         // +0x04, written by 4149C000

    Vector3 center;                  // +0x08..+0x10

    uint8_t in_time_window_14;        // +0x14
    uint8_t valid_after_check_15;     // +0x15
    uint8_t record_flag_0x10_16;      // +0x16
    uint8_t special_link_17;          // +0x17

    uint8_t caller_flag_18;           // +0x18
    uint8_t pad_19[0x04];             // +0x19..+0x1C
    uint8_t caller_flag_1D;           // +0x1D
    uint8_t caller_flag_1E;           // +0x1E

    uint8_t unknown_1F_to_CF[0xD0 - 0x1F];

    float distance;                  // +0xD0
    float fov_or_angle;              // +0xD4
    int32_t candidate_type;           // +0xD8

    uint8_t rest[0x170 - 0xDC];
};

/*
signature reconstructed from callsite 414A5452 and later 4149C3AD:

  414A5452 call:
    [esp+00] = record simulation time
    [esp+04] = window
    [esp+08] = record + 0xA4 validity byte pointer
    [esp+0C] = &tick_a
    [esp+10] = &tick_b

  4149C3AD call:
    [esp+00] = record simulation time
    [esp+04] = window
    [esp+08] = item + 0x15 validity byte pointer
    [esp+0C] = &tick_a
    [esp+10] = &tick_b

Important:
  This function has two effects:

    1. it writes a validity byte through the pointer argument.

    2. it returns a bool in AL/BL indicating whether record_simtime is inside
       the corrected time window.

the final compare was fully traced:

  ref_tick_f = tick_b
  interval   = globals interval
  raw_corr   = 0.200000003
  cap_a      = [esi+0x435940]
  cap_b      = [esi+0x435944]

  cap_sum = cap_a + cap_b

  correction =
      raw_corr < cap_sum
          ? raw_corr
          : max(0.0f, cap_sum)

because raw_corr is positive, this is equivalent to:

  correction = min(raw_corr, max(0.0f, cap_a + cap_b))

then:

  ref_time = tick_b * interval

  delta = abs(record_simtime - ref_time + correction)

  return delta <= window

observed example:
  raw_corr   = 0.200000003
  cap_a      = 0.031250000
  cap_b      = 0.015689956
  cap_sum    = 0.046939956
  correction = 0.046939956

last passing record:
  delta  = 0.171810044
  window = 0.184375003
  AL=1

next failing record:
  delta  = 0.187435044
  window = 0.184375003
  AL=0
*/
bool IsBacktrackRecordInsideWindow_41400BF0(
    float record_simtime,
    float window,
    uint8_t* validity_byte_out,
    int32_t* tick_a,
    int32_t* tick_b
) {
    GlobalsLike* globals = Read<GlobalsLike*>(G_GLOBALS_PTR);

    float interval = globals
        ? globals->interval_per_tick_20
        : 0.015625f;

    /*
        raw_max_unlag comes from a virtual/helper call inside 41400BF0:
          41400D08 - mov eax,[ecx]
          41400D0A - call dword ptr [eax+30]
          41400D0D - fstp dword ptr [esp+08]

        Observed value:
          0.200000003f
    */
    float raw_max_unlag = Read<float>(G_MAX_BACKTRACK_WINDOW);

    /*
        cap_a and cap_b are read from a large state object in ESI:

          41400D17 - movss xmm2,[esi+00435944]
          41400D1F - addss xmm2,[esi+00435940]

        i do not yet have the exact original owner/name of ESI.

        my guess:
          cap_a/cap_b are latency/interpolation/lerp correction components.

        but the exact math is known even if the names are not.
    */
    float cap_a = 0.031250000f;     // observed representative value
    float cap_b = 0.015689956f;     // observed representative value

    float cap_sum = cap_a + cap_b;

    float correction;
    if (raw_max_unlag < cap_sum) {
        correction = raw_max_unlag;
    } else {
        correction = (cap_sum > 0.0f) ? cap_sum : 0.0f;
    }

    /*
        validity byte update.

        disassembly summary:
          tick_for_valid = tick_a ? *tick_a : fallback
          valid_time     = tick_for_valid * interval
          valid_time    -= raw_max_unlag
          valid_time_i   = trunc(valid_time)
          *validity_byte = float(valid_time_i) < record_simtime

        this explains traces where records entered with valid=0 and left with
        valid=1 before being accepted into the source vector.
    */
    if (validity_byte_out) {
        int32_t tick_for_valid = tick_a
            ? *tick_a
            : FallbackCurrentTick();

        float valid_time = float(tick_for_valid) * interval;
        valid_time -= raw_max_unlag;

        int32_t valid_time_i = int32_t(valid_time); // cvttps2dq truncation
        float valid_time_truncated = float(valid_time_i);

        *validity_byte_out = (valid_time_truncated < record_simtime) ? 1 : 0;
    }

    /*
        final timing predicate.

        tick_b is the reference tick for the final compare
    */
    int32_t tick_for_window = tick_b
        ? *tick_b
        : FallbackCurrentTick();

    float ref_time = float(tick_for_window) * interval;

    float delta = std::fabs(record_simtime - ref_time + correction);

    return delta <= window;
}

/*
purpose:
  build the source vector of BacktrackRecord* for a target/player.

inputs:
  out      = output vector of BacktrackRecord*
  target   = target/entity/player-like pointer
  window   = time window, normally 0.184375003
  tick_a   = tick pointer used for validity-byte computation
  tick_b   = tick pointer used for final time-window compare

key observations:
  - the function clears the output vector first.
  - it obtains a player index-like value from target via a virtual call.
  - it rejects indices >= 0x41.
  - it indexes history as:
        history = [424B004C] + index * 0x10940
  - it iterates a ring/history buffer.
  - it skips records whose byte at +0xB1 has bits 0x18 set.
  - it calls 41400BF0 with:
        rec->simulation_time
        window
        &rec->valid_byte_A4
        tick_a
        tick_b
  - it pushes only if AL is true AND rec->valid_byte_A4 is true.
  - it has additional stop/continue logic based on rec->record_flags.
*/
void BuildBacktrackRecordList_414A5310(
    PointerVector<BacktrackRecord*>* out,
    void* target,
    float window,
    int32_t* tick_a,
    int32_t* tick_b
) {
    /*
        414A5343..414A5350:
          zero output vector.
    */
    out->begin = nullptr;
    out->end = nullptr;
    out->capacity = nullptr;

    uint8_t* history_base = Read<uint8_t*>(G_HISTORY_BASE_PTR);

    /*
        414A535F..414A536F:
          player_index = target virtual call result.
          if player_index >= 0x41, bail.

        im keeping it pseudocode because the target vtable is not fully documented.
    */
    int32_t player_index = /* target->GetIndexLike() */ 0;

    if (player_index >= 0x41)
        return;

    /*
        per player history stride
    */
    uint8_t* history = history_base + player_index * 0x10940;

    /*
        the exact history struct is not fully documented, but the behavior is:
          - it contains a ring/list of BacktrackRecord*
          - it contains start/count/mask-like fields
          - if no records, bail
    */
    bool history_has_records = true; // pseudocode placeholder
    if (!history_has_records)
        return;

    /*
        pseudocode placeholders for the ring fields observed in the disassembly.
    */
    BacktrackRecord** ring = nullptr;
    int32_t ring_mask = 0;
    int32_t start_index = 0;
    int32_t count = 0;

    int32_t i = start_index;
    int32_t end_index = start_index + count;

    while (i != end_index) {
        BacktrackRecord* rec = ring[i & ring_mask];

        /*
            414A53E6:
              test byte ptr [record+0xB1], 0x18

            since record_flags is at +0xB0, byte +0xB1 is the second byte of
            that flags dword.
        */
        uint8_t flags_B1 = *(reinterpret_cast<uint8_t*>(&rec->record_flags) + 1);
        if (flags_B1 & 0x18) {
            i++;
            continue;
        }

        bool in_window = IsBacktrackRecordInsideWindow_41400BF0(
            rec->simulation_time,
            window,
            &rec->valid_byte_A4,
            tick_a,
            tick_b
        );

        if (in_window && rec->valid_byte_A4) {
            /*
                real code pushes the record pointer into the vector with
                allocator/growth logic.
            */
            // out->push_back(rec);
        }

        /*
            stop/continue logic after each record.

            bit 0x08:
              stop immediately.

            bit 0x800:
              do age-limit test:
                age = current_tick - rec->sim_tick
                half_limit = [4223DC24] / 2
                continue only if age <= half_limit
              otherwise stop.
        */
        int32_t flags = rec->record_flags;

        if (flags & 0x08)
            break;

        if (!(flags & 0x800)) {
            i++;
            continue;
        }

        int32_t current_tick = Read<int32_t>(G_CURRENT_TICK_PTR);
        int32_t age = current_tick - rec->sim_tick;

        int32_t limit = Read<int32_t>(G_RECORD_AGE_LIMIT_PTR);
        int32_t half_limit = limit / 2;

        if (age <= half_limit) {
            i++;
            continue;
        }

        break;
    }
}

/*
purpose:
  this function is the main observed "Extended Backtrack" stage.

inputs observed at callsite 414B44A3:
  [esp+00] = output vector
  [esp+04] = source vector
  [esp+08] = target/player
  [esp+0C] = float window-ish value
  [esp+10] = tick arg A
  [esp+14] = tick arg B
  [esp+18] = byte/flag arg

important:
  this stage happens AFTER the source vector has already been time-filtered by
  414A5310 / 41400BF0.

therefore:
  extended Backtracking does not make invalid records valid.
  it decides whether extra valid historical records are kept for shot scanning.

observed behavior from traces:
  - source vector can contain many records, e.g. 12.
  - final output can be all 12, or can be reduced to sparse sets like 3 or 2.
  - selected records often have rec+0xB4 and rec+0xB8 populated with tick-like
    values after the shot path begins.
  - when gates fail, the output can be cleared/empty.
*/
void FilterOrSelectExtendedBacktrackVector_414AB700(
    PointerVector<BacktrackRecord*>* out,
    const PointerVector<BacktrackRecord*>* source,
    void* target,
    float window,
    int32_t tick_a,
    int32_t tick_b,
    uint8_t arg6
) {
    (void)target;
    (void)window;
    (void)arg6;

    /*
        first compute byte size / count.

        observed:
          byte_size = source->end - source->begin
          count     = byte_size / 4
    */
    int32_t byte_size = int32_t(
        reinterpret_cast<uintptr_t>(source->end) -
        reinterpret_cast<uintptr_t>(source->begin)
    );

    int32_t count = byte_size / 4;

    /*
        if source is tiny, there is no "extended" work to do.

        the observed compare was against 0x0B bytes.
        since these are 4-byte pointers:
          byte_size <= 0x0B means count <= 2.
    */
    if (byte_size <= 0x0B) {
        // copy_vector(out, source);
        return;
    }

    /*
        extended backtrack config read

        414AB7BC:
          cfg = [424F639C]

        414AB7C2:
          compare cfg[0x9AE] with 1

        414AB7C9 / 414AB7CE:
          if cfg[0x9AE] == 1:
              offset = 0x9AD
          else:
              offset = 0x9AC

        414AB7D1:
          compare cfg[offset] with 0

        in the ui trace:
          label "Extended Backtrack"
          value pointer = 0x4BE129B4
          cfg base      = 0x4BE12008
          0x4BE12008 + 0x9AC = 0x4BE129B4
    */
    uint8_t* cfg = Read<uint8_t*>(G_EXTENDED_CFG_BASE_PTR);

    uint32_t extended_offset = (cfg[0x9AE] == 1)
        ? 0x9AD
        : 0x9AC;

    bool extended_enabled = cfg[extended_offset] != 0;

    if (!extended_enabled) {
        /*
            this is the "Extended Backtrack disabled" behavior for large vectors:
            the extra/large list is not allowed through.
        */
        // clear_vector(out);
        return;
    }

    /*
        runtime state gate.

        414AB7DB..414AB7EA:
          state = [424F4B74]
          if state && *state > 1:
              clear/return
    */
    int32_t* state = Read<int32_t*>(G_EXT_FILTER_STATE_PTR);
    if (state && *state > 1) {
        // clear_vector(out);
        return;
    }

    /*
        timing gate.

        observed around 414AB7EA..414AB819:

          a = [ [424B040C] + 0x08 ]
          b = [ [4223CA04] + 0x20 ]   // interval_per_tick
          c = [ [4223CA04] + 0x0C ]

          if (a + 0.001f > b):
              clear/return

          if (b * 0.5f <= c):
              clear/return

        exact semantic names are unknown.
        behavior:
          these checks prevent extended records under certain timing/frame states.
    */
    uint8_t* p_time = Read<uint8_t*>(G_EXT_FILTER_TIME_PTR);
    GlobalsLike* globals = Read<GlobalsLike*>(G_GLOBALS_PTR);

    float a = p_time ? *reinterpret_cast<float*>(p_time + 0x08) : 0.0f;
    float b = globals ? globals->interval_per_tick_20 : 0.015625f;
    float c = globals ? globals->time_or_fraction_0C : 0.0f;

    if (a + Read<float>(G_FLOAT_001) > b) {
        // clear_vector(out);
        return;
    }

    if ((b * Read<float>(G_FLOAT_HALF)) <= c) {
        // clear_vector(out);
        return;
    }

    /*
        selection/copy behavior.

        in simple allowed cases this can copy the source vector.

        in shot traces, when the source vector was large, the final vector often
        became a sparse subset. for example:
          source count 12 -> final count 3
          source count 9  -> final count 2

        the selected records were not random; they tended to include:
          - the newest/current usable record,
          - one or more older records spaced several ticks apart,
          - records with B4/B8 tick markers populated.

        exact loop semantics remain only partially typed, but the functional
        effect is:

          "allow/select extended historical records for shot candidate scanning
           if Extended Backtrack is enabled and runtime gates pass."
    */

    bool should_sparse_select = true; // pseudocode placeholder

    if (!should_sparse_select) {
        // copy_vector(out, source);
        return;
    }

    // clear_vector(out);
    for (int32_t i = 0; i < count; ++i) {
        BacktrackRecord* rec = source->begin[i];

        bool keep =
            (i == 0) ||                                // newest/current candidate
            (rec->linked_or_selected_tick_B4 != -1) || // linked/selected record
            (rec->linked_or_selected_tick_B8 != -1);   // linked/base tick marker

        if (keep) {
            /*
                observed marks:
                  B4 sometimes becomes a future/current tick-like value.
                  B8 sometimes becomes base/source tick-like value.
            */
            rec->linked_or_selected_tick_B4 = tick_b;
            rec->linked_or_selected_tick_B8 = tick_a;

            // out->push_back(rec);
        }
    }
}

/*
this wrapper is called by the shot time path:

  4149BB0A - call 414B4220
  4149BB0F - add esp,18

it returns a vector pointer in EAX.

the vector returned to the caller is stored inside the caller context:

  ESI + 0x10 = begin
  ESI + 0x14 = end
  ESI + 0x18 = capacity

at 4149BB0F trace:
  EAX returned vec = 0x009AD080
  ESI              = 0x009AD070
  therefore:
    returned vec = ESI + 0x10
*/
struct BacktrackVectorBuildContext {
    uint8_t unknown_00[0x10];

    // +0x10
    BacktrackRecord** begin;

    // +0x14
    BacktrackRecord** end;

    // +0x18
    BacktrackRecord** capacity;

    // +0x1C...
};


/*
FIX LOCATION SUMMARY:
  The 0.2-window fix belongs in BuildBacktrackVectorWrapper_414B4220, right
  where base_window is computed before calling 414A5310 and 414AB700.

  Original:
      base_window = max_backtrack_window - interval_per_tick;

  Fixed:
      base_window = max_backtrack_window;

  Byte patch:
      NOP 414B430F subss xmm1,xmm0
      NOP 414B43B3 subss xmm2,[eax+20]
*/

PointerVector<BacktrackRecord*>* BuildBacktrackVectorWrapper_414B4220(
    PointerVector<BacktrackRecord*>* caller_output_vec,
    void* target,
    uint8_t arg_byte,
    int32_t* tick_a,
    int32_t* tick_b
) {
    /*
          1. compute:
               base_window = [4205A9FC] - globals->interval_per_tick

          2. call 414A5310 to build source vector.

          3. call 414AB700 to apply Extended Backtrack vector stage.

          4. move local final vector into caller_output_vec.

          5. return caller_output_vec in EAX.
    */

    GlobalsLike* globals = Read<GlobalsLike*>(G_GLOBALS_PTR);
    float interval = globals ? globals->interval_per_tick_20 : 0.015625f;

    /*
        neverlose does NOT pass the raw max backtrack window directly into
        the source record builder. it first subtracts one tick interval:

            original logic:
                base_window = [4205A9FC] - globals->interval_per_tick

            observed values:
                [4205A9FC]              = 0.200000003
                globals->interval_tick  = 0.015625000

            therefore:
                base_window             = 0.184375003

        that 0.184375003 value is what 414A5310 passes into 41400BF0 when it
        decides whether a BacktrackRecord is allowed into the source vector.

        my fix removes that one tick subtraction so the builder receives the
        full raw max unlag/backtrack window:

            fixed logic:
                base_window = [4205A9FC]

            fixed value:
                base_window = 0.200000003
    */
	
    float base_window = Read<float>(G_MAX_BACKTRACK_WINDOW) - interval;
	// the fix: float base_window = Read<float>(G_MAX_BACKTRACK_WINDOW); 

    PointerVector<BacktrackRecord*> source{};
    PointerVector<BacktrackRecord*> final{};

    BuildBacktrackRecordList_414A5310(
        &source,
        target,
        base_window,
        tick_a,
        tick_b
    );

    FilterOrSelectExtendedBacktrackVector_414AB700(
        &final,
        &source,
        target,
        base_window,
        tick_a ? *tick_a : 0,
        tick_b ? *tick_b : 0,
        arg_byte
    );

    /*
        the real code destructs/frees previous caller_output_vec storage before moving
        final into it.

        around 414B44A8:
          mov esi,[ebp+08]
          ...
          movsd xmm0,[ebp-3C]
          movsd [esi],xmm0
          mov eax,[ebp-34]
          mov [esi+08],eax
          mov eax,esi
          ret
    */
    *caller_output_vec = final;
    return caller_output_vec;
}

/*
after 414B4220 returns, the caller consumes the BacktrackRecord* vector and
builds ShotCandidateItem entries.

important observed code:

  4149BB12 - mov eax,[esi+14]      ; vector end
  4149BB17 - sub ecx,[esi+10]      ; end - begin
  4149BB1A - je 4149BCE5           ; no records -> cleanup/return

  4149BBF7 - mov edi,[esi+10]      ; iterator = begin
  4149BBFA - mov eax,[esi+14]      ; end
  4149BC3F - cmp edi,eax
  4149BC41 - je 4149BCE2

  4149BC7B - mov [esi+0C],edi      ; current iterator pointer

  4149BC92 - push [ebp+08]
  4149BC95 - call 4148B3B0         ; construct/init output item if capacity available

  4149BCA0 - lea ecx,[eax+170]
  4149BCA6 - mov [edx+04],ecx      ; advance output item vector end by 0x170

  4149BC2C - push [edx]            ; *iterator = BacktrackRecord*
  4149BC2E - call 4149C000         ; build one ShotCandidateItem
*/
void ConsumeBacktrackVector_4149BB0F(
    BacktrackVectorBuildContext* ctx,
    PointerVector<ShotCandidateItem>* output_items,
    void* target,
    int32_t* tick_a,
    int32_t* tick_b
) {
    BacktrackRecord** it = ctx->begin;
    BacktrackRecord** end = ctx->end;

    if (it == end) {
        return;
    }

    while (it != end) {
        BacktrackRecord* rec = *it;

        /*
            real code grows or constructs a 0x170-byte ShotCandidateItem in
            output_items. im expressing it as push/placement construction.
        */
        ShotCandidateItem* item = nullptr; // output_items->emplace_back()

        BuildShotCandidateFromBacktrackRecord_4149C000(
            item,
            rec,
            reinterpret_cast<Vector3*>(/* eye/shoot position arg */ nullptr),
            tick_a,
            tick_b
        );

        ++it;
    }
}

/*
purpose:
  convert one BacktrackRecord* into one 0x170-byte ShotCandidateItem.

calling convention observed at 4149BC2E:
  ECX       = item
  [esp+00] = BacktrackRecord* rec
  [esp+04] = Vector3* eye_or_shoot_position
  [esp+08] = tick_a/current-tick-like value or pointer context
  [esp+0C] = tick_b/final-tick-like value or pointer context

confirmed by compact trace:
  item+04 = rec
  item+08/+0C/+10 = center
  item+D0 = distance
  item+D4 = angle/FOV-like value
  item+D8 = type

for a normal player bbox:
  mins = {-16, -16, 0}
  maxs = { 16,  16, 72}

so:
  center.x = origin.x
  center.y = origin.y
  center.z = origin.z + 36

example:
  origin.z = -167.968750
  center.z = -131.968750
*/
void BuildShotCandidateFromBacktrackRecord_4149C000(
    ShotCandidateItem* item,
    BacktrackRecord* rec,
    const Vector3* eye_or_shoot_position,
    int32_t* tick_a,
    int32_t* tick_b
) {
    /*
        4149C007:
          esi = ecx
        we name ESI as item.

        4149C009:
          ebx = [esp+40]
        because of pushes/sub esp, EBX becomes the record pointer.
    */

    /*
        4149C018:
          mov [ecx+04],ebx

        store the source BacktrackRecord pointer into the shot candidate.
    */
    item->record = rec;

    /*
        4149C01B..4149C033:
          eax = 0x81
          eax &= rec->record_flags
          ecx = (eax == 0x80)
          ecx += 3
          item->candidate_type = ecx

        observed:
          record_flags = 0x20 -> type = 3
    */
    int type_extra = (((rec->record_flags & 0x81) == 0x80) ? 1 : 0);
    item->candidate_type = type_extra + 3;

    /*
        4149C039..4149C073:
          center = origin + (mins + maxs) * 0.5
    */
    item->center.x = rec->origin.x + (rec->mins.x + rec->maxs.x) * 0.5f;
    item->center.y = rec->origin.y + (rec->mins.y + rec->maxs.y) * 0.5f;
    item->center.z = rec->origin.z + (rec->mins.z + rec->maxs.z) * 0.5f;

    /*
        4149C078..4149C0A7:
          delta = center - eye/shoot position
          distance = sqrt(delta dot delta)
          item+0xD0 = distance

        compact trace showed the center stays constant for the same record while
        distance and angle/FOV change slightly between ticks because the eye
        position changes.
    */
    Vector3 delta = item->center - *eye_or_shoot_position;
    item->distance = Length(delta);

    /*
        4149C0AF..4149C0CE:
          if distance > 1.0:
              delta /= distance

        this normalizes direction from eye to candidate center.
    */
    Vector3 direction = delta;

    if (item->distance > 1.0f) {
        float inv_dist = 1.0f / item->distance;
        direction = direction * inv_dist;
    }

    /*
        4149C0E9..4149C109:
          41575170(direction, temp)
          41573E80(424AFD14, temp)
          fstp [item+0xD4]

        this is very likely:
          - convert direction vector to angles, then
          - compare to current view/aim angle,
          - output angular distance / FOV-like score.

        observed values around:
          13.8 - 16.8 degrees-ish in traces.
    */
    item->fov_or_angle = ComputeFovOrAngleLikeValueFromDirection(
        direction.x,
        direction.y,
        direction.z
    );

    /*
        status tail.

        there are two paths:

          fast/simple path:
            if 413F6DB0([424B004C]) returns true:
              item+14 = 1
              item+15 = 1
              item+16 = 0
              skip slow branch

          slow/status path:
            if 413F6DB0 returns false:
              compute timing_adjust
              window = 0.2 - timing_adjust
              call 41400BF0(rec->simulation_time, window, &item+15, tick_a, tick_b)
              item+14 = AL
              item+16 = (rec->record_flags >> 4) & 1

        in the mystery branch trace, 413F6DB0 naturally returned AL=0, so the
        slow branch was entered without needing to fake the branch.
    */

    if (SomeGlobalFastPath_413F6DB0()) {
        /*
            4149C11E:
              mov word ptr [esi+14],0101

            because little endian:
              byte +14 = 1
              byte +15 = 1

            4149C124:
              mov byte ptr [esi+16],00
        */
        item->in_time_window_14 = 1;
        item->valid_after_check_15 = 1;
        item->record_flag_0x10_16 = 0;
    } else {
        /*
            4149C12D..4149C375:
              compute timing_adjust.

            4149C375:
              window = [4205A9FC] - timing_adjust

            4149C381:
              record_simtime = rec->simulation_time

            4149C3AD:
              call 41400BF0

            4149C3B2:
              item+14 = AL

            4149C3C1:
              item+16 = (rec->record_flags >> 4) & 1
        */
        float timing_adjust = ComputeCandidateTimingAdjust_4149C12D();

        float window = Read<float>(G_MAX_BACKTRACK_WINDOW) - timing_adjust;

        bool in_window = IsBacktrackRecordInsideWindow_41400BF0(
            rec->simulation_time,
            window,
            &item->valid_after_check_15,
            tick_a,
            tick_b
        );

        item->in_time_window_14 = in_window ? 1 : 0;

        item->record_flag_0x10_16 =
            (uint8_t(rec->record_flags) >> 4) & 1;
    }

    /*
        4149C3C4..4149C3EB:
          push [item+4C]
          call 41343B20
          if helper returns object:
              rec2 = item->record
              if rec2->byte_A5 != 0:
                  if helper_object->field_2C == helper_object->field_4C:
                      item+17 = 1

        exact name is unknown.
        it appears to be a special link/state bit, probably related to record
        animation/sequence/resolution state.
    */
    bool special_link_condition = false; // pseudocode placeholder
    if (special_link_condition) {
        item->special_link_17 = 1;
    }
}

/*
observed branch trace:
  byte425009DC = 0
  globals+0x0C = about 0.019136 / 0.020074
  interval+0x20 = 0.015625
  42504F3C = 16
  42504F1C = 1
  4223E328 = 16
  42241A3C = 9
  [4223F428]+0x4D3C = 320574 / 320578
  425007F8 = -1
  4223FE28 = 2147483647

the branch reached 4149C375 and then called 41400BF0.

the exact purpose of the array scan over 42504EDC/42504EEC is still
not fully known. it looks like a timing/choke/command-window consistency check.
*/
float ComputeCandidateTimingAdjust_4149C12D() {
    GlobalsLike* globals = Read<GlobalsLike*>(G_GLOBALS_PTR);

    float interval = globals
        ? globals->interval_per_tick_20
        : 0.015625f;

    /*
        4149C136:
          cmp byte ptr [425009DC],00
          je 4149C14B

        If byte != 0:
          use interval directly.
    */
    if (Read<uint8_t>(G_BYTE_425009DC) != 0) {
        return interval;
    }

    /*
        4149C14B:
          call 414F21D0

        If helper returns true:
          adjust = interval * 2

          then:
            if [42504F3C] < 2:
                early return / use that adjust
            else:
                continue deeper

        If helper returns false:
          adjust = interval
          continue.
    */
    bool helper = Helper_414F21D0();

    float adjust = interval;

    if (helper) {
        adjust = interval + interval;

        if (Read<int32_t>(G_MODE_COUNT_42504F3C) < 2) {
            return adjust;
        }
    }

    /*
        4149C178:
          eax = [4223E328]
          eax -= [42241A3C]
          if eax < 3:
              return adjust

        Then:
          eax = [42504F3C]
          if eax < 2:
              return adjust
    */
    int32_t delta_ticks =
        Read<int32_t>(G_TICK_A_4223E328) -
        Read<int32_t>(G_TICK_B_42241A3C);

    if (delta_ticks < 3)
        return adjust;

    int32_t mode_count = Read<int32_t>(G_MODE_COUNT_42504F3C);

    if (mode_count < 2)
        return adjust;

    /*
        4149C19D:
          xmm1 = [42077FC8]
          xmm1 /= adjust
          xmm1 += 0.5
          ecx = int(xmm1)

          if [42504F1C] > ecx:
              return adjust
    */
    float constant = Read<float>(G_FLOAT_42077FC8);
    int32_t needed = int32_t((constant / adjust) + 0.5f);

    if (Read<int32_t>(G_COUNT_42504F1C) > needed)
        return adjust;

    /*
        4149C1C1..4149C1DF:
          cur = [[4223F428] + 0x4D3C]

          if [425007F8] > cur:
              return adjust

          if [4223FE28] > cur:
              return adjust
    */
    uint8_t* p = Read<uint8_t*>(G_PTR_4223F428);
    int32_t cur = p ? *reinterpret_cast<int32_t*>(p + 0x4D3C) : 0;

    if (Read<int32_t>(G_ORDER_A_425007F8) > cur)
        return adjust;

    if (Read<int32_t>(G_ORDER_B_4223FE28) > cur)
        return adjust;

    /*
        4149C1E5..4149C372:
          if mode_count < 8:
              scalar path
          else:
              SSE vectorized min/max path

        it scans integer arrays:
          42504EDC
          42504EEC

        it computes min/max-like values across the arrays.

        at 4149C366:
          cmp edx,edi
          jne 4149C375
          movaps xmm2,xmm0

        meaning:
          if the derived min/max values are equal,
          use one timing value; otherwise keep the existing adjust.

        the exact field names remain unknown, but it behaves like a command
        buffer/choke window consistency check.
    */
    bool array_values_collapse_to_same_value = false; // placeholder

    if (array_values_collapse_to_same_value) {
        adjust = interval;
    }

    return adjust;
}

/*
conclusion: a record reaches shot scanning only if it survives multiple layers:

  layer 1: history/build layer
    414A5310 reads records from the player history ring.

  layer 2: time validity layer
    41400BF0 checks:
      abs(record_simtime - ref_time + correction) <= window

  layer 3: extended backtrack vector layer
    414AB700 decides whether the large/extra list is allowed, copied, cleared,
    or reduced to a sparse subset.

  layer 4: shot candidate layer
    4149BB0F loops final BacktrackRecord* entries.

  layer 5: candidate item layer
    4149C000 creates ShotCandidateItem entries:
      center, distance, FOV/angle, status bytes.

in simple terms:
  41400BF0 answers:
    "is this record temporally legal/valid for the requested tick window?"

  414AB700 answers:
    "should the shot path consider the larger/extended historical record set?"

  4149C000 answers:
    "convert this selected record into a scored shot candidate item."