.section ".data"
.align 4

.global _binary_initfs_bin_start
_binary_initfs_bin_start:

.incbin "initfs.bin"

.global _binary_initfs_bin_end
_binary_initfs_bin_end:
