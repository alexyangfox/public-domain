;;; -*- Package: ALPHA; Log: C.Log -*-
;;;
;;; **********************************************************************
;;; This code was written as part of the CMU Common Lisp project at
;;; Carnegie Mellon University, and has been placed in the public domain.
;;;
(ext:file-comment
  "$Header: float.lisp,v 1.2 94/10/31 04:39:51 ram Exp $")
;;;
;;; **********************************************************************
;;;
;;;    This file contains floating point support for the Alpha.
;;;
;;; Written by Rob MacLachlan
;;; Conversion by Sean Hallgren
;;;
(in-package "ALPHA")


;;;; Move functions:

(define-move-function (load-fp-zero 1) (vop x y)
  ((fp-single-zero) (single-reg)
   (fp-double-zero) (double-reg))
  (inst fmove x y))

(define-move-function (load-single 1) (vop x y)
  ((single-stack) (single-reg))
  (inst lds y (* (tn-offset x) word-bytes) (current-nfp-tn vop)))

(define-move-function (store-single 1) (vop x y)
  ((single-reg) (single-stack))
  (inst sts x (* (tn-offset y) word-bytes) (current-nfp-tn vop)))


(define-move-function (load-double 2) (vop x y)
  ((double-stack) (double-reg))
  (let ((nfp (current-nfp-tn vop))
	(offset (* (tn-offset x) word-bytes)))
    (inst ldt y offset nfp)))

(define-move-function (store-double 2) (vop x y)
  ((double-reg) (double-stack))
  (let ((nfp (current-nfp-tn vop))
	(offset (* (tn-offset y) word-bytes)))
    (inst stt x offset nfp)))



;;;; Move VOPs:

(macrolet ((frob (vop sc)
	     `(progn
		(define-vop (,vop)
		  (:args (x :scs (,sc)
			    :target y
			    :load-if (not (location= x y))))
		  (:results (y :scs (,sc)
			       :load-if (not (location= x y))))
		  (:note "float move")
		  (:generator 0
		    (unless (location= y x)
		      (inst fmove x y))))
		(define-move-vop ,vop :move (,sc) (,sc)))))
  (frob single-move single-reg)
  (frob double-move double-reg))


(define-vop (move-from-float)
  (:args (x :to :save))
  (:results (y))
  (:temporary (:scs (non-descriptor-reg)) ndescr)
  (:variant-vars double-p size type data)
  (:note "float to pointer coercion")
  (:generator 13
    (with-fixed-allocation (y ndescr type size)
      (if double-p
	  (inst stt x (- (* data word-bytes) other-pointer-type) y)
	  (inst sts x (- (* data word-bytes) other-pointer-type) y)))))

(macrolet ((frob (name sc &rest args)
	     `(progn
		(define-vop (,name move-from-float)
		  (:args (x :scs (,sc) :to :save))
		  (:results (y :scs (descriptor-reg)))
		  (:variant ,@args))
		(define-move-vop ,name :move (,sc) (descriptor-reg)))))
  (frob move-from-single single-reg
    nil single-float-size single-float-type single-float-value-slot)
  (frob move-from-double double-reg
    t double-float-size double-float-type double-float-value-slot))

(macrolet ((frob (name sc double-p value)
	     `(progn
		(define-vop (,name)
		  (:args (x :scs (descriptor-reg)))
		  (:results (y :scs (,sc)))
		  (:note "pointer to float coercion")
		  (:generator 2
                    ,@(if double-p
			  `((inst ldt y (- (* ,value word-bytes)
					   other-pointer-type)
				  x))
			  `((inst lds y (- (* ,value word-bytes)
					  other-pointer-type)
				 x)))))
		(define-move-vop ,name :move (descriptor-reg) (,sc)))))
  (frob move-to-single single-reg nil single-float-value-slot)
  (frob move-to-double double-reg t double-float-value-slot))


(macrolet ((frob (name sc stack-sc double-p)
	     `(progn
		(define-vop (,name)
		  (:args (x :scs (,sc) :target y)
			 (nfp :scs (any-reg)
			      :load-if (not (sc-is y ,sc))))
		  (:results (y))
		  (:note "float argument move")
		  (:generator ,(if double-p 2 1)
		    (sc-case y
		      (,sc
		       (unless (location= x y)
			 (inst fmove x y)))
		      (,stack-sc
		       (let ((offset (* (tn-offset y) word-bytes)))
			 ,@(if double-p
			       '((inst stt x offset nfp))
			       '((inst sts x offset nfp))))))))
		(define-move-vop ,name :move-argument
		  (,sc descriptor-reg) (,sc)))))
  (frob move-single-float-argument single-reg single-stack nil)
  (frob move-double-float-argument double-reg double-stack t))


(define-move-vop move-argument :move-argument
  (single-reg double-reg) (descriptor-reg))


;;;; Arithmetic VOPs:

(define-vop (float-op)
  (:args (x) (y))
  (:results (r))
  (:policy :fast-safe)
  (:note "inline float arithmetic")
  (:vop-var vop)
  (:save-p :compute-only))

(macrolet ((frob (name sc ptype)
	     `(define-vop (,name float-op)
		(:args (x :scs (,sc))
		       (y :scs (,sc)))
		(:results (r :scs (,sc)))
		(:arg-types ,ptype ,ptype)
		(:result-types ,ptype))))
  (frob single-float-op single-reg single-float)
  (frob double-float-op double-reg double-float))

(macrolet ((frob (op sinst sname scost dinst dname dcost)
	     `(progn
		(define-vop (,sname single-float-op)
		  (:translate ,op)
		  (:variant-cost ,scost)
		  (:generator ,scost
                    (inst ,sinst x y r)))
		(define-vop (,dname double-float-op)
		  (:translate ,op)
		  (:variant-cost ,dcost)
		  (:generator ,dcost
		    (inst ,dinst x y r))))))
  (frob + adds +/single-float 2 addt +/double-float 2)
  (frob - subs -/single-float 2 subt -/double-float 2)
  (frob * muls */single-float 4 mult */double-float 5)
  (frob / divs //single-float 12 divt //double-float 19))

(macrolet ((frob (name inst translate sc type)
	     `(define-vop (,name)
		(:args (x :scs (,sc) :target y))
		(:results (y :scs (,sc)))
		(:translate ,translate)
		(:policy :fast-safe)
		(:arg-types ,type)
		(:result-types ,type)
		(:note "inline float arithmetic")
		(:vop-var vop)
		(:save-p :compute-only)
		(:generator 1
		  (note-this-location vop :internal-error)
		  (inst ,inst x y)))))
  (frob abs/single-float fabs abs single-reg single-float)
  (frob abs/double-float fabs abs double-reg double-float)
  (frob %negate/single-float fneg %negate single-reg single-float)
  (frob %negate/double-float fneg %negate double-reg double-float))


;;;; Comparison:

(define-vop (float-compare)
  (:args (x) (y))
  (:conditional)
  (:info target not-p)
  (:variant-vars eq complement)
  (:temporary (:scs (single-reg)) temp)
  (:policy :fast-safe)
  (:note "inline float comparison")
  (:vop-var vop)
  (:save-p :compute-only)
  (:generator 3
    (note-this-location vop :internal-error)
    (if eq
	(inst cmpteq x y temp)
	(if complement
	    (inst cmptle x y temp)
	    (inst cmptlt x y temp)))
    (if (if complement (not not-p) not-p)
	(inst fbeq temp target)
	(inst fbne temp target))))

(macrolet ((frob (name sc ptype)
	     `(define-vop (,name float-compare)
		(:args (x :scs (,sc))
		       (y :scs (,sc)))
		(:arg-types ,ptype ,ptype))))
  (frob single-float-compare single-reg single-float)
  (frob double-float-compare double-reg double-float))

(macrolet ((frob (translate complement sname dname eq)
	     `(progn
		(define-vop (,sname single-float-compare)
		  (:translate ,translate)
		  (:variant ,eq ,complement))
		(define-vop (,dname double-float-compare)
		  (:translate ,translate)
		  (:variant ,eq ,complement)))))
  (frob < nil </single-float </double-float nil)
  (frob > t >/single-float >/double-float nil)
  (frob = nil =/single-float =/double-float t))


;;;; Conversion:

(macrolet ((frob (name translate inst ld-inst to-sc to-type &optional single)
             `(define-vop (,name)
                (:args (x :scs (signed-reg) :target temp
                          :load-if (not (sc-is x signed-stack))))
                (:temporary (:scs (single-stack)) temp)
                (:results (y :scs (,to-sc)))
                (:arg-types signed-num)
                (:result-types ,to-type)
                (:policy :fast-safe)
                (:note "inline float coercion")
                (:translate ,translate)
                (:vop-var vop)
                (:save-p :compute-only)
                (:generator 5
                  (let ((stack-tn
                         (sc-case x
                           (signed-reg
                            (inst stl x
                                  (* (tn-offset temp) vm:word-bytes)
				  (current-nfp-tn vop))
                            temp)
                           (signed-stack
                            x))))
                    (inst ,ld-inst y
			  (* (tn-offset stack-tn) vm:word-bytes)
			  (current-nfp-tn vop))
                    (note-this-location vop :internal-error)
		    ,@(when single
			`((inst cvtlq y y)))
                    (inst ,inst y y))))))
  (frob %single-float/signed %single-float cvtqs lds single-reg single-float t)
  (frob %double-float/signed %double-float cvtqt lds double-reg double-float t))

(macrolet ((frob (name translate inst from-sc from-type to-sc to-type)
             `(define-vop (,name)
                (:args (x :scs (,from-sc)))
                (:results (y :scs (,to-sc)))
                (:arg-types ,from-type)
                (:result-types ,to-type)
                (:policy :fast-safe)
                (:note "inline float coercion")
                (:translate ,translate)
                (:vop-var vop)
                (:save-p :compute-only)
                (:generator 2
                  (note-this-location vop :internal-error)
		  (inst ,inst x y)))))
  (frob %single-float/double-float %single-float cvtts
    double-reg double-float single-reg single-float)
  (frob %double-float/single-float %double-float fmove
    single-reg single-float double-reg double-float))

(macrolet ((frob (trans from-sc from-type inst &optional single)
             `(define-vop (,(symbolicate trans "/" from-type))
                (:args (x :scs (,from-sc) :target temp))
                (:temporary (:from (:argument 0) :sc single-reg) temp)
                (:temporary (:scs (signed-stack)) stack-temp)
                (:results (y :scs (signed-reg)
                             :load-if (not (sc-is y signed-stack))))
                (:arg-types ,from-type)
                (:result-types signed-num)
                (:translate ,trans)
                (:policy :fast-safe)
                (:note "inline float truncate")
                (:vop-var vop)
                (:save-p :compute-only)
                (:generator 5
                  (note-this-location vop :internal-error)
                  (inst ,inst x temp)
                  (sc-case y
                    (signed-stack
                     (inst stt temp
                           (* (tn-offset y) vm:word-bytes)
			    (current-nfp-tn vop)))
                    (signed-reg
                     (inst stt temp
			   (* (tn-offset stack-temp) vm:word-bytes)
			   (current-nfp-tn vop))
                     (inst ldq y
			   (* (tn-offset stack-temp) vm:word-bytes)
			   (current-nfp-tn vop))))))))
  (frob %unary-truncate single-reg single-float cvttq/c t)
  (frob %unary-truncate double-reg double-float cvttq/c)
  (frob %unary-round single-reg single-float cvttq t)
  (frob %unary-round double-reg double-float cvttq))

(define-vop (make-single-float)
  (:args (bits :scs (signed-reg) :target res
	       :load-if (not (sc-is bits signed-stack))))
  (:results (res :scs (single-reg)
		 :load-if (not (sc-is res single-stack))))
  (:temporary (:scs (signed-reg) :from (:argument 0) :to (:result 0)) temp)
  (:temporary (:scs (signed-stack)) stack-temp)
  (:arg-types signed-num)
  (:result-types single-float)
  (:translate make-single-float)
  (:policy :fast-safe)
  (:vop-var vop)
  (:generator 4
    (sc-case bits
      (signed-reg
       (sc-case res
	 (single-reg
	  (inst stl bits
		(* (tn-offset stack-temp) vm:word-bytes)
		(current-nfp-tn vop))
	  (inst lds res
		(* (tn-offset stack-temp) vm:word-bytes)
		(current-nfp-tn vop)))
	 (single-stack
	  (inst stl bits
		(* (tn-offset res) vm:word-bytes)
		(current-nfp-tn vop)))))
      (signed-stack
       (sc-case res
	 (single-reg
	  (inst lds res
		(* (tn-offset bits) vm:word-bytes)
		(current-nfp-tn vop)))
	 (single-stack
	  (unless (location= bits res)
	    (inst ldl temp
		  (* (tn-offset bits) vm:word-bytes)
		  (current-nfp-tn vop))
	    (inst stl temp
		  (* (tn-offset res) vm:word-bytes)
		  (current-nfp-tn vop)))))))))

(define-vop (make-double-float)
  (:args (hi-bits :scs (signed-reg))
	 (lo-bits :scs (unsigned-reg)))
  (:results (res :scs (double-reg)
		 :load-if (not (sc-is res double-stack))))
  (:temporary (:scs (double-stack)) temp)
  (:arg-types signed-num unsigned-num)
  (:result-types double-float)
  (:translate make-double-float)
  (:policy :fast-safe)
  (:vop-var vop)
  (:generator 2
    (let ((stack-tn (sc-case res
		      (double-stack res)
		      (double-reg temp))))
      (inst stl hi-bits
	    (* (1+ (tn-offset stack-tn)) vm:word-bytes)
	    (current-nfp-tn vop))
      (inst stl lo-bits
	    (* (tn-offset stack-tn) vm:word-bytes)
	    (current-nfp-tn vop)))
    (when (sc-is res double-reg)
      (inst ldt res
	    (* (tn-offset temp) vm:word-bytes)
	    (current-nfp-tn vop)))))

(define-vop (single-float-bits)
  (:args (float :scs (single-reg descriptor-reg)
		:load-if (not (sc-is float single-stack))))
  (:results (bits :scs (signed-reg)
		  :load-if (or (sc-is float descriptor-reg single-stack)
			       (not (sc-is bits signed-stack)))))
  (:temporary (:scs (signed-stack)) stack-temp)
  (:arg-types single-float)
  (:result-types signed-num)
  (:translate single-float-bits)
  (:policy :fast-safe)
  (:vop-var vop)
  (:generator 4
    (sc-case bits
      (signed-reg
       (sc-case float
	 (single-reg
	  (inst sts float
		(* (tn-offset stack-temp) vm:word-bytes)
		(current-nfp-tn vop))
	  (inst ldl bits
		(* (tn-offset stack-temp) vm:word-bytes)
		(current-nfp-tn vop)))
	 (single-stack
	  (inst ldl bits
		(* (tn-offset float) vm:word-bytes)
		(current-nfp-tn vop)))
	 (descriptor-reg
	  (loadw bits float vm:single-float-value-slot vm:other-pointer-type))))
      (signed-stack
       (sc-case float
	 (single-reg
	  (inst sts float
		(* (tn-offset bits) vm:word-bytes)
		(current-nfp-tn vop))))))))

(define-vop (double-float-high-bits)
  (:args (float :scs (double-reg descriptor-reg)
		:load-if (not (sc-is float double-stack))))
  (:results (hi-bits :scs (signed-reg)
		     :load-if (or (sc-is float descriptor-reg double-stack)
				  (not (sc-is hi-bits signed-stack)))))
  (:temporary (:scs (double-stack)) stack-temp)
  (:arg-types double-float)
  (:result-types signed-num)
  (:translate double-float-high-bits)
  (:policy :fast-safe)
  (:vop-var vop)
  (:generator 5
    (sc-case float
      (double-reg
        (inst stt float
	      (* (tn-offset stack-temp) vm:word-bytes)
	      (current-nfp-tn vop))
        (inst ldl hi-bits
	      (* (1+ (tn-offset stack-temp)) vm:word-bytes)
	      (current-nfp-tn vop)))
      (double-stack
        (inst ldl hi-bits
	      (* (1+ (tn-offset float)) vm:word-bytes)
	      (current-nfp-tn vop)))
      (descriptor-reg
        (loadw hi-bits float (1+ vm:double-float-value-slot)
	       vm:other-pointer-type)))))

(define-vop (double-float-low-bits)
  (:args (float :scs (double-reg descriptor-reg)
		:load-if (not (sc-is float double-stack))))
  (:results (lo-bits :scs (unsigned-reg)
		     :load-if (or (sc-is float descriptor-reg double-stack)
				  (not (sc-is lo-bits unsigned-stack)))))
  (:temporary (:scs (double-stack)) stack-temp)
  (:arg-types double-float)
  (:result-types unsigned-num)
  (:translate double-float-low-bits)
  (:policy :fast-safe)
  (:vop-var vop)
  (:generator 5
    (sc-case float
      (double-reg
        (inst stt float
	      (* (tn-offset stack-temp) vm:word-bytes)
	      (current-nfp-tn vop))
	(inst ldl lo-bits
	      (* (tn-offset stack-temp) vm:word-bytes)
	      (current-nfp-tn vop)))
      (double-stack
       (inst ldl lo-bits
	     (* (tn-offset float) vm:word-bytes)
	     (current-nfp-tn vop)))
      (descriptor-reg
       (loadw lo-bits float vm:double-float-value-slot
	      vm:other-pointer-type)))
    (inst mskll lo-bits 4 lo-bits)))


;;;; Float mode hackery:

(deftype float-modes () '(unsigned-byte 24))
(defknown floating-point-modes () float-modes (flushable))
(defknown ((setf floating-point-modes)) (float-modes)
  float-modes)

(define-vop (floating-point-modes)
  (:results (res :scs (unsigned-reg)))
  (:result-types unsigned-num)
  (:translate floating-point-modes)
  (:policy :fast-safe)
  (:vop-var vop)
  (:temporary (:sc unsigned-stack) temp)
  (:temporary (:sc single-reg) temp1)
  (:generator 3
    (let ((nfp (current-nfp-tn vop)))
      (inst mf_fpcr temp1 temp1 temp1)
      (inst sts temp1 (* word-bytes (tn-offset temp)) nfp)
      (loadw res nfp (tn-offset temp)))))

(define-vop (set-floating-point-modes)
  (:args (new :scs (unsigned-reg) :target res))
  (:results (res :scs (unsigned-reg)))
  (:arg-types unsigned-num)
  (:result-types unsigned-num)
  (:translate (setf floating-point-modes))
  (:policy :fast-safe)
  (:temporary (:sc unsigned-stack) temp)
  (:temporary (:sc single-reg) temp1)
  (:vop-var vop)
  (:generator 3
    (let ((nfp (current-nfp-tn vop)))
      (storew new nfp (tn-offset temp))
      (inst lds temp1 (* word-bytes (tn-offset temp)) nfp)
      (inst mt_fpcr temp1 temp1 temp1)
      (move res new))))
