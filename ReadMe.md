## System Information

C-library, that provide CPU, RAM and HDD information.

## Example

In one .cpp/.c file add :
``` cpp
#define SYSTEM_INFORMATION_IMPLEMENTATION
#include "SystemInformation.h"
```

Then you can use it :
``` cpp
struct SystemInformation information;
if (!getSystemInformation(&information))
{
    //  something went wrong.
}
```
