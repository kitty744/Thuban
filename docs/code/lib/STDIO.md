# Stdio library documentation for Thuban.

Here you will find documentation on how to use the **stdio** library.
The **stdio** library provide's many helpers, used to make development way easier.

## 1. printf()

**About:** printf Is a method defined In the stdio library for Thuban, It allow's the user to print something to the screen with formatting.

**printf** come's with many formatting option's. The most common one's are:

| Format  | Value                |
| :------ | :------------------- |
| **%d**  | **Integer**          |
| **%ud** | **Unsigned Integer** |
| **%c**  | **Character**        |
| **%s**  | **String**           |

**Example Usage:**

```
#include <stdio.h>

void example()
{
    printf("INTEGER: %d, STRING: %s, CHARACTER: %c", 1, "Hi", 'C');
}

output: 1
```
