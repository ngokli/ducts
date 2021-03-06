// ducts.c
// version 2.1
// Fri Sep 23 00:04:54 PDT 2016
// https://github.com/ngokli/ducts
//
// Neal Gokli
//
// A Solution to the Quora Datacenter Cooling problem
//
// Problem statement: http://www.businessinsider.com/heres-the-test-you-have-to-pass-to-work-at-quora-silicon-valleys-hot-new-86-million-startup-2010-4
//
// See https://github.com/ngokli/ducts for latest version (and other versions)
//
//
// Version History
// 1.0 Initial release
// 1.1 Added this block of comments
// 2.0 Using a 64-bit uint64_t instead of an array to store the room structure.
//       This means width*length is limited to 64.
// 2.1 Performance bug fix: No longer continuing to search after reaching the
//       end room.
// 3.0 Replace position struct with a uint64_t bitmask into the room structure.
//       Exactly one bit is set in a position bitmask at any time.
//       Finally, something faster than version 1! But only about 25% faster.
// 3.1 Simplified search2() with a (kind of complicated) macro.
//       Performance unchanged.
// 4.0 Added periodic checking for empty rooms that are cut off, using a
//       recursive flood-fill algorithm. The flood-fill is *much* less expensive
//       than the path search, so avoiding some large dead-end branches gives a
//       huge performance gain.
// 4.1 Added forward declarations, moved functions and global variable
//       definitions, and fixed up comments/formatting.  Sorry for the messy
//       diff, but rest assured there are no material changes.
// 4.2 Added scanf_int_safe(), a scanf wrapper to handle error checking and
//       reporting.
// 4.3 Added macros to replace hardcoded values.
// 4.4 Replaced call_with_param() macro with call_with_params() macro (takes
//       variable number of parameters).  Just to play with __VA_ARGS__ in
//       macros ^_^.  Maybe I will use the added functionality later.
// 4.5 Added some methods to handle input, setup, and debug output.
// 4.6 Fixed another performance bug. try_flood() no longer returns success
//       when exactly one room is cut off. This does not affect the results, but
//       fixing it gives a huge performance boost.
// 4.7 Fixed error message formatting.
// 4.8 Bugfix: Now works for 8x8 datacenters
// 4.9 Used #ifdef WITH_STATS_AND_PROGRESS to exclude that code unless it is
//       explicitly desired.  20% to 25% percent performance gain.
// 5.0 Bugfix: Now correctly handles 64-room inputs. Also fixed scanf error
//       checking, and added a check for input with more than 64 rooms.

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include <errno.h>
#include <math.h>




// *******  Constant Macros  *******

// Input values
#define EMPTY_ROOM_VAL   0
#define EXCLUDE_ROOM_VAL 1
#define START_ROOM_VAL   2
#define END_ROOM_VAL     3

// Threshold for printing progress dots in verbose mode
#define SEARCHES_PER_DOT (1 << 30)




// *******  Global Variables  *******

// *** Output Verbosity ***
bool quiet   = false; // From command line -q
bool verbose = false; // From command line -v



// ** room setup **
int       width;
int       length;
int       num_rooms = 0;
uint64_t  start_room;
uint64_t  end_room;



// *** Bitmasks ***
// For manipulating rooms and position

// "largest" room position
uint64_t max_pos;

// Bits along the corresponding edges are set in these variables
uint64_t left_edge;
uint64_t right_edge;
uint64_t up_edge;
uint64_t down_edge;



// *** Flood-Fill Check ***
// Global variables to avoid passing
int      flood_rooms_left;
int      flood_rooms_left_threshold;
uint64_t flood_rooms;



// *** Stats ***

#ifdef WITH_STATS_AND_PROGRESS

  // Number of calls to search() and flood_fill()
  uint64_t search_count     = 0;
  uint64_t flood_fill_count = 0;

  // Number of times flood-filling check allows the search to stop early (or not)
  uint64_t flood_early_stop_count    = 0;
  uint64_t flood_no_early_stop_count = 0;

#endif



// *******  Macros  *******


// Allows a macro to call a function after piecing together the function name
#define call_with_params(name, ...) name(__VA_ARGS__)


// Move pos in the corresponding direction
#define pos_left(pos)  ((pos) >> 1      )
#define pos_right(pos) ((pos) << 1      )
#define pos_up(pos)    ((pos) >> (width))
#define pos_down(pos)  ((pos) << (width))


// call search() in a particular direction from the passed pos.
//   Adds the return value of search() to solution_count.
//
//   This macro is meant to simplify the code in search2().
//   An alternative is to have a helper function that takes a direction
//   "index" between 0 and 3.  The direction macros (e.g. pos_left()) would be
//   accessed through an array of function pointers.  The edge bitmasks would
//   also be accessed through an array.  This may complicate things for
//   compiler optimization.  Or not?  I should try it out.
//
//   This is also meant for me to get to play around with complicated macros
#define search_in_direction(pos, direction, rooms, rooms_left, solution_count) \
  do {                                                                         \
    if (0 == (direction ## _edge & pos)) {                                     \
      solution_count += search(call_with_params(pos_ ## direction, pos),       \
                               rooms, rooms_left);                             \
    }                                                                          \
  } while (0)


// Like search_in_direction, but for flood_fill()
#define flood_fill_in_direction(pos, direction, flood_rooms, flood_rooms_left) \
  do {                                                                         \
    uint64_t next_pos = call_with_params(pos_ ## direction, pos);              \
    if ( (0 == (direction ## _edge & pos) ) &&                                 \
         (room_free(next_pos, flood_rooms))   ) {                              \
      flood_fill(next_pos);                                                    \
      if (0 == flood_rooms_left) {                                             \
        return;                                                                \
      }                                                                        \
    }                                                                          \
  } while (0)



// *******  Forward Declarations  *******


// *** I/O Methods ***

// Prints usage info
void print_usage(char *basename);

// Read and handle command-line arguments
void handle_cli_args(int argc, char* argv[]);

// Read and handle datacenter rooms map input
uint64_t handle_datacenter_input();

// Prints unless quiet is set
void print_normal(char* fmt, ...);

// Prints only if verbose is set
void print_verbose(char* fmt, ...);

// Prints rooms in a friendly way
void print_rooms(uint64_t rooms);

// Prints rooms and other helpful debug info
void print_rooms_setup(uint64_t rooms);

// Wraps scanf("%d", &var) in some error checking and reporting code.
//   error_string is used for context in error messages.
//   Returns the int value read from stdin
int scanf_int_safe(char* error_string);


// *** Recursive Flood-Fill Check ***
// Checks whether all empty rooms can be reached from the current state

// Sets flood_rooms_left_threshold.  Flood-fill checks are only performed when
//   rooms_left is below this value.
//   Uses global variables to determine the value called.
void set_flood_fill_threshold();

// Decides if a flood-fill check should be done
bool should_flood_fill(int rooms_left);

// Recursive method that fills every empty room (in flood_rooms) that can be
//   reached from pos. Use try_flood() to initiate this flood-fill check.
//   Uses flood_rooms and flood_rooms_left global variables.
void flood_fill(uint64_t pos);

// Do a flood-fill check.
//   Checks if all empty rooms can be reached from the current state.
//   Returns true iff it is possible.
//   If try_flood returns false, the current search branch should be abandoned.
bool try_flood(uint64_t pos, uint64_t rooms, int rooms_left);



// *** Recursive Path Search ***
// The core functionality

// search() checks if the current state is a solution or a possible dead end.
//   Otherwise, it calls search2() to continue the search.
//   Returns the number of solutions (paths) found.
//   Increments global variable search_count each time search() is called.
int search(uint64_t pos, uint64_t rooms, int rooms_left);

// search2() simply calls search() in the four directions, avoiding wrapping
//   over edges.
int search2(uint64_t pos, uint64_t rooms, int rooms_left);



// *** Other helper methods ***

// Checks if room at pos is empty
//   Returns true if empty
bool room_free(uint64_t pos, uint64_t rooms);

// Set edges (left_edge, etc.) based on height and width
void set_edges();




// *******  I/O methods  *******

void print_usage(char *basename) {
  fprintf(stderr, "Usage: %s [-q]\n", basename);
}

void handle_cli_args(int argc, char* argv[]) {
  // Can take -q or -v as a command line argument
  if (argc > 2) {
    print_usage(argv[0]);
    exit(1);
  }
  if (argc == 2) {
    if (strcmp(argv[1], "-q") == 0) {
      quiet = true;
    }
    else if(strcmp(argv[1], "-v") == 0) {
      verbose = true;
    }
    else {
      print_usage(argv[0]);
      exit(2);
    }
  }
}

uint64_t handle_datacenter_input() {
  // Get the width and length first
  width  = scanf_int_safe("width");
  length = scanf_int_safe("length");

  if (64 < width * length) {
    fputs("Error: width*length greater than 64.\n", stderr);
    exit(3);
  }

  max_pos = (uint64_t)1 << (width * length - 1);

  set_edges();

  // Get the actual room structure
  uint64_t rooms = 0;
  uint64_t pos   = 1;
  do {
    int val;
    char input_info[32];
    snprintf(input_info, sizeof(input_info), "rooms[pos=%lu]", pos);
    val = scanf_int_safe(input_info);
    if (val == EMPTY_ROOM_VAL) {
      num_rooms++;
    }
    else if (val == EXCLUDE_ROOM_VAL) {
      rooms |= pos;
    }
    else if (val == START_ROOM_VAL) {
      rooms |= pos;
      start_room = pos;
    }
    else if (val == END_ROOM_VAL) {
      end_room = pos;
    }
    pos <<= 1;
  } while ((pos <= max_pos) && (pos != 0));

  set_flood_fill_threshold();

  return rooms;
}

void print_normal(char* fmt, ...) {
  if (!quiet) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
  }
}

void print_verbose(char* fmt, ...) {
  if (verbose) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
  }
}

void print_rooms(uint64_t rooms) {
  uint64_t pos = 1;
  do {
    print_verbose("%d ", !room_free(pos, rooms));

    if (0 != (pos & right_edge)) {
      print_verbose("\n");
    }
    pos <<= 1;
  } while ((pos <= max_pos) && (pos != 0));
}

void print_rooms_setup(uint64_t rooms) {
  // Print helpful info
  print_verbose("width: %hd, length: %hd\n", width, length);
  print_verbose("start_room: %lu\n", start_room);
  print_verbose("end_room: %lu\n", end_room);
  print_rooms(rooms);
  print_verbose("num_rooms: %d\n", num_rooms);
  print_verbose("flood_rooms_left_threshold: %d\n", flood_rooms_left_threshold);
}

int scanf_int_safe(char* error_string) {
  errno = 0;
  int val;
  int scanf_count;
  scanf_count = scanf("%d", &val);
  if (scanf_count == 1 ) {
    return val;
  }

  if (scanf_count == EOF) {
    fprintf(stderr, "Mismatch when reading %s.\n", error_string);
    exit(4);
  }

  fprintf(stderr, "Error reading %s: %s\n", error_string, strerror(errno));
  exit(errno);
}



// *******  Recursive Flood-Fill  *******

void set_flood_fill_threshold() {
  // First attempt at a heuristic for a flood threshold
  // Flood-filling at the very begining of a search makes no sense, because it
  // is unlikely for the first few ducts to create a "bubble".
  // flood_rooms_left_threshold is used by should_flood_fill() to decide when
  // to start flood-filling.
  flood_rooms_left_threshold = num_rooms -
                                 (1.5 * sqrt((double)num_rooms) + 1);

  // I need to play around with threshold formulas a bit more, and try them
  // with more varied and complex datacenters.

  // Here's another attempt which seems to change the runtimes slightly (both
  // up and down: need to test more).  Since the runtimes don't change much,
  // I think these are close to optimal for test cases being used.
  // int min_dim = (width > length) ? length : width;
  // flood_rooms_left_threshold = num_rooms - min_dim - 2;
}

bool should_flood_fill(int rooms_left) {
  return ( ( 0 == (rooms_left % 4) ) &&
           ( flood_rooms_left_threshold > rooms_left) );
}

void flood_fill(uint64_t pos) {
  #ifdef WITH_STATS_AND_PROGRESS
    flood_fill_count++;
  #endif

  flood_rooms_left--;
  if (flood_rooms_left == 0) {
    return;
  }

  flood_rooms |= pos;

  flood_fill_in_direction(pos, left,  flood_rooms, flood_rooms_left);
  flood_fill_in_direction(pos, right, flood_rooms, flood_rooms_left);
  flood_fill_in_direction(pos, up,    flood_rooms, flood_rooms_left);
  flood_fill_in_direction(pos, down,  flood_rooms, flood_rooms_left);
}

bool try_flood(uint64_t pos, uint64_t rooms, int rooms_left) {
  flood_rooms      = rooms;
  // flood_rooms_left works a bit differently from rooms_left, because the
  // count has to be decremented up front.  Could be re-architected, but it
  // would be too much work for this toy project.
  flood_rooms_left = rooms_left + 1;

  flood_fill(pos);
  return (0 == flood_rooms_left);
}



// *******  Recursive Path Search  *******

int search(uint64_t pos, uint64_t rooms, int rooms_left) {
  #ifdef WITH_STATS_AND_PROGRESS
    // Print some dots so we can monitor the speed
    search_count++;
    if (0 == (search_count % SEARCHES_PER_DOT)) {
      print_verbose(".");
      fflush(NULL);
    }
  #endif

  if (room_free(pos, rooms)) {
    if (0 < rooms_left) {
      // This room is empty, so it could be the end room
      if (pos == end_room) {
        // This is the end room, but there are still rooms left... no good!
        return 0;
      }

      if (should_flood_fill(rooms_left)) {
        // Try flood-filling (check if the end room is reachable from pos)
        if (!try_flood(pos, rooms, rooms_left)) {
          #ifdef WITH_STATS_AND_PROGRESS
            flood_early_stop_count++;
          #endif
          return 0;
        }
        #ifdef WITH_STATS_AND_PROGRESS
          else {
            flood_no_early_stop_count++;
          }
        #endif
      }

      // This room is empty and there are rooms left, so continue the search!
      return search2(pos, (rooms | pos), (rooms_left - 1));
    }

    // This room is empty, so it could be the end room
    if (pos == end_room) {
      // No rooms left and this is the end room, so we found a solution!
      return 1;
    }

    // No rooms left and this is not the end room.
    // We should never reach here?
    return 0;
  }

  // This is not a free room, so this is not the way!
  return 0;
}

int search2(uint64_t pos, uint64_t rooms, int rooms_left) {
    int solution_count = 0;

    search_in_direction(pos, left,  rooms, rooms_left, solution_count);
    search_in_direction(pos, right, rooms, rooms_left, solution_count);
    search_in_direction(pos, up,    rooms, rooms_left, solution_count);
    search_in_direction(pos, down,  rooms, rooms_left, solution_count);

    return solution_count;
}



// *******  Other Helper Methods  *******

bool room_free(uint64_t pos, uint64_t rooms) {
  return (0 == (pos & rooms));
}


void set_edges() {
  // Each of these edges has the corresponding bits set for its edge.

  // Set left_edge and right_edge
  left_edge  = 0;
  right_edge = 0;
  uint64_t right_edge_start = (uint64_t)1 << (width - 1);
  for (int i = 0; i < length; i++) {
    left_edge  = (left_edge  << width) + 1;
    right_edge = (right_edge << width) + right_edge_start;
  }

  // Set up_edge and down_edge
  up_edge   = 0;
  down_edge = 0;
  uint64_t down_edge_start = max_pos >> (width - 1);
  for (int i = 0; i < width; i++) {
    up_edge   = (up_edge << 1) + 1;
    down_edge = (down_edge << 1) + down_edge_start;
  }
}




// *******  Main()  *******
void main(int argc, char *argv[]) {
  handle_cli_args(argc, argv);

  uint64_t rooms;
  rooms = handle_datacenter_input();

  print_rooms_setup(rooms);

  // Search!
  int solution_count = search2(start_room, rooms, num_rooms);
  print_verbose("\nsolutions: ");
  print_normal("%d\n", solution_count);

  #ifdef WITH_STATS_AND_PROGRESS
    print_verbose("search_count: %lu\n", search_count);
    print_verbose("flood_fill_count: %lu\n", flood_fill_count);
    print_verbose("flood_early_stop_count: %lu, flood_no_early_stop_count: %lu\n",
                  flood_early_stop_count, flood_no_early_stop_count);
  #endif
}
