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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include <errno.h>

// Number of calls to search()
uint64_t search_count = 0;

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


// macros to move the position
#define pos_left(pos)  ((pos) >> 1    )
#define pos_right(pos) ((pos) << 1    )
#define pos_up(pos)    ((pos) >> width)
#define pos_down(pos)  ((pos) << width)

#define search_in_direction(pos, direction, rooms, rooms_left, solution_count) \
  if (0 == (direction ## _edge & (pos))) {                                     \
    solution_count += search(pos_ ## direction ## (pos), rooms, rooms_left);   \
  }



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

// display room_array in a friendly way
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


// search() and search2(): recursive methods to do the path search
// search() checks for solutions or a dead end, and calls search2() to
//     continue the search.
// search2() simply calls search() in the four directions.

int search(uint64_t pos, uint64_t rooms, int rooms_left);

int search2(uint64_t pos, uint64_t rooms, int rooms_left) {
    int solution_count = 0;

    if (0 == (left_edge & pos)) {
      solution_count += search(pos_left(pos), rooms, rooms_left);
    }

    if (0 == (right_edge & pos)) {
      solution_count += search(pos_right(pos), rooms, rooms_left);
    }

    if (0 == (up_edge & pos)) {
      solution_count += search(pos_up(pos), rooms, rooms_left);
    }

    if (0 == (down_edge & pos)) {
      solution_count += search(pos_down(pos), rooms, rooms_left);
    }

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
      printf("a");
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
}
