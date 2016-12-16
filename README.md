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
PLog file.  This is useful in combination with filters, because one truncate a
larger PLog into a smaller one, write out a new PLog with only chosen types, or
subsample a limited range of records, to create a smaller and more specific
PLog file from a larger one.

### Truncation

~~~
➜  polysync-transcode ibeo.25.plog --slice :2 plog -o ibeo.2.plog
➜  polysync-transcode ibeo.2.plog --type log_record
log_record x2
~~~
The first command created a new file `ibeo.2.plog` that contains a duplicate of
the header ps_log_header from ibeo.25.plog, plus its first two records.  This
is because the filter argument '--slice :2' selects the records from the
beginning of the file and stops after the second one. This can be checked with
a binary comparison:
~~~
➜  wc -c ibeo.2.plog
28216 ibeo.2.plog
➜  cmp ibeo.2.plog ibeo.25.plog --bytes 28216
~~~
The first command just computes how many bytes to compare (which is the entire
file ibeo.2.plog) and the second one is a standard command line tool to check
binary equality.

# Command Line Tool: Part II

The Transcoder privides valuable introspection tools to create and diagnose
sensor data models.

## Model Diagnostics

~~~
➜  polysync-transcode ibeo.25.plog --slice 2 dump
log_record {
    index: 2
    size: 5510
    prev_size: 26840
    timestamp: 1464470621735352
    msg_header {
        ...
    } 
    ps_byte_array_msg {
        ...
    } 
    ibeo.header {
        magic: 0xaffec0c2
        ...
        data_type: 0x2280
        time: 0xdaf49729f8d76ab6
    } 
    raw: [ 70 6c 91 2 0 0 0 0 f8 ab 90 2 ... 0 0 88 57 2c 5c ff 7f 0 0 2 0 ] (5450 elements)
} 
~~~
Arg, what is up with that last field `raw: ...`?  The first two records in the file have no
raw fields, just fully decoded records of type `ibeo.vehicle_state` and
`ibeo.scan_data`, respectively.  (Hint: the transcoder will read raw bytes like
this whenever it cannot determine a better model.)  Diagnose the problem:
~~~
➜  polysync-transcode ibeo.25.plog --slice 2 dump --loglevel debug1
...
transcode[debug1]: loading descriptions from "../share/ibeo.toml"
transcode[debug1]: loading descriptions from "../share/sensor.toml"
transcode[debug1]: loading descriptions from "../share/core.toml"
...
transcode[verbose]: index: 2 size: 5510 prev_size: 26840 timestamp: 1464470621735352 
detector[debug1]: ps_byte_array_msg matched from parent "msg_header"
detector[debug1]: ibeo.header matched from parent "ps_byte_array_msg"
detector[debug1]: type not detected, returning raw sequence
log_record {
    ...
~~~
Turning on the `debug1` loglevel shows us that the type descriptions are loaded
from TOML files.  While parsing record #2, we see the types
`ps_byte_array_msg`, and then `ibeo.header` were "matched", then the "type was
not detected" so we got the raw sequence instead of a decoded type.

What if this result is surprising, and you think that ibeo type 0x2280 should
be fully decoded because the description actually exists?  Check more detail:

~~~
➜  polysync-transcode ibeo.25.plog --slice 2 dump --loglevel debug2
...
detector[debug2]: ibeo.vehicle_state not matched: parent "msg_header" != "ibeo.header"
detector[debug2]: ibeo.scan_data not matched: parent "msg_header" != "ibeo.header"
detector[debug2]: ibeo.header not matched: parent "msg_header" != "ps_byte_array_msg"
detector[debug2]: ps_image_data_msg: mismatched { type: 16 != 9 }
detector[debug2]: ps_can_frame_msg: mismatched { type: 16 != 9 }
detector[debug1]: ps_byte_array_msg matched from parent "msg_header"
decoder[debug2]: decoding "ps_byte_array_msg" at offset 28256
...
detector[debug2]: ibeo.vehicle_state not matched: parent "ps_byte_array_msg" != "ibeo.header"
detector[debug2]: ibeo.scan_data not matched: parent "ps_byte_array_msg" != "ibeo.header"
detector[debug2]: ps_image_data_msg not matched: parent "ps_byte_array_msg" != "msg_header"
detector[debug2]: ps_byte_array_msg not matched: parent "ps_byte_array_msg" != "msg_header"
detector[debug2]: ps_can_frame_msg not matched: parent "ps_byte_array_msg" != "msg_header"
detector[debug1]: ibeo.header matched from parent "ps_byte_array_msg"
decoder[debug2]: decoding "ibeo.header" at offset 28272
...
detector[debug2]: ibeo.vehicle_state: mismatched { data_type: 0x2280 missing from description }
detector[debug2]: ibeo.scan_data: mismatched { data_type: 0x2280 missing from description }
detector[debug2]: ibeo.header not matched: parent "ibeo.header" != "ps_byte_array_msg"
detector[debug2]: ps_image_data_msg not matched: parent "ibeo.header" != "msg_header"
detector[debug2]: ps_byte_array_msg not matched: parent "ibeo.header" != "msg_header"
detector[debug2]: ps_can_frame_msg not matched: parent "ibeo.header" != "msg_header"
detector[debug1]: type not detected, returning raw sequence
~~~
Here, we see the transcoder searching for the type, including mismatched types.
For more insight, the detectors are defined, partly in `share/ibeo.toml`.  In
there, the `ibeo.header` type contains two detectors: one for
`ibeo.vehicle_state` and another for `ibeo.scan_data`.  In the above debug
output, the messages including `not matched: parent` are saying the message
cannot match because the previous message, the "parent", is the wrong type.
The other reason that a detector fails is because the field values do not match
the (otherwise correct) parent.  For instance, an `ibeo.vehicle_state` message
always follows an `ibeo.header`, and for the match to succeed, two fields
(`magic` and `data_type`), have to contain the exact Ibeo magic number
`0xAFFEC0C2` and the exact `data_type` value 0x2807.

So now we see that the reason that record #2 fails to decode is because it has
a field `data_type` with value 0x2280, which has no detector defined for it.
To fix this, you would add a new detector to the `ibeo.header` section in the
file `ibeo.toml`.

# Describing a Data Model

Sensors typically create a binary blob that is sensor data serialized as a mix
of data types.  PolySync writes these blobs, unmodified, as PLog payloads
inside of `ps_byte_array_msg`, `ps_can_frame_msg`, `ps_image_data_msg`, and
others.  The job of the transcoder is to decode these blobs into meaningful
numbers in a generic, flexible way, and possibly write them back out in a more
useful format.  Think of the TOML library as a uniform way to model every
message type from any sensor supported by PolySync, encapsulated all the
special domain knowledge of each sensor and it's programming manual.  In
addition to using a PolySync provided library of models, other TOML models may
be easily written and modified in the field by users.  This matters when the
sensor is proprietary, or if the user just wants the description immediately,
rather than wait for PolySync or some other third party to write the model.

In TOML, each model contains two sections: `description`, and `detector`.

## Description

Descriptions are just an array of fields.  Each field is either a table with
exactly two required entries: `name`, and `type`, or something special like
`skip` reserved or padding bytes.  Extra metadata may also be described; at the
moment, the supported metadata fields are `endian`, which can only equal the
value `big`, and `format`, which can hold the value `hex`.  Setting the
`endian` field to `big` instructs the decoder to swap the byte order on a
little endian processor.  The `format` field allows specialization of how to
print the value; right now only `hex` is supported but formatters for various
time formats is planned.

~~~
[ibeo.header]
    description = [
        { name = "magic", type = "uint32", endian="big", format="hex" },
        { name = "prev_size", type = "uint32", endian="big" },
        { name = "size", type = "uint32", endian="big" },
        { skip = 1 },
        { name = "device_id", type = "uint8" },
        { name = "data_type", type = "uint16", endian="big", format = "hex" },
        { name = "time", "type" = "uint64", endian="big", format="hex" }
    ]
    detector = ...
~~~

## Detector

Many types, especially headers, do not describe the entire binary blob, but
rather represent a generic structure used across a wide variety of specific
messages.  The `detector` section specifies what to do with the remaining bytes
in a blob after the first type (the "parent") is decoded.  Most header types
will have many detectors, but not every type will have any detectors at all.
In the Ibeo case, `ibeo.vehicle_state` and `ibeo.scan_data` gobble the entire
remainder of the blob, so they contain no detectors.  They may still have
nested types, such as `ibeo.scan_point`, that need no detector because one of
the fields in the type names the nested type explicitly and there is no choice
the transcoder must make.
~~~
[ibeo.header]
    description = ... 
    detector = [ 
        { name = "ibeo.vehicle_state", magic = "0xAFFEC0C2", data_type = "0x2807" }, 
        { name = "ibeo.scan_data", magic = "0xAFFEC0C2", data_type = "0x2205" }
    ] 
~~~

