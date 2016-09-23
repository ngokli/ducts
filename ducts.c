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
// 1D array to store rooms
// So smaller datacenters can be cached as close to the processor as possible
// (Using a fixed-sized 2D array would leave gaps in the data)
short int room_array[1024];
struct position start_room;



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



// translate (w,l)-coordinates to room_array index
int array_pos(short int w, short int l) {
  return w + (l * width);
}

// translate (w,l)-coordinates to struct position
struct position get_position(short int w, short int l) {
  struct position pos = {w, l};
  return pos;
}

// add corresponding coordinates of a and b
// if b is one of the directional consts above, moves one room in b direction
struct position move(struct position a, struct position b) {
  struct position retval = {a.w + b.w, a.l + b.l};
  return retval;
}

// get value from room_array
short int get_room_array_val(const struct position pos) {
  return room_array[array_pos(pos.w, pos.l)];
}

// set value in room_array
void set_room_array_val(const struct position pos, short int val) {
  room_array[array_pos(pos.w, pos.l)] = val;
}



// display room_array in a friendly way
void print_rooms() {
  for (int l = 0; l < length; l++) {
    for (int w = 0; w < width; w++) {
      print_verbose("%hd ", get_room_array_val(get_position(w, l)));
    }
    print_verbose("\n");
  }
}


// search() and search2(): recursive methods to do the path search
// search() checks for solutions or a dead end, and calls search2() to
//     continue the search.
// search2() simply calls search() in the four directions.

int search(struct position pos, int rooms_left);

int search2(struct position pos, int rooms_left) {
    int solution_count = 0;
    short int w = pos.w;
    short int l = pos.l;

    if (pos.w > 0) {
      solution_count += search(move(pos, left), rooms_left);
    }
    
    if (pos.l > 0) {
      solution_count += search(move(pos, up), rooms_left);
    }

    if (pos.w < (width - 1)) {
      solution_count += search(move(pos, right), rooms_left);
    }

    if (pos.l < (length - 1)) {
      solution_count += search(move(pos, down), rooms_left);
    }

    return solution_count;
}

int search(struct position pos, int rooms_left) {
  // Print some dots so we can monitor the speed
  static uint64_t       search_count = 0;
  const static uint64_t search_count_max = 1 << 30;
  search_count++;
  if (0 == (search_count % search_count_max)) {
    print_verbose(".");
    fflush(NULL);
  }

  short int room_val = get_room_array_val(pos);
  if (rooms_left == 0) {
    if (room_val == 3) {
      // If no rooms left and this is the end room, we found a solution!
      return 1;
    }
    else {
      // If no rooms left and this is not the end room, not a solution.
      // (Is this possible, since we check below to avoid the end room when
      // there are rooms left?)
      return 0;
    }
  }


  if (room_val == 0) {
    // This room is empty, so continue the search!
    set_room_array_val(pos, 1);
    int solution_count = search2(pos, rooms_left - 1);
    set_room_array_val(pos, 0);

    return solution_count;
  }

  // We have rooms left, but this isn't an empty room, so this is not the way!
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
  
  
  int num_rooms = 0;

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

  for (int l = 0; l < length; l++) {
    for (int w = 0; w < width; w++) {
      short int val;
      if (scanf("%hd", &val) < 0) {
        fprintf(stderr, "Input error: rooms[w: %d][l: %d]\n", w, l);
        exit(1);
      }
      else if (errno != 0) {
        perror("Error reading rooms");
        exit(errno);
      }
      room_array[array_pos(w, l)] = val;
      if (val == 0) {
        num_rooms++;
      }
      if (val == 2) {
        start_room = get_position(w, l);
      }
    }
  }


  // Print helpful info
  print_verbose("len: %hd, width: %hd\n", length, width);
  print_rooms();
  
  // Search!
  int solution_count = search2(start_room, num_rooms);
  print_verbose("\nsolutions: ");
  print_normal("%d\n", solution_count);
}
