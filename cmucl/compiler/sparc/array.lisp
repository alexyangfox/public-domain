;;; -*- Package: SPARC -*-
;;;
;;; **********************************************************************
;;; This code was written as part of the CMU Common Lisp project at
;;; Carnegie Mellon University, and has been placed in the public domain.
;;;
(ext:file-comment
  "$Header: array.lisp,v 1.13 94/11/02 03:39:51 ram Exp $")
;;;
;;; **********************************************************************
;;;
;;;    This file contains the SPARC definitions for array operations.
;;;
;;; Written by William Lott
;;;
(in-package "SPARC")


;;;; Allocator for the array header.

(define-vop (make-array-header)
  (:translate make-array-header)
  (:policy :fast-safe)
  (:args (type :scs (any-reg))
	 (rank :scs (any-reg)))
  (:arg-types tagged-num tagged-num)
  (:temporary (:scs (descriptor-reg) :to (:result 0) :target result) header)
  (:temporary (:scs (non-descriptor-reg)) ndescr)
  (:results (result :scs (descriptor-reg)))
  (:generator 0
    (pseudo-atomic ()
      (inst or header alloc-tn other-pointer-type)
      (inst add ndescr rank (* (1+ array-dimensions-offset) vm:word-bytes))
      (inst andn ndescr 4)
      (inst add alloc-tn ndescr)
      (inst add ndescr rank (fixnum (1- vm:array-dimensions-offset)))
      (inst sll ndescr ndescr vm:type-bits)
      (inst or ndescr ndescr type)
      (inst srl ndescr ndescr 2)
      (storew ndescr header 0 vm:other-pointer-type))
    (move result header)))


;;;; Additional accessors and setters for the array header.

(defknown lisp::%array-dimension (t fixnum) fixnum
  (flushable))
(defknown lisp::%set-array-dimension (t fixnum fixnum) fixnum
  ())

(define-vop (%array-dimension word-index-ref)
  (:translate lisp::%array-dimension)
  (:policy :fast-safe)
  (:variant vm:array-dimensions-offset vm:other-pointer-type))

(define-vop (%set-array-dimension word-index-set)
  (:translate lisp::%set-array-dimension)
  (:policy :fast-safe)
  (:variant vm:array-dimensions-offset vm:other-pointer-type))



(defknown lisp::%array-rank (t) fixnum (flushable))

(define-vop (array-rank-vop)
  (:translate lisp::%array-rank)
  (:policy :fast-safe)
  (:args (x :scs (descriptor-reg)))
  (:temporary (:scs (non-descriptor-reg)) temp)
  (:results (res :scs (any-reg descriptor-reg)))
  (:generator 6
    (loadw temp x 0 vm:other-pointer-type)
    (inst sra temp vm:type-bits)
    (inst sub temp (1- vm:array-dimensions-offset))
    (inst sll res temp 2)))



;;;; Bounds checking routine.


(define-vop (check-bound)
  (:translate %check-bound)
  (:policy :fast-safe)
  (:args (array :scs (descriptor-reg))
	 (bound :scs (any-reg descriptor-reg))
	 (index :scs (any-reg descriptor-reg) :target result))
  (:results (result :scs (any-reg descriptor-reg)))
  (:vop-var vop)
  (:save-p :compute-only)
  (:generator 5
    (let ((error (generate-error-code vop invalid-array-index-error
				      array bound index)))
      (inst cmp index bound)
      (inst b :geu error)
      (inst nop)
      (move result index))))



;;;; Accessors/Setters

;;; Variants built on top of word-index-ref, etc.  I.e. those vectors whos
;;; elements are represented in integer registers and are built out of
;;; 8, 16, or 32 bit elements.

(defmacro def-data-vector-frobs (type variant element-type &rest scs)
  `(progn
     (define-vop (,(intern (concatenate 'simple-string
					"DATA-VECTOR-REF/"
					(string type)))
		  ,(intern (concatenate 'simple-string
					(string variant)
					"-REF")))
       (:note "inline array access")
       (:variant vm:vector-data-offset vm:other-pointer-type)
       (:translate data-vector-ref)
       (:arg-types ,type positive-fixnum)
       (:results (value :scs ,scs))
       (:result-types ,element-type))
     (define-vop (,(intern (concatenate 'simple-string
					"DATA-VECTOR-SET/"
					(string type)))
		  ,(intern (concatenate 'simple-string
					(string variant)
					"-SET")))
       (:note "inline array store")
       (:variant vm:vector-data-offset vm:other-pointer-type)
       (:translate data-vector-set)
       (:arg-types ,type positive-fixnum ,element-type)
       (:args (object :scs (descriptor-reg))
	      (index :scs (any-reg zero immediate))
	      (value :scs ,scs))
       (:results (result :scs ,scs))
       (:result-types ,element-type))))

(def-data-vector-frobs simple-string byte-index
  base-char base-char-reg)
(def-data-vector-frobs simple-vector word-index
  * descriptor-reg any-reg)

(def-data-vector-frobs simple-array-unsigned-byte-8 byte-index
  positive-fixnum unsigned-reg)
(def-data-vector-frobs simple-array-unsigned-byte-16 halfword-index
  positive-fixnum unsigned-reg)
(def-data-vector-frobs simple-array-unsigned-byte-32 word-index
  unsigned-num unsigned-reg)


;;; Integer vectors whos elements are smaller than a byte.  I.e. bit, 2-bit,
;;; and 4-bit vectors.
;;; 

(eval-when (compile eval)

(defmacro def-small-data-vector-frobs (type bits)
  (let* ((elements-per-word (floor vm:word-bits bits))
	 (bit-shift (1- (integer-length elements-per-word))))
    `(progn
       (define-vop (,(symbolicate 'data-vector-ref/ type))
	 (:note "inline array access")
	 (:translate data-vector-ref)
	 (:policy :fast-safe)
	 (:args (object :scs (descriptor-reg))
		(index :scs (unsigned-reg)))
	 (:arg-types ,type positive-fixnum)
	 (:results (value :scs (any-reg)))
	 (:result-types positive-fixnum)
	 (:temporary (:scs (non-descriptor-reg) :to (:result 0)) temp result)
	 (:generator 20
	   (inst srl temp index ,bit-shift)
	   (inst sll temp 2)
	   (inst add temp (- (* vm:vector-data-offset vm:word-bytes)
			     vm:other-pointer-type))
	   (inst ld result object temp)
	   (inst and temp index ,(1- elements-per-word))
	   (inst xor temp ,(1- elements-per-word))
	   ,@(unless (= bits 1)
	       `((inst sll temp ,(1- (integer-length bits)))))
	   (inst srl result temp)
	   (inst and result ,(1- (ash 1 bits)))
	   (inst sll value result 2)))
       (define-vop (,(symbolicate 'data-vector-ref-c/ type))
	 (:translate data-vector-ref)
	 (:policy :fast-safe)
	 (:args (object :scs (descriptor-reg)))
	 (:arg-types ,type (:constant index))
	 (:info index)
	 (:results (result :scs (unsigned-reg)))
	 (:result-types positive-fixnum)
	 (:temporary (:scs (non-descriptor-reg)) temp)
	 (:generator 15
	   (multiple-value-bind (word extra) (floor index ,elements-per-word)
	     (setf extra (logxor extra (1- ,elements-per-word)))
	     (let ((offset (- (* (+ word vm:vector-data-offset) vm:word-bytes)
			      vm:other-pointer-type)))
	       (cond ((typep offset '(signed-byte 13))
		      (inst ld result object offset))
		     (t
		      (inst li temp offset)
		      (inst ld result object temp))))
	     (unless (zerop extra)
	       (inst srl result
		     (logxor (* extra ,bits) ,(1- elements-per-word))))
	     (unless (= extra ,(1- elements-per-word))
	       (inst and result ,(1- (ash 1 bits)))))))
       (define-vop (,(symbolicate 'data-vector-set/ type))
	 (:note "inline array store")
	 (:translate data-vector-set)
	 (:policy :fast-safe)
	 (:args (object :scs (descriptor-reg))
		(index :scs (unsigned-reg) :target shift)
		(value :scs (unsigned-reg zero immediate) :target result))
	 (:arg-types ,type positive-fixnum positive-fixnum)
	 (:results (result :scs (unsigned-reg)))
	 (:result-types positive-fixnum)
	 (:temporary (:scs (non-descriptor-reg)) temp old offset)
	 (:temporary (:scs (non-descriptor-reg) :from (:argument 1)) shift)
	 (:generator 25
	   (inst srl offset index ,bit-shift)
	   (inst sll offset 2)
	   (inst add offset (- (* vm:vector-data-offset vm:word-bytes)
			       vm:other-pointer-type))
	   (inst ld old object offset)
	   (inst and shift index ,(1- elements-per-word))
	   (inst xor shift ,(1- elements-per-word))
	   ,@(unless (= bits 1)
	       `((inst sll shift ,(1- (integer-length bits)))))
	   (unless (and (sc-is value immediate)
			(= (tn-value value) ,(1- (ash 1 bits))))
	     (inst li temp ,(1- (ash 1 bits)))
	     (inst sll temp shift)
	     (inst not temp)
	     (inst and old temp))
	   (unless (sc-is value zero)
	     (sc-case value
	       (immediate
		(inst li temp (logand (tn-value value) ,(1- (ash 1 bits)))))
	       (unsigned-reg
		(inst and temp value ,(1- (ash 1 bits)))))
	     (inst sll temp shift)
	     (inst or old temp))
	   (inst st old object offset)
	   (sc-case value
	     (immediate
	      (inst li result (tn-value value)))
	     (t
	      (move result value)))))
       (define-vop (,(symbolicate 'data-vector-set-c/ type))
	 (:translate data-vector-set)
	 (:policy :fast-safe)
	 (:args (object :scs (descriptor-reg))
		(value :scs (unsigned-reg zero immediate) :target result))
	 (:arg-types ,type
		     (:constant index)
		     positive-fixnum)
	 (:info index)
	 (:results (result :scs (unsigned-reg)))
	 (:result-types positive-fixnum)
	 (:temporary (:scs (non-descriptor-reg)) offset-reg temp old)
	 (:generator 20
	   (multiple-value-bind (word extra) (floor index ,elements-per-word)
	     (let ((offset (- (* (+ word vm:vector-data-offset) vm:word-bytes)
			      vm:other-pointer-type)))
	       (cond ((typep offset '(signed-byte 13))
		      (inst ld old object offset))
		     (t
		      (inst li offset-reg offset)
		      (inst ld old object offset-reg)))
	       (unless (and (sc-is value immediate)
			    (= (tn-value value) ,(1- (ash 1 bits))))
		 (cond ((zerop extra)
			(inst sll old ,bits)
			(inst srl old ,bits))
		       (t
			(inst li temp
			      (lognot (ash ,(1- (ash 1 bits))
					   (* (logxor extra
						      ,(1- elements-per-word))
					      ,bits))))
			(inst and old temp))))
	       (sc-case value
		 (zero)
		 (immediate
		  (let ((value (ash (logand (tn-value value)
					    ,(1- (ash 1 bits)))
				    (* (logxor extra
					       ,(1- elements-per-word))
				       ,bits))))
		    (cond ((typep value '(signed-byte 13))
			   (inst or old value))
			  (t
			   (inst li temp value)
			   (inst or old temp)))))
		 (unsigned-reg
		  (inst sll temp value
			(* (logxor extra ,(1- elements-per-word)) ,bits))
		  (inst or old temp)))
	       (if (typep offset '(signed-byte 13))
		   (inst st old object offset)
		   (inst st old object offset-reg)))
	     (sc-case value
	       (immediate
		(inst li result (tn-value value)))
	       (t
		(move result value)))))))))

); eval-when (compile eval)

(def-small-data-vector-frobs simple-bit-vector 1)
(def-small-data-vector-frobs simple-array-unsigned-byte-2 2)
(def-small-data-vector-frobs simple-array-unsigned-byte-4 4)


;;; And the float variants.
;;; 

(define-vop (data-vector-ref/simple-array-single-float)
  (:note "inline array access")
  (:translate data-vector-ref)
  (:policy :fast-safe)
  (:args (object :scs (descriptor-reg))
	 (index :scs (any-reg)))
  (:arg-types simple-array-single-float positive-fixnum)
  (:results (value :scs (single-reg)))
  (:temporary (:scs (non-descriptor-reg)) offset)
  (:result-types single-float)
  (:generator 5
    (inst add offset index (- (* vm:vector-data-offset vm:word-bytes)
			      vm:other-pointer-type))
    (inst ldf value object offset)))


(define-vop (data-vector-set/simple-array-single-float)
  (:note "inline array store")
  (:translate data-vector-set)
  (:policy :fast-safe)
  (:args (object :scs (descriptor-reg))
	 (index :scs (any-reg))
	 (value :scs (single-reg) :target result))
  (:arg-types simple-array-single-float positive-fixnum single-float)
  (:results (result :scs (single-reg)))
  (:result-types single-float)
  (:temporary (:scs (non-descriptor-reg)) offset)
  (:generator 5
    (inst add offset index
	  (- (* vm:vector-data-offset vm:word-bytes)
	     vm:other-pointer-type))
    (inst stf value object offset)
    (unless (location= result value)
      (inst fmovs result value))))

(define-vop (data-vector-ref/simple-array-double-float)
  (:note "inline array access")
  (:translate data-vector-ref)
  (:policy :fast-safe)
  (:args (object :scs (descriptor-reg))
	 (index :scs (any-reg)))
  (:arg-types simple-array-double-float positive-fixnum)
  (:results (value :scs (double-reg)))
  (:result-types double-float)
  (:temporary (:scs (non-descriptor-reg)) offset)
  (:generator 7
    (inst sll offset index 1)
    (inst add offset (- (* vm:vector-data-offset vm:word-bytes)
			vm:other-pointer-type))
    (inst lddf value object offset)))

(define-vop (data-vector-set/simple-array-double-float)
  (:note "inline array store")
  (:translate data-vector-set)
  (:policy :fast-safe)
  (:args (object :scs (descriptor-reg))
	 (index :scs (any-reg))
	 (value :scs (double-reg) :target result))
  (:arg-types simple-array-double-float positive-fixnum double-float)
  (:results (result :scs (double-reg)))
  (:result-types double-float)
  (:temporary (:scs (non-descriptor-reg)) offset)
  (:generator 20
    (inst sll offset index 1)
    (inst add offset (- (* vm:vector-data-offset vm:word-bytes)
			vm:other-pointer-type))
    (inst stdf value object offset)
    (unless (location= result value)
      (inst fmovs result value)
      (inst fmovs-odd result value))))

;;; These VOPs are used for implementing float slots in structures (whose raw
;;; data is an unsigned-32 vector.
;;;
(define-vop (raw-ref-single data-vector-ref/simple-array-single-float)
  (:translate %raw-ref-single)
  (:arg-types simple-array-unsigned-byte-32 positive-fixnum))
;;;
(define-vop (raw-set-single data-vector-set/simple-array-single-float)
  (:translate %raw-set-single)
  (:arg-types simple-array-unsigned-byte-32 positive-fixnum single-float))
;;;
(define-vop (raw-ref-double data-vector-ref/simple-array-double-float)
  (:translate %raw-ref-double)
  (:arg-types simple-array-unsigned-byte-32 positive-fixnum))
;;;
(define-vop (raw-set-double data-vector-set/simple-array-double-float)
  (:translate %raw-set-double)
  (:arg-types simple-array-unsigned-byte-32 positive-fixnum double-float))


;;; These vops are useful for accessing the bits of a vector irrespective of
;;; what type of vector it is.
;;; 

(define-vop (raw-bits word-index-ref)
  (:note "raw-bits VOP")
  (:translate %raw-bits)
  (:results (value :scs (unsigned-reg)))
  (:result-types unsigned-num)
  (:variant 0 vm:other-pointer-type))

(define-vop (set-raw-bits word-index-set)
  (:note "setf raw-bits VOP")
  (:translate %set-raw-bits)
  (:args (object :scs (descriptor-reg))
	 (index :scs (any-reg zero immediate))
	 (value :scs (unsigned-reg)))
  (:arg-types * tagged-num unsigned-num)
  (:results (result :scs (unsigned-reg)))
  (:result-types unsigned-num)
  (:variant 0 vm:other-pointer-type))



;;;; Misc. Array VOPs.


#+nil
(define-vop (vector-word-length)
  (:args (vec :scs (descriptor-reg)))
  (:results (res :scs (any-reg descriptor-reg)))
  (:generator 6
    (loadw res vec clc::g-vector-header-words)
    (inst niuo res res clc::g-vector-words-mask-16)))

(define-vop (get-vector-subtype get-header-data))
(define-vop (set-vector-subtype set-header-data))

