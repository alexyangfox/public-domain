;;; -*- Mode: Lisp; Package: System -*-
;;;
;;; **********************************************************************
;;; This code was written as part of the CMU Common Lisp project at
;;; Carnegie Mellon University, and has been placed in the public domain.
;;;
(ext:file-comment
  "$Header: generic-site.lisp,v 1.11 94/10/31 04:11:27 ram Exp $")
;;;
;;; **********************************************************************
;;;
;;; Generic site specific initialization for CMU CL.  This can be used as a
;;; template for non-cmu "library:site-init" files.
;;;
(in-package "SYSTEM")

;;; Put your site name here...
(setq *short-site-name* "Unknown")
(setq *long-site-name* "Site name not initialized")

;;; We would appreciate it if each site establishes a local maintainer who can
;;; filter bug reports from novice users to make sure that they really have
;;; found a bug.  Fill in the maintainer's address here..
(rplaca
 (cdr (member :bugs *herald-items*))
 '("Send bug reports and questions to your local CMU CL maintainer, "
   "or to" terpri
   "cmucl-bugs@cs.cmu.edu." terpri
   "Loaded subsystems:" terpri))

;;; If you have sources installed on your system, un-comment the following form
;;; and change it to point to the source location.  This will allow the Hemlock
;;; "Edit Definition" command and the debugger to find sources for functions in
;;; the core.
#|
(setf (search-list "target:") "<the source tree root>/")
|#
