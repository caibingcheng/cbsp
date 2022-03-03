## cbsp

cbsp is a header-only C++ library. It provides utility methods for archiving and extracting files. tar is complex and heavy but cbsp is simple and light, and can be easily added to your project.

### Comparation


#### Version 22.2.10.0
##### archive

|files|size|cbsp|tar|
|--|--|--|--|
|1|1GB|0.642s|0.711s|
|1000|100MB|17.900s|0.147s|

##### extract

|files|size|cbsp|tar|
|--|--|--|--|
|1|1GB|0.836s|0.862s|
|1000|100MB|16.777s|0.180s|

### what's a cbsp file?

!["cbsp file"](https://cdn.jsdelivr.net/gh/caibingcheng/resources@main/images/cbsp-CBSPFile.png)

### what's the linkage of header and blockers?

!["cbsp linkage"](https://cdn.jsdelivr.net/gh/caibingcheng/resources@main/images/cbsp-Linkage.png)

### what's a cbsp header?

!["cbsp header"](https://cdn.jsdelivr.net/gh/caibingcheng/resources@main/images/cbsp-Header.png)

### what's a cbsp blocker?

!["cbsp blocker"](https://cdn.jsdelivr.net/gh/caibingcheng/resources@main/images/cbsp-Blocker.png)
