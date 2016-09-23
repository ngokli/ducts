// ducts.c
// version 2.0
// Thu Sep 22 19:54:58 PDT 2016
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
// 2.0 Using a 64-bit uint_64 instead of an array to store the room structure.
//       This means width*length is limited to 64.


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include <errno.h>

struct position {
  short int w;
  short int l;
};

const struct position up    = { 0, -1};
const struct position down  = { 0,  1};
const struct position left  = {-1,  0};
const struct position right = { 1,  0};



// room setup
short int width;
short int length;
struct position end_room;


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



// translate pos into bitmask for uint64_t rooms variable
uint64_t get_bitmask(struct position pos) {
  return (uint64_t)1 << (pos.w + (pos.l * width));
}

// translate (w,l)-coordinates to struct position
struct position get_position(short int w, short int l) {
  struct position pos = {w, l};
  return pos;
}

// add corresponding coordinates of a and b
// if b is one of the directional consts above, moves one room in b direction
struct position move(struct position a, struct position b) {
  struct position pos = {a.w + b.w, a.l + b.l};
  return pos;
}


// check if room at pos is empty
bool room_free(struct position pos, uint64_t rooms) {
  return (0 == (get_bitmask(pos) & rooms));

}

// display room_array in a friendly way
void print_rooms(uint64_t rooms) {
  struct position pos = {0, 0};
  while (pos.l < length) {
    pos.w = 0;
    while (pos.w < width) {
      print_verbose("%d ", !room_free(pos, rooms));
      pos.w++;
    }
    print_verbose("\n");
    pos.l++;
  }
}


// search() and search2(): recursive methods to do the path search
// search() checks for solutions or a dead end, and calls search2() to
//     continue the search.
// search2() simply calls search() in the four directions.

int search(struct position pos, uint64_t rooms, int rooms_left);

int search2(struct position pos, uint64_t rooms, int rooms_left) {
    int solution_count = 0;
    short int w = pos.w;
    short int l = pos.l;

    if (pos.w > 0) {
      solution_count += search(move(pos, left), rooms, rooms_left);
    }

    if (pos.l > 0) {
      solution_count += search(move(pos, up), rooms, rooms_left);
    }

    if (pos.w < (width - 1)) {
      solution_count += search(move(pos, right), rooms, rooms_left);
    }

    if (pos.l < (length - 1)) {
      solution_count += search(move(pos, down), rooms, rooms_left);
    }

    return solution_count;
}

int search(struct position pos, uint64_t rooms, int rooms_left) {
  // Print some dots so we can monitor the speed
  static uint64_t       search_count = 0;
  const static uint64_t search_count_max = 1 << 30;
  search_count++;
  if (0 == (search_count % search_count_max)) {
    print_verbose(".");
    fflush(NULL);
  }

  if (room_free(pos, rooms)) {
    if (0 < rooms_left) {
      // This room is empty and there are rooms left, so continue the search!
      return search2(pos, (rooms | get_bitmask(pos)), (rooms_left - 1));
    }

    // This room is empty, so it could be the end room
    if ((pos.w == end_room.w) && (pos.l == end_room.l)) {
      // No rooms left and this is the end room, so we found a solution!
      return 1;
    }
  }

  // This is not a free room, so this is not the way!
  return 0;
}


int main(int argc, char *argv[]) {
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


  uint64_t rooms     = 0;
  int      num_rooms = 0;
  struct position start_room;

  // Read rooms from stdin
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

  struct position pos = {0, 0};
  while (pos.l < length) {
    pos.w = 0;
    while (pos.w < width) {
      int val;
      if (scanf("%d", &val) < 0) {
        fprintf(stderr, "Input error: rooms[w: %d][l: %d]\n", pos.w, pos.l);
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
        rooms |= get_bitmask(pos);
      }
      else if (val == 2) {
        rooms |= get_bitmask(pos);
        start_room = pos;
      }
      else if (val == 3) {
        end_room = pos;
      }
      pos.w++;
    }
    pos.l++;
  }


  // Print helpful info
  print_verbose("width: %hd, length: %hd\n", width, length);
  print_verbose("start_room: %hd, %hd\n", start_room.w, start_room.l);
  print_verbose("end_room: %hd, %hd\n", end_room.w, end_room.l);
  print_rooms(rooms);
  print_verbose("rooms: %lu\n", rooms);

  // Search!
  int solution_count = search2(start_room, rooms, num_rooms);
  print_verbose("\nsolutions: ");
  print_normal("%d\n", solution_count);
}
