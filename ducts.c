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


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include <errno.h>
#include <math.h>


// Number of calls to search()
uint64_t search_count = 0;

// Number of times flood filling allows the search to stop early (or not)
uint64_t flood_early_stop_count    = 0;
uint64_t flood_no_early_stop_count = 0;


// room setup
short int width;
short int length;
uint64_t  end_room;

// "largest" room position
uint64_t max_pos;



// Bits along the corresponding edges are set in these variables
uint64_t left_edge;
uint64_t right_edge;
uint64_t up_edge;
uint64_t down_edge;


// For Flood filling
// Global variables to avoid passing
int      flood_rooms_left;
int      flood_rooms_left_threshold;
uint64_t flood_rooms;


// allows a macro to call a function after piecing together the function name
#define call_with_param(name, param) name(param)


// These macros move pos in the corresponding direction.
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
//   This macro is also meant for me to play around with complicated macros
#define search_in_direction(pos, direction, rooms, rooms_left, solution_count) \
  do {                                                                         \
    if (0 == (direction ## _edge & pos)) {                                     \
      solution_count += search(call_with_param(pos_ ## direction, pos),        \
                               rooms, rooms_left);                             \
    }                                                                          \
  } while (0)


// Like search_in_direction, but for flood filling
#define flood_fill_in_direction(pos, direction, flood_rooms, flood_rooms_left) \
  do {                                                                         \
    uint64_t next_pos = call_with_param(pos_ ## direction, pos);               \
    if ( (0 == (direction ## _edge & pos) ) &&                                 \
         (room_free(next_pos, flood_rooms))   ) {                              \
      flood_fill(next_pos);                                                    \
      if (0 == flood_rooms_left) {                                             \
        return;                                                                \
      }                                                                        \
    }                                                                          \
  } while (0)




// Print usage info
void print_usage(char *basename) {
  fprintf(stderr, "Usage: %s [-q]\n", basename);
}

// Control verbosity from command line
bool quiet   = false; // From command line -q
bool verbose = false; // From command line -v

// Prints unless quiet is set
void print_normal(char* fmt, ...) {
  if (!quiet) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
  }
}

// Prints only if verbose is set
void print_verbose(char* fmt, ...) {
  if (verbose) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
  }
}




// check if room at pos is empty
bool room_free(uint64_t pos, uint64_t rooms) {
  return (0 == (pos & rooms));

}

// display rooms in a friendly way
void print_rooms(uint64_t rooms) {
  uint64_t pos = 1;
  while (pos <= max_pos) {
    print_verbose("%d ", !room_free(pos, rooms));

    if (0 != (pos & right_edge)) {
      print_verbose("\n");
    }
    pos <<= 1;
  }
}



// Define if a flood fill should be done
bool should_flood_fill(int rooms_left) {
  return ( ( 0 == (rooms_left % 4) ) &&
           ( flood_rooms_left_threshold > rooms_left) );
}


// Fill every empty room (in flood_rooms) that can be reached from pos.
//   Uses flood_rooms and flood_rooms_left global variables.
void flood_fill(uint64_t pos) {
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

// Check if all empty rooms can be reached from the current state.
// Returns true iff it is possible.
// If try_flood returns false, the current search branch should be abandoned.
bool try_flood(uint64_t pos, uint64_t rooms, int rooms_left) {
  flood_rooms      = rooms;
  flood_rooms_left = rooms_left;
  flood_fill(pos);
  return (0 == flood_rooms_left);
}



// search() and search2() recursive methods to do the path search
// search() checks if the current state is a solution or a possible dead end.
//   Otherwise, it calls search2() to continue the search.
//   Returns the number of solutions (paths) found.
//   Increments global variables search_count each time search() is called.
// search2() simply calls search() in the four directions, avoiding wrapping
//   over edges.

int search(uint64_t pos, uint64_t rooms, int rooms_left);

int search2(uint64_t pos, uint64_t rooms, int rooms_left) {
    int solution_count = 0;

    search_in_direction(pos, left,  rooms, rooms_left, solution_count);
    search_in_direction(pos, right, rooms, rooms_left, solution_count);
    search_in_direction(pos, up,    rooms, rooms_left, solution_count);
    search_in_direction(pos, down,  rooms, rooms_left, solution_count);

    return solution_count;
}

int search(uint64_t pos, uint64_t rooms, int rooms_left) {
  // Print some dots so we can monitor the speed
  const static uint64_t search_count_max = 1 << 30;
  search_count++;
  if (0 == (search_count % search_count_max)) {
    print_verbose(".");
    fflush(NULL);
  }

  if (room_free(pos, rooms)) {
    if (0 < rooms_left) {
      // This room is empty, so it could be the end room
      if (pos == end_room) {
        // This is the end room, but there are still rooms left... no good!
        return 0;
      }


      if (should_flood_fill(rooms_left)) {
        // Try flood filling (check if the end room is reachable from pos)
        if (!try_flood(pos, rooms, rooms_left)) {
          flood_early_stop_count++;
          return 0;
        }
        else {
          flood_no_early_stop_count++;
        }
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




int main(int argc, char *argv[]) {
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
      exit(1);
    }
  }


  // Read rooms from stdin

  // Get the width and length first
  errno = 0;
  if (scanf("%hd", &width) < 0 ) {
    fprintf(stderr, "Mismatch when reading width.\n");
    exit(1);
  }
  else if (errno != 0) {
    perror("Error reading width");
    exit(errno);
  }

  if (scanf("%hd", &length) < 0) {
    fprintf(stderr, "Mismatch when reading length.\n");
    exit(1);
  }
  else if (errno != 0) {
    perror("Error reading length");
    exit(errno);
  }

  max_pos = (uint64_t)1 << (width * length - 1);


  // Set left_edge, right_edge, up_edge, and down_edge.
  //   Each of these has the corresponding bits set for its edge.
  left_edge  = 0;
  right_edge = 0;
  for (int i = 0; i < length; i++) {
    left_edge  = (left_edge  << width) + 1;
    right_edge = (right_edge << width) + (1 << (width - 1));
  }

  up_edge   = 0;
  down_edge = 0;
  uint64_t down_edge_start = max_pos >> (width - 1);
  for (int i = 0; i < width; i++) {
    up_edge   = (up_edge << 1) + 1;
    down_edge = (down_edge << 1) + down_edge_start;
  }


  // Get the actual room structure
  uint64_t rooms     = 0;
  int      num_rooms = 0;
  uint64_t start_room;

  uint64_t pos = 1;
  while (pos <= max_pos) {
    int val;
    if (scanf("%d", &val) < 0) {
      fprintf(stderr, "Input error: rooms[pos=%lu]\n", pos);
      exit(1);
    }
    else if (errno != 0) {
      perror("Error reading rooms");
      exit(errno);
    }

    if (val == 0) {
      num_rooms++;
    }
    else if (val == 1) {
      rooms |= pos;
    }
    else if (val == 2) {
      rooms |= pos;
      start_room = pos;
    }
    else if (val == 3) {
      end_room = pos;
    }
    pos <<= 1;
  }


  // First attempt at a heuristic for a flood threshold
  // Flooding at the very begining of a search makes no sense, because it
  // is unlikely for the first few ducts to create a "bubble".
  flood_rooms_left_threshold = num_rooms -
                                 (1.5 * sqrt((double)num_rooms) + 1);


  // Print helpful info
  print_verbose("width: %hd, length: %hd\n", width, length);
  print_verbose("start_room: %lu\n", start_room);
  print_verbose("end_room: %lu\n", end_room);
  print_rooms(rooms);
  print_verbose("rooms: %lu\n", rooms);

  // Search!
  int solution_count = search2(start_room, rooms, num_rooms);
  print_verbose("\nsolutions: ");
  print_normal("%d\n", solution_count);

  print_verbose("search_count: %lu\n", search_count);
  print_verbose("flood_early_stop_count: %lu, flood_no_early_stop_count: %lu\n",
                flood_early_stop_count, flood_no_early_stop_count);
}
