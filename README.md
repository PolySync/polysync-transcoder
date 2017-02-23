### PolySync Transcoder

The application `polysync-transcoder` converts the binary flat files created by Polysync to external formats used in industry standard data science and machine learning tools. 

The `polysync-transcoder` is the recommended way to convert the time-sychronized logfile data captured by PolySync into other useful formats such as HDF5, rosbag, and more. 

Be sure to see the [Wiki](https://github.com/PolySync/polysync-transcoder/wiki) for tutorials and more detailed documentation.


#### When to use the PolySync Transcoder

The PolySync Transcoder has the ability to decode any `plog` file and tell you general information about the binary data within it. The Transcoder requires sensor descriptors to be able to describe the binary data. This is implemented through TOML files. Once the binary data has been modeled with TOML, the PolySync Transcoder uses custom plugins to re-encode the data to the desired format.

The `plog` format is very efficient in the vehicle, but is not ideal for data science and machine learning tools. The Transcoder tool provides a convenient interface to decode and translate massive amounts of time-synchronized PolySync logfile data. 


#### Installation

The following packages are required for the PolySync Transcoder to build and run successfully:

- GCC >= 6.0.0
- Clang >= 3.5.0
- Boost 1.63.0
- Mettle

To install the PolySync Transcoder and its dependencies follow the [installation guide](https://github.com/PolySync/polysync-transcoder/wiki/Install).


#### Examples

The best way to start with the PolySync Transcoder is to become familiar with the command-line interface. Learn how to [analyze a `plog` file](https://github.com/PolySync/polysync-transcoder/wiki/Query-commands) from the command line by using the PolySync Transcoder on the sample `plog` data.

Next the data needs to be modeled. See how to [write TOML models](https://github.com/PolySync/polysync-transcoder/wiki/Descriptions), and [write TOML descriptors](https://github.com/PolySync/polysync-transcoder/wiki/Descriptions).

The modeled data can be encoded to a new format through Transcoder plugins. See this article on [encoding `plog` data to new formats](https://github.com/PolySync/polysync-transcoder/wiki/Encoding-%60plog%60-Files).


#### Contact Information

Please direct questions regarding the PolySync Transcoder to help@polysync.io.

Distributed as-is; no warranty is given.

Copyright (c) 2017 PolySync Technologies, Inc. All Rights Reserved.
