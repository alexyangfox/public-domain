.global systable_start
.global systable_end


.align 64


/ ****************************************************************************

.data


systable_start:
	.long Exit
	.long CreateProcess
	.long Fork
	.long Exec
	.long WaitPid
	.long GetPID

	.long UMap
	.long UUnmap
	.long UProtect

	.long USleep
	.long UGetTimeOfDay

	.long Mount
	.long Unmount
	.long Open
	.long Close
	.long Unlink
	.long Pipe
	.long Dup
	.long Dup2
	.long Rename
	.long Ftruncate
	.long Fstat
	.long Fstatvfs
	.long Isatty
	.long Sync
	.long Fsync
	.long Read
	.long Write
	.long Seek
	.long Getcwd
	.long Chdir
	.long Mkdir
	.long Rmdir

	.long Opendir
	.long Closedir
	.long Readdir
	.long Rewinddir
	
	.long UDebug
	
	.long Tcgetattr
	.long Tcsetattr
	
	.long Fcntl
	.long USizeOfMap
	.long Stat
	
	.long SetAssign
	.long GetAssign
	
	.long Ioctl
	.long Access
	.long Umask
	.long Chmod
	.long Chown

	.long GetUID
	.long SetUID
	.long GetEUID
	.long GetGID
	.long SetGID
	.long GetEGID
	.long SetPGID
	.long GetPGID
	.long GetPGRP
	.long Format
	
	.long Tcsetpgrp
	.long Tcgetpgrp
	.long UKill
	.long SetPGRP
	
	.long USigAction
	.long USigSuspend
	.long USigProcMask
	.long USigSetTrampoline
	.long USigPending
	
	.long GetPPID
	
	.long UAlarm
	
	.long GetPageSize
	.long Inhibit
	.long Uninhibit
	.long Relabel
	.long Reboot
	.long MountInfo
	
systable_end:


