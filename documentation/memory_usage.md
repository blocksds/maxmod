# Memory Usage

Maxmod's memory footprint is very small for its capability.

## GBA Memory Usage

The measurements below are taken using standard configuration (8 channels @
16KHz).

<center>
Memory Section         | Approximate Usage
-----------------------|------------------
EWRAM (256KB external) | 1.79 KB
IWRAM (32KB internal)  | 5.69 KB
ROM (cartridge)        | 6.54 KB
</center>

Note that the measurements do not calculate the extra memory required for the
soundbank data. The soundbank data size is optimized, and is to be loaded into
the cartridge space.

## DS Memory Usage

<center>
Memory Section    | Approximate Usage
------------------|------------------
IWRAM (ARM7 Code) | 19.25 KB
IWRAM (ARM7 Data) | 11.95 KB
EWRAM (ARM9 Code) | 2.37 KB
EWRAM (ARM9 Data) | 0.16 KB
</center>

Note that the measurements do not calculate the extra memory required for
soundbank data. This requirement depends on how many songs/samples will be
loaded into memory at a time. The loaded data will be placed in the 4 MB shared
external memory region.
