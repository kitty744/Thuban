# Stdio library documentation for Thuban.

Welcome! Here you will find documentation on how to use the Stdio library.

## 1. printf()

**About:** printf Is a method defined In the stdio library for Thuban, It allow's the user to print something to the screen with formatting.

**printf** come's with many formatting specificers. The most common one's are:

```
%d - Print's integer
%c - Print's character
%s - Print's string
```

**Example Usage:**

```
#include <stdio.h>

void kmain()
{
    printf("Hello, world! This Is an integer: %d", 1);
}

output: 1
```

As you can see from the **Example Usage**, you place the format Inside the "" string, then have the variable/value after the comma.
