# Polysync Transcoder

The application `polysync-transcode` converts the binary PLog flat files created by
Polysync ('.plog') to external formats used in industry standard data
science and machine learning tools.  It can also query, slice and dice, and
re-encode the PLog format.


The Transcoder consists of several components:

* Command line tool `polysync-transcode`, inspired by the industry standard
HDF5 workflow including `h5ls` and `h5dump`.
* Set of output plugins tailored to a specific external format (like HDF5)
* Set of data model descriptions that describe the layout of arbitrary binary blobs
* C++ library
* Python extension (planned)

# Command line Tool

## Identify the contents of a plog

### Count the instances of each datatype:
~~~
➜ transcode ibeo.1.plog
ibeo.header x1
ibeo.vehicle_state x1
log_record x1
msg_header x1
ps_byte_array_msg x1
~~~

### Print the data model

~~~
➜  build git:(PS-1463-work) ✗ transcode ibeo.1.plog model --detail
ibeo.header {
    magic: uint32
    prev_size: uint32
    size: uint8
    skip:1: skip-1(4)
    device_id: uint8
    data_type: uint16
    time: uint64
} x1
ibeo.vehicle_state {
    skip:1: skip-1(4)
    timestamp: uint64
    distance_x: int32
    ...
~~~

### Print the first record in the file "input.plog":

The argument '--last 1' stops the program after the first log record.  The plugin 'dump' will print the contents of the file to the console.

`transcode --last 1 input.plog dump`

* log_record:
    * index: 0
    * size: 150
    * prev_size: 0
    * timestamp: 1464470621734607
    * msg_header:
        * type: 16
        * timestamp: 1464470621734607
        * src_guid: 0
    * ps_byte_array_msg:
        * dest_guid: 0
        * data_type: 160
        * payload: 114
    * ibeo.header:
        * magic: 2952708290
        * prev_size: 5458
        * size: 0
        * device_id: 0
        * data_type: 10247
        * time: 15777401601108159419
    * ibeo.vehicle_state:
        * timestamp: 15777401601954560034
        * distance_x: 784592
        * distance_y: -2097145
        * course_angle: 0.623269
        * longitudinal_velocity: 0.01
        * ...

