;;;; -*- Mode: Lisp ; Package: Toolkit -*-
;;;
;;; **********************************************************************
;;; This code was written as part of the CMU Common Lisp project at
;;; Carnegie Mellon University, and has been placed in the public domain.
;;;
(ext:file-comment
  "$Header: interface-build.lisp,v 1.3 94/10/31 04:54:48 ram Exp $")
;;;
;;; **********************************************************************
;;;
;;; Written by Michael Garland
;;;
;;; Interface Builder
;;;
;;; This defines functions to crunch through the information generated
;;; about the various request operations and automatically generate the
;;; interface files for the C server.

(in-package "TOOLKIT")



;;;; Files where interface information will be stored

(defconstant *string-table-file* "target:motif/server/StringTable.h")
(defconstant *class-file* "target:motif/server/ClassTable.h")
(defconstant *interface-file* "target:motif/server/Interface.h")
(defconstant *type-file* "target:motif/server/TypeTable.h")



;;;; Functions for building the C interface files

(defun build-class-file (out)
  (declare (stream out))
  (write-line "WidgetClass *class_table[] = {" out)
  (dotimes (index (length *class-table*))
    (let ((entry (aref *class-table* index)))
      (format out "  (WidgetClass *)(&~a),~%" (cdr entry))))
  (format out "  NULL~%};~%~%#define CLASS_TABLE_SIZE ~a~%"
	  (length *class-table*)))

(defun build-type-file (out)
  (declare (stream out))
  (write-line "type_entry type_table[] = {" out)
  (dotimes (index next-type-tag)
    (let* ((stuff (svref *type-table* index))
	   (name (car stuff))
	   (kind (cdr stuff)))
      (if (and (eq kind name) (gethash kind *enum-table*))
	  (setf kind (symbol-atom :enum))
	  (setf kind (symbol-atom kind)))
      (format out "  {\"~a\",message_write_~(~a~),message_read_~(~a~)},~%"
	      (symbol-class name) kind kind)))
  (format out "  {NULL,NULL,NULL}~%};~%~%#define TYPE_TABLE_SIZE ~a~%"
	  next-type-tag))

(defun build-interface-file (out)
  (declare (stream out))
  (write-line "request_f request_table[] = {" out)
  (dotimes (index (length *request-table*))
    (format out "  ~a,~%" (aref *request-table* index)))
  (format out "  NULL~%};~%"))

(defun build-string-table (out)
  (declare (stream out))
  (let ((table xti::*toolkit-string-table*))
    (declare (simple-vector table))
    (write-line "String string_table[] = {" out)
    (dotimes (index (length table))
      (format out "  \"~a\",~%" (svref table index)))
    (format out "  NULL~%};~%~%")
    (format out "#define STRING_TABLE_SIZE ~a~%" (length table))))

(defun build-toolkit-interface ()
  (with-open-file (out *class-file*
		       :direction :output :if-exists :supersede
		       :if-does-not-exist :create)
    (build-class-file out))
  (with-open-file (out *type-file*
		       :direction :output :if-exists :supersede
		       :if-does-not-exist :create)
    (build-type-file out))
  (with-open-file (out *interface-file*
		       :direction :output :if-exists :supersede
		       :if-does-not-exist :create)
    (build-interface-file out))
  (with-open-file (out *string-table-file*
		       :direction :output :if-exists :supersede
		       :if-does-not-exist :create)
    (build-string-table out)))
