# Native part of Dousi

## What we provide here?
(1) A method discovery service provided.  
(2) Server JNI of Dousi RPC provided.  
(3) Client JNI of Dousi RPC provided.  

## Setup dependencies
### Build Boost.asio
(1) Download the Boost.asio from https://www.boost.org/users/download/
(2) Unzip to a the a directory like `build`
(3) `cd boost_1_73_0 && sh bootstrap.sh`      # Prepare
(4) `./b2 --with-system --with-thread --with-date_time --with-regex --with-serialization stage`  # Compile and install
(5) Now the library thread, date_time regex anf serialization were installed.

## Design details
1. We use [Nameof](https://github.com/Neargye/nameof) repository to get the name of services, rpc methods in compling time.
