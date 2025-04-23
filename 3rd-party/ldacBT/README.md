# ldacBT

AOSP libldac dispatcher

https://android.googlesource.com/platform/external/libldac

## Build
```bash
git clone https://github.com/EHfive/ldacBT.git
cd ldacBT
git submodule update --init
```

----------

```bash
mkdir build && cd build
cmake \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DINSTALL_LIBDIR=/usr/lib \
    -DLDAC_SOFT_FLOAT=OFF \
    ../
make DESTDIR=$DEST_DIR install
```

### Cmake options
| option/definition | description | default value |
|--------|-------------|---------------|
|CMAKE_INSTALL_PREFIX|
|INSTALL_LIBDIR|path to shared libraries dir|${CMAKE_INSTALL_PREFIX}/lib|
|INSTALL_INCLUDEDIR|path to header include dir|${CMAKE_INSTALL_PREFIX}/include|
|INSTALL_PKGCONFIGDIR|path to pkg-config dir|${INSTALL_LIBDIR}/pkgconfig|
|INSTALL_LDAC_INCLUDEDIR|path to ldacBT headers dir|${INSTALL_INCLUDEDIR}/ldac|
|LDAC_SOFT_FLOAT|ON/OFF inner soft-float function|OFF|

## Copyright

### libldac
```
 Copyright (C) 2013 - 2016 Sony Corporation
 
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
```

NOTICE
```
---------------
 Certification
---------------
   Taking the certification process is required to use LDAC in your products.
   For the detail of certification process, see the following URL:
      https://www.sony.net/Products/LDAC/aosp/

```

### this repo
```
 Copyright 2018-2019 Huang-Huang Bao

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
```
