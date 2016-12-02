# Polysync Transcoder

The application `polysync-transcode` converts the binary PLog flat files created by
Polysync ('.plog') to external formats used in industry standard data
science and machine learning tools.  It can also query, slice and dice, and
re-encode the PLog format.

The Transcoder consists of several components:

* Command line tool `polysync-transcode`, inspired by the industry standard
HDF5 workflow including `h5ls` and `h5dump`.
* Set of output plugins tailored to a specific external format like PLog, and HDF5 (planned) 
* Set of data model descriptions that describe the layout of arbitrary binary blobs
* C++ library for users to write their own custom data science tools
* Python extension (planned)

# Command Line Tool: Part I

Suppose you have a PLog file `ibeo.25.plog` fresh from your latest drive, and
you need to find out what it contains.  The transcoder gives you some
exploratory options.  (For practice, you can find the file `ibeo.25.plog` in the
transcoder distribution among the testing artifacts.)

## Identify the contents of a PLog

### Count the instances of each datatype:
~~~
➜ polysync-transcode ibeo.25.plog
ibeo.header x25
ibeo.resolution_info x8
ibeo.scan_data x8
ibeo.scan_point x8
ibeo.scanner_info x8
ibeo.vehicle_state x9
log_record x25
msg_header x25
ps_byte_array_msg x25
~~~

This is the complete list of all the compound datatypes found in the file.

### Print the data model

Great, but what is a "ibeo.vehicle_state", anyway?  Introspect and print the data
model using the `--detail` flag:

~~~
➜  polysync-transcode ibeo.25.plog --detail
ibeo.header {
    magic: uint32
    ...
    time: uint64
} x1
...
ibeo.vehicle_state {
    skip-1: skip-1(4)
    ...
    longitudinal_acceleration: float
} x9
...
ps_byte_array_msg {
    dest_guid: uint64
    data_type: uint32
    payload: uint32
} x25
~~~

Woah!  That is way too much information.  

### Filter on the type name:
~~~
➜  polysync-transcode ibeo.25.plog --detail --type ibeo.header
ibeo.header {
    magic: uint32
    prev_size: uint32
    size: uint32
    skip-1: skip-1(1)
    device_id: uint8
    data_type: uint16
    time: uint64
} x25
~~~
Much better.  Furthermore, `--type` is a composing argument so these two equivalent commands show two models:
~~~
➜  polysync-transcode ibeo.25.plog --detail --type ibeo.header ps_byte_array_msg
➜  polysync-transcode ibeo.25.plog --detail --type ibeo.header --type ps_byte_array_msg
ibeo.header {
    magic: uint32
    prev_size: uint32
    size: uint32
    skip-1: skip-1(1)
    device_id: uint8
    data_type: uint16
    time: uint64
} x25
ps_byte_array_msg {
    dest_guid: uint64
    data_type: uint32
    payload: uint32
} x25
~~~
## Filter on the record number
What types are contained by the 2nd record (0 based offset)?
~~~
➜  polysync-transcode ibeo.25.plog --slice 1
ibeo.header x1
ibeo.resolution_info x1
ibeo.scan_data x1
ibeo.scan_point x1
ibeo.scanner_info x1
log_record x1
msg_header x1
ps_byte_array_msg x1
~~~
What types are contained in the first 20 records?
~~~
➜  polysync-transcode ibeo.25.plog --slice :20
ibeo.header x20
ibeo.resolution_info x7
ibeo.scan_data x7
ibeo.scan_point x7
ibeo.scanner_info x7
ibeo.vehicle_state x7
log_record x20
msg_header x20
ps_byte_array_msg x20
~~~
In general, the `--slice` argument accepts any slice using the syntax swiped
from Python Numpy.  So you can select a portion of the PLog for processing that
is the first 20 records using `--slice :20`, all but the first 20 records using
`--slice 20:`, a set from the middle using `--slice 10:20`, subsample every 3rd
record using `--slice :3:`. The transcoder can only iterate forward in the
file, so negative strides are not allowed. Additionally, the transcoder does
not know the length of file until it gets there, so negative indices are also
not allowed - i.e., `--slice -20:` would be nice syntax to select the last 20
records, but is not supported because there is no efficient way to implement
this feature.

## Print the contents of a PLog

Printing the data contained in a PLog quickly becomes unwieldy; the filters are
useful to zero in on what you want to see.

### Print, or "dump", the first record in the file

~~~
➜  polysync-transcode ibeo.25.plog --slice 0 dump
log_record {
    index: 0
    size: 150
    prev_size: 0
    timestamp: 1464470621734607
    msg_header {
        type: 16
        timestamp: 1464470621734607
        src_guid: 0
    } 
    ps_byte_array_msg {
        dest_guid: 0
        data_type: 160
        payload: 114
    } 
    ibeo.header {
        magic: 0xaffec0c2
        ...
        time: 0xdaf49729f8bb2bbb
    } 
    ibeo.vehicle_state {
        skip-1: [ ff ff ff ff ] (4 elements)
        timestamp: 0xdaf4972a2b2e3822
        distance_x: 784592
        distance_y: -2097145
        ...
        longitudinal_acceleration: nan
    } 
} 

~~~

## PLog Encoding

The first actual transcoding example will write the data back out to another
PLog file.  This would be pretty useless, except for the filters which makes
PLog->Plog transcoding interesting.

### Truncation

~~~
➜  polysync-transcode ibeo.25.plog --slice :2 plog -n ibeo.2.plog
➜  polysync-transcode ibeo.2.plog --type log_record
log_record x2
~~~
So we have a new file `ibeo.2.plog` that contains a duplicate of the header
plus first two records from `ibeo.26.plog`. This can be checked:
~~~
➜  wc -c ibeo.2.plog
28216 ibeo.2.plog
➜  cmp ibeo.2.plog ibeo.25.plog --bytes 28216
~~~

# Command Line Tool: Part II

So Part I was nice and all, but you are interested in the introspection features probably because something is going wrong in your workflow.
