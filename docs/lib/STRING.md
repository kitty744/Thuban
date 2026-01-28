# String Library (string)

## Overview

The Thuban string library provides essential string manipulation and memory operations. It implements standard C library functions for string handling, memory copying, and comparison.

## Table of Contents

1. [String Length](#string-length)
2. [String Copying](#string-copying)
3. [String Comparison](#string-comparison)
4. [String Concatenation](#string-concatenation)
5. [String Searching](#string-searching)
6. [Memory Operations](#memory-operations)
7. [Implementation Details](#implementation-details)

---

## String Length

### strlen()

```c
size_t strlen(const char *s);
```

**Description:**  
Returns the length of a null-terminated string.

**Parameters:**

- `s`: Null-terminated string

**Returns:**  
Number of characters before null terminator

**Example:**

```c
const char *str = "Hello";
size_t len = strlen(str);  // Returns 5
```

**Time Complexity:** O(n)

**Notes:**

- Does NOT include null terminator in count
- Undefined behavior if `s` is not null-terminated
- Returns 0 for empty string

---

## String Copying

### strcpy()

```c
char *strcpy(char *dest, const char *src);
```

**Description:**  
Copies string from source to destination, including null terminator.

**Parameters:**

- `dest`: Destination buffer
- `src`: Source string

**Returns:**  
Pointer to `dest`

**Example:**

```c
char buffer[20];
strcpy(buffer, "Hello");
// buffer now contains "Hello\0"
```

**Warning:**  
No bounds checking! Can cause buffer overflow. Use `strncpy()` instead.

---

### strncpy()

```c
char *strncpy(char *dest, const char *src, size_t n);
```

**Description:**  
Copies up to `n` characters from source to destination.

**Parameters:**

- `dest`: Destination buffer
- `src`: Source string
- `n`: Maximum characters to copy

**Returns:**  
Pointer to `dest`

**Behavior:**

- Copies at most `n` characters
- Stops at null terminator if found before `n`
- Pads with null bytes if `src` < `n` characters
- **Does NOT null-terminate if `src` >= `n` characters**

**Example:**

```c
char buffer[10];
strncpy(buffer, "Hello, World!", sizeof(buffer));
buffer[sizeof(buffer)-1] = '\0';  // Ensure null termination
```

**Safe Usage:**

```c
// Always ensure null termination
strncpy(dest, src, sizeof(dest) - 1);
dest[sizeof(dest) - 1] = '\0';
```

---

## String Comparison

### strcmp()

```c
int strcmp(const char *s1, const char *s2);
```

**Description:**  
Compares two strings lexicographically.

**Parameters:**

- `s1`: First string
- `s2`: Second string

**Returns:**

- `< 0` if s1 < s2
- `0` if s1 == s2
- `> 0` if s1 > s2

**Example:**

```c
if (strcmp(cmd, "help") == 0) {
    print_help();
}

// Check alphabetical order
if (strcmp("apple", "banana") < 0) {
    printf("apple comes first\n");
}
```

**Notes:**

- Case-sensitive comparison
- Compares byte-by-byte
- Returns on first difference

---

### strncmp()

```c
int strncmp(const char *s1, const char *s2, size_t n);
```

**Description:**  
Compares up to `n` characters of two strings.

**Parameters:**

- `s1`: First string
- `s2`: Second string
- `n`: Maximum characters to compare

**Returns:**

- `< 0` if s1 < s2
- `0` if s1 == s2 (in first n chars)
- `> 0` if s1 > s2

**Example:**

```c
// Check command prefix
if (strncmp(cmd, "mem", 3) == 0) {
    // Handle commands starting with "mem"
}

// Safe string comparison with length limit
if (strncmp(input, "password", 8) == 0) {
    authenticate();
}
```

---

## String Concatenation

### strcat()

```c
char *strcat(char *dest, const char *src);
```

**Description:**  
Appends source string to destination string.

**Parameters:**

- `dest`: Destination string (must have space)
- `src`: Source string to append

**Returns:**  
Pointer to `dest`

**Example:**

```c
char buffer[50] = "Hello, ";
strcat(buffer, "World!");
// buffer now contains "Hello, World!"
```

**Warning:**  
No bounds checking! Can overflow. Use `strncat()` instead.

---

### strncat()

```c
char *strncat(char *dest, const char *src, size_t n);
```

**Description:**  
Appends up to `n` characters from source to destination.

**Parameters:**

- `dest`: Destination string
- `src`: Source string
- `n`: Maximum characters to append

**Returns:**  
Pointer to `dest`

**Example:**

```c
char buffer[20] = "Hello";
strncat(buffer, ", World!", sizeof(buffer) - strlen(buffer) - 1);
```

**Notes:**

- Always null-terminates
- Appends at most `n` characters from `src`
- Safer than `strcat()`

---

## String Searching

### strchr()

```c
char *strchr(const char *s, int c);
```

**Description:**  
Finds first occurrence of character in string.

**Parameters:**

- `s`: String to search
- `c`: Character to find

**Returns:**  
Pointer to first occurrence, or NULL if not found

**Example:**

```c
const char *str = "Hello, World!";
char *comma = strchr(str, ',');
if (comma) {
    printf("Found comma at position %ld\n", comma - str);
}
```

---

### strrchr()

```c
char *strrchr(const char *s, int c);
```

**Description:**  
Finds last occurrence of character in string.

**Parameters:**

- `s`: String to search
- `c`: Character to find

**Returns:**  
Pointer to last occurrence, or NULL if not found

**Example:**

```c
const char *path = "/usr/local/bin/program";
char *filename = strrchr(path, '/');
if (filename) {
    filename++;  // Skip the '/'
    printf("Filename: %s\n", filename);
}
```

---

### strstr()

```c
char *strstr(const char *haystack, const char *needle);
```

**Description:**  
Finds first occurrence of substring in string.

**Parameters:**

- `haystack`: String to search in
- `needle`: Substring to find

**Returns:**  
Pointer to first occurrence, or NULL if not found

**Example:**

```c
const char *text = "The quick brown fox";
char *found = strstr(text, "brown");
if (found) {
    printf("Found at: %s\n", found);  // "brown fox"
}
```

---

## Memory Operations

### memset()

```c
void *memset(void *s, int c, size_t n);
```

**Description:**  
Fills memory block with specified byte value.

**Parameters:**

- `s`: Pointer to memory block
- `c`: Byte value to fill (converted to unsigned char)
- `n`: Number of bytes to fill

**Returns:**  
Pointer to `s`

**Example:**

```c
char buffer[100];
memset(buffer, 0, sizeof(buffer));     // Zero buffer
memset(buffer, 'A', 10);                // Fill first 10 with 'A'

struct data obj;
memset(&obj, 0, sizeof(obj));          // Zero structure
```

**Common Uses:**

- Clear buffers: `memset(buf, 0, size)`
- Initialize arrays: `memset(array, -1, size)`
- Clear structures: `memset(&struct, 0, sizeof(struct))`

---

### memcpy()

```c
void *memcpy(void *dest, const void *src, size_t n);
```

**Description:**  
Copies `n` bytes from source to destination.

**Parameters:**

- `dest`: Destination pointer
- `src`: Source pointer
- `n`: Number of bytes to copy

**Returns:**  
Pointer to `dest`

**Example:**

```c
char src[] = "Hello";
char dest[10];
memcpy(dest, src, strlen(src) + 1);  // Copy with null terminator

// Copy structure
struct data src_data, dest_data;
memcpy(&dest_data, &src_data, sizeof(struct data));
```

**Warning:**  
Undefined behavior if source and destination overlap! Use `memmove()` instead.

---

### memmove()

```c
void *memmove(void *dest, const void *src, size_t n);
```

**Description:**  
Copies `n` bytes from source to destination, handling overlaps correctly.

**Parameters:**

- `dest`: Destination pointer
- `src`: Source pointer
- `n`: Number of bytes to copy

**Returns:**  
Pointer to `dest`

**Example:**

```c
char buffer[] = "Hello, World!";
// Shift string left by 2 (overlapping regions)
memmove(buffer, buffer + 2, strlen(buffer + 2) + 1);
// buffer now contains "llo, World!"
```

**Use Cases:**

- Overlapping memory regions
- Shifting data within same buffer
- When you're unsure if regions overlap

---

### memcmp()

```c
int memcmp(const void *s1, const void *s2, size_t n);
```

**Description:**  
Compares `n` bytes of two memory blocks.

**Parameters:**

- `s1`: First memory block
- `s2`: Second memory block
- `n`: Number of bytes to compare

**Returns:**

- `< 0` if s1 < s2
- `0` if s1 == s2
- `> 0` if s1 > s2

**Example:**

```c
uint8_t hash1[32], hash2[32];
if (memcmp(hash1, hash2, 32) == 0) {
    printf("Hashes match!\n");
}

// Compare structures
if (memcmp(&struct1, &struct2, sizeof(struct1)) == 0) {
    printf("Structures identical\n");
}
```

---

## Implementation Details

### Performance

**Optimized Operations:**

- `memset()` - Word-aligned writes when possible
- `memcpy()` - Word-aligned copies
- String operations - Simple byte-by-byte

**Alignment:**
Most functions work byte-by-byte and don't require alignment.

### Memory Safety

**Bounds Checking:**

- `strncpy()`, `strncat()`, `strncmp()` - Have size limits
- `strcpy()`, `strcat()`, `strcmp()` - **NO bounds checking**

**Null Termination:**

- String functions expect null-terminated strings
- `strncpy()` may NOT null-terminate if buffer is too small

### Optimization Flags

For better performance:

```makefile
CFLAGS += -O2 -fno-builtin
```

Some compilers have built-in optimizations for these functions that may be faster.

---

## Common Pitfalls

### 1. Buffer Overflow

**Bad:**

```c
char buf[10];
strcpy(buf, "This is too long!");  // OVERFLOW!
```

**Good:**

```c
char buf[10];
strncpy(buf, "This is too long!", sizeof(buf) - 1);
buf[sizeof(buf) - 1] = '\0';
```

### 2. Missing Null Termination

**Bad:**

```c
char buf[10];
strncpy(buf, "Exactly10!", sizeof(buf));  // Not null-terminated!
```

**Good:**

```c
char buf[10];
strncpy(buf, "Exactly10!", sizeof(buf) - 1);
buf[sizeof(buf) - 1] = '\0';
```

### 3. Overlapping Memory

**Bad:**

```c
char buf[] = "Hello";
memcpy(buf, buf + 1, 4);  // Undefined behavior!
```

**Good:**

```c
char buf[] = "Hello";
memmove(buf, buf + 1, 4);  // Correct
```

### 4. Off-by-One Errors

**Bad:**

```c
char dest[5];
strncpy(dest, "Hello", 5);  // No room for null!
```

**Good:**

```c
char dest[6];  // +1 for null terminator
strncpy(dest, "Hello", sizeof(dest) - 1);
dest[sizeof(dest) - 1] = '\0';
```

---

## Best Practices

### 1. Always Use Safe Functions

```c
// Prefer:
strncpy(), strncat(), strncmp()

// Over:
strcpy(), strcat(), strcmp() // (when dealing with untrusted input)
```

### 2. Check Return Values

```c
char *found = strchr(str, 'x');
if (found) {
    // Use found pointer
} else {
    // Handle not found
}
```

### 3. Ensure Null Termination

```c
char buf[SIZE];
strncpy(buf, src, sizeof(buf) - 1);
buf[sizeof(buf) - 1] = '\0';  // Always!
```

### 4. Use sizeof() for Buffer Sizes

```c
char buf[100];
strncpy(buf, src, sizeof(buf) - 1);  // Correct
// NOT: strncpy(buf, src, 100 - 1);   // Error-prone
```

---

## Performance Tips

### Fast String Copy

```c
// Fastest for small strings:
strcpy(dest, src);

// Fast for known length:
memcpy(dest, src, len + 1);  // +1 for null

// Safest:
strncpy(dest, src, sizeof(dest) - 1);
dest[sizeof(dest) - 1] = '\0';
```

### Fast Memory Clear

```c
// Clear structure:
memset(&obj, 0, sizeof(obj));

// Clear array (size known at compile time):
#define ARRAY_SIZE 100
static char array[ARRAY_SIZE];
memset(array, 0, sizeof(array));
```

---

## Testing

### Unit Test Examples

```c
void test_strlen(void) {
    assert(strlen("") == 0);
    assert(strlen("a") == 1);
    assert(strlen("Hello") == 5);
}

void test_strcmp(void) {
    assert(strcmp("abc", "abc") == 0);
    assert(strcmp("abc", "abd") < 0);
    assert(strcmp("abd", "abc") > 0);
}

void test_memcpy(void) {
    char src[] = "test";
    char dest[5];
    memcpy(dest, src, 5);
    assert(strcmp(dest, "test") == 0);
}
```

---

## See Also

- `STDIO.md` - Standard I/O functions

---

**Maintainer:** Trollycat  
**License:** MIT  
**Last Updated:** January 2026
