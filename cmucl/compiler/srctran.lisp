;;; -*- Package: C; Log: C.Log -*-
;;;
;;; **********************************************************************
;;; This code was written as part of the CMU Common Lisp project at
;;; Carnegie Mellon University, and has been placed in the public domain.
;;;
(ext:file-comment
  "$Header: srctran.lisp,v 1.45 94/10/31 04:27:28 ram Exp $")
;;;
;;; **********************************************************************
;;;
;;;    This file contains macro-like source transformations which convert
;;; uses of certain functions into the canonical form desired within the
;;; compiler.  ### and other IR1 transforms and stuff.  Some code adapted from
;;; CLC, written by Wholey and Fahlman.
;;;
;;; Written by Rob MacLachlan
;;;
(in-package "C")

;;; Source transform for Not, Null  --  Internal
;;;
;;;    Convert into an IF so that IF optimizations will eliminate redundant
;;; negations.
;;;
(def-source-transform not (x) `(if ,x nil t))
(def-source-transform null (x) `(if ,x nil t))

;;; Source transform for Endp  --  Internal
;;;
;;;    Endp is just NULL with a List assertion.
;;;
(def-source-transform endp (x) `(null (the list ,x)))

;;; We turn Identity into Prog1 so that it is obvious that it just returns the
;;; first value of its argument.  Ditto for Values with one arg.
(def-source-transform identity (x) `(prog1 ,x))
(def-source-transform values (x) `(prog1 ,x))

;;; CONSTANTLY source transform  --  Internal
;;;
;;;    Bind the values and make a closure that returns them.
;;;
(def-source-transform constantly (value &rest values)
  (let ((temps (loop repeat (1+ (length values))
		     collect (gensym)))
	(dum (gensym)))
    `(let ,(loop for temp in temps and
	         value in (list* value values)
	         collect `(,temp ,value))
       #'(lambda (&rest ,dum)
	   (declare (ignore ,dum))
	   (values ,@temps)))))


;;; COMPLEMENT IR1 transform  --  Internal
;;;
;;;    If the function has a known number of arguments, then return a lambda
;;; with the appropriate fixed number of args.  If the destination is a
;;; FUNCALL, then do the &REST APPLY thing, and let MV optimization figure
;;; things out.
;;;
(deftransform complement ((fun) * * :node node :when :both)
  "open code"
  (multiple-value-bind (min max)
		       (function-type-nargs (continuation-type fun))
    (cond
     ((and min (eql min max))
      (let ((dums (loop repeat min collect (gensym))))
	`#'(lambda ,dums (not (funcall fun ,@dums)))))
     ((let* ((cont (node-cont node))
	     (dest (continuation-dest cont)))
	(and (combination-p dest)
	     (eq (combination-fun dest) cont)))
      '#'(lambda (&rest args)
	   (not (apply fun args))))
     (t
      (give-up "Function doesn't have fixed argument count.")))))


;;;; List hackery:

;;;
;;; Translate CxxR into car/cdr combos.

(defun source-transform-cxr (form)
  (if (or (byte-compiling) (/= (length form) 2))
      (values nil t)
      (let ((name (symbol-name (car form))))
	(do ((i (- (length name) 2) (1- i))
	     (res (cadr form)
		  `(,(ecase (char name i)
		       (#\A 'car)
		       (#\D 'cdr))
		    ,res)))
	    ((zerop i) res)))))

(do ((i 2 (1+ i))
     (b '(1 0) (cons i b)))
    ((= i 5))
  (dotimes (j (ash 1 i))
    (setf (info function source-transform
		(intern (format nil "C~{~:[A~;D~]~}R"
				(mapcar #'(lambda (x) (logbitp x j)) b))))
	  #'source-transform-cxr)))

;;;
;;; Turn First..Fourth and Rest into the obvious synonym, assuming whatever is
;;; right for them is right for us.  Fifth..Tenth turn into Nth, which can be
;;; expanded into a car/cdr later on if policy favors it.
(def-source-transform first (x) `(car ,x))
(def-source-transform rest (x) `(cdr ,x))
(def-source-transform second (x) `(cadr ,x))
(def-source-transform third (x) `(caddr ,x))
(def-source-transform fourth (x) `(cadddr ,x))
(def-source-transform fifth (x) `(nth 4 ,x))
(def-source-transform sixth (x) `(nth 5 ,x))
(def-source-transform seventh (x) `(nth 6 ,x))
(def-source-transform eighth (x) `(nth 7 ,x))
(def-source-transform ninth (x) `(nth 8 ,x))
(def-source-transform tenth (x) `(nth 9 ,x))


;;;
;;; Translate RPLACx to LET and SETF.
(def-source-transform rplaca (x y)
  (once-only ((n-x x))
    `(progn
       (setf (car ,n-x) ,y)
       ,n-x)))
;;;
(def-source-transform rplacd (x y)
  (once-only ((n-x x))
    `(progn
       (setf (cdr ,n-x) ,y)
       ,n-x)))


(def-source-transform nth (n l) `(car (nthcdr ,n ,l)))
  
(defvar *default-nthcdr-open-code-limit* 6)
(defvar *extreme-nthcdr-open-code-limit* 20)

(deftransform nthcdr ((n l) (unsigned-byte t) * :node node)
  "convert NTHCDR to CAxxR"
  (unless (constant-continuation-p n) (give-up))
  (let ((n (continuation-value n)))
    (when (> n
	     (if (policy node (= speed 3) (= space 0))
		 *extreme-nthcdr-open-code-limit*
		 *default-nthcdr-open-code-limit*))
      (give-up))

    (labels ((frob (n)
	       (if (zerop n)
		   'l
		   `(cdr ,(frob (1- n))))))
      (frob n))))


;;;; ARITHMETIC and NUMEROLOGY.

(def-source-transform plusp (x) `(> ,x 0))
(def-source-transform minusp (x) `(< ,x 0))
(def-source-transform zerop (x) `(= ,x 0))

(def-source-transform 1+ (x) `(+ ,x 1))
(def-source-transform 1- (x) `(- ,x 1))

(def-source-transform oddp (x) `(not (zerop (logand ,x 1))))
(def-source-transform evenp (x) `(zerop (logand ,x 1)))

;;; Note that all the integer division functions are available for inline
;;; expansion.

(macrolet ((frob (fun)
	     `(def-source-transform ,fun (x &optional (y nil y-p))
		(declare (ignore y))
		(if y-p
		    (values nil t)
		    `(,',fun ,x 1)))))
  (frob truncate)
  (frob round))

(def-source-transform lognand (x y) `(lognot (logand ,x ,y)))
(def-source-transform lognor (x y) `(lognot (logior ,x ,y)))
(def-source-transform logandc1 (x y) `(logand (lognot ,x) ,y))
(def-source-transform logandc2 (x y) `(logand ,x (lognot ,y)))
(def-source-transform logorc1 (x y) `(logior (lognot ,x) ,y))
(def-source-transform logorc2 (x y) `(logior ,x (lognot ,y)))
(def-source-transform logtest (x y) `(not (zerop (logand ,x ,y))))
(def-source-transform logbitp (index integer)
  `(not (zerop (logand (ash 1 ,index) ,integer))))
(def-source-transform byte (size position) `(cons ,size ,position))
(def-source-transform byte-size (spec) `(car ,spec))
(def-source-transform byte-position (spec) `(cdr ,spec))
(def-source-transform ldb-test (bytespec integer)
  `(not (zerop (mask-field ,bytespec ,integer))))


;;; With the ratio and complex accessors, we pick off the "identity" case, and
;;; use a primitive to handle the cell access case.
;;;
(def-source-transform numerator (num)
  (once-only ((n-num `(the rational ,num)))
    `(if (ratiop ,n-num)
	 (%numerator ,n-num)
	 ,n-num)))
;;;
(def-source-transform denominator (num)
  (once-only ((n-num `(the rational ,num)))
    `(if (ratiop ,n-num)
	 (%denominator ,n-num)
	 1)))
;;;
(def-source-transform realpart (num)
  (once-only ((n-num num))
    `(if (complexp ,n-num)
	 (%realpart ,n-num)
	 ,n-num)))
;;;
(def-source-transform imagpart (num)
  (once-only ((n-num num))
    `(cond ((complexp ,n-num)
	    (%imagpart ,n-num))
	   ((floatp ,n-num)
	    (float 0 ,n-num))
	   (t
	    0))))


;;;; Numeric Derive-Type methods:

;;; Derive-Integer-Type  --  Internal
;;;
;;;    Utility for defining derive-type methods of integer operations.  If the
;;; types of both X and Y are integer types, then we compute a new integer type
;;; with bounds determined Fun when applied to X and Y.  Otherwise, we use
;;; Numeric-Contagion.
;;;
(defun derive-integer-type (x y fun)
  (declare (type continuation x y) (type function fun))
  (let ((x (continuation-type x))
	(y (continuation-type y)))
    (if (and (numeric-type-p x) (numeric-type-p y)
	     (eq (numeric-type-class x) 'integer)
	     (eq (numeric-type-class y) 'integer)
	     (eq (numeric-type-complexp x) :real)
	     (eq (numeric-type-complexp y) :real))
	(multiple-value-bind (low high)
			     (funcall fun x y)
	  (make-numeric-type :class 'integer  :complexp :real
			     :low low  :high high))
	(numeric-contagion x y))))


(defoptimizer (+ derive-type) ((x y))
  (derive-integer-type
   x y
   #'(lambda (x y)
       (flet ((frob (x y)
		(if (and x y)
		    (+ x y)
		    nil)))
	 (values (frob (numeric-type-low x) (numeric-type-low y))
		 (frob (numeric-type-high x) (numeric-type-high y)))))))

(defoptimizer (- derive-type) ((x y))
  (derive-integer-type
   x y
   #'(lambda (x y)
       (flet ((frob (x y)
		(if (and x y)
		    (- x y)
		    nil)))
	 (values (frob (numeric-type-low x) (numeric-type-high y))
		 (frob (numeric-type-high x) (numeric-type-low y)))))))

(defoptimizer (* derive-type) ((x y))
  (derive-integer-type
   x y
   #'(lambda (x y)
       (let ((x-low (numeric-type-low x))
	     (x-high (numeric-type-high x))
	     (y-low (numeric-type-low y))
	     (y-high (numeric-type-high y)))
	 (cond ((not (and x-low y-low))
		(values nil nil))
	       ((or (minusp x-low) (minusp y-low))
		(if (and x-high y-high)
		    (let ((max (* (max (abs x-low) (abs x-high))
				  (max (abs y-low) (abs y-high)))))
		      (values (- max) max))
		    (values nil nil)))
	       (t
		(values (* x-low y-low)
			(if (and x-high y-high)
			    (* x-high y-high)
			    nil))))))))

(defoptimizer (/ derive-type) ((x y))
  (numeric-contagion (continuation-type x) (continuation-type y)))


(defoptimizer (ash derive-type) ((n shift))
  (or (let ((n-type (continuation-type n)))
	(when (numeric-type-p n-type)
	  (let ((n-low (numeric-type-low n-type))
		(n-high (numeric-type-high n-type)))
	    (if (constant-continuation-p shift)
		(let ((shift (continuation-value shift)))
		  (make-numeric-type :class 'integer  :complexp :real
				     :low (when n-low
					    #+new-compiler
					    (ash n-low shift)
					    ;; ### fuckin' bignum bug.
					    #-new-compiler
					    (* n-low (ash 1 shift)))
				     :high (when n-high (ash n-high shift))))
		(let ((s-type (continuation-type shift)))
		  (when (numeric-type-p s-type)
		    (let ((s-low (numeric-type-low s-type))
			  (s-high (numeric-type-high s-type)))
		      (if (and s-low s-high (<= s-low 32) (<= s-high 32))
			  (make-numeric-type :class 'integer  :complexp :real
					     :low (when n-low
						    (min (ash n-low s-high)
							 (ash n-low s-low)))
					     :high (when n-high
						     (max (ash n-high s-high)
							  (ash n-high s-low))))
			  (make-numeric-type :class 'integer
					     :complexp :real)))))))))
      *universal-type*))


(macrolet ((frob (fun)
	     `#'(lambda (type type2)
		  (declare (ignore type2))
		  (let ((lo (numeric-type-low type))
			(hi (numeric-type-high type)))
		    (values (if hi (,fun hi) nil) (if lo (,fun lo) nil))))))

  (defoptimizer (%negate derive-type) ((num))
    (derive-integer-type num num (frob -)))

  (defoptimizer (lognot derive-type) ((int))
    (derive-integer-type int int (frob lognot))))


(defoptimizer (abs derive-type) ((num))
  (let ((type (continuation-type num)))
    (if (and (numeric-type-p type)
	     (eq (numeric-type-class type) 'integer)
	     (eq (numeric-type-complexp type) :real))
	(let ((lo (numeric-type-low type))
	      (hi (numeric-type-high type)))
	  (make-numeric-type :class 'integer :complexp :real
			     :low (cond ((and hi (minusp hi))
					 (abs hi))
					(lo
					 (max 0 lo))
					(t
					 0))
			     :high (if (and hi lo)
				       (max (abs hi) (abs lo))
				       nil)))
	(numeric-contagion type type))))


(defoptimizer (truncate derive-type) ((number divisor))
  (let ((number-type (continuation-type number))
	(divisor-type (continuation-type divisor))
	(integer-type (specifier-type 'integer)))
    (if (and (numeric-type-p number-type)
	     (csubtypep number-type integer-type)
	     (numeric-type-p divisor-type)
	     (csubtypep divisor-type integer-type))
	(let ((number-low (numeric-type-low number-type))
	      (number-high (numeric-type-high number-type))
	      (divisor-low (numeric-type-low divisor-type))
	      (divisor-high (numeric-type-high divisor-type)))
	  (values-specifier-type
	   `(values ,(integer-truncate-derive-type number-low number-high
						   divisor-low divisor-high)
		    ,(integer-rem-derive-type number-low number-high
					      divisor-low divisor-high))))
	*universal-type*)))

;;; NUMERIC-RANGE-INFO  --  internal.
;;;
;;; Derive useful information about the range.  Returns three values:
;;; - '+ if its positive, '- negative, or nil if it overlaps 0.
;;; - The abs of the minimal value (i.e. closest to 0) in the range.
;;; - The abs of the maximal value if there is one, or nil if it is unbounded.
;;; 
(defun numeric-range-info (low high)
  (cond ((and low (not (minusp low)))
	 (values '+ low high))
	((and high (not (plusp high)))
	 (values '- (- high) (if low (- low) nil)))
	(t
	 (values nil 0 (and low high (max (- low) high))))))

;;; INTEGER-TRUNCATE-DERIVE-TYPE -- internal
;;; 
(defun integer-truncate-derive-type
       (number-low number-high divisor-low divisor-high)
  ;; The result cannot be larger in magnitude than the number, but the sign
  ;; might change.  If we can determine the sign of either the number or
  ;; the divisor, we can eliminate some of the cases.
  (multiple-value-bind
      (number-sign number-min number-max)
      (numeric-range-info number-low number-high)
    (multiple-value-bind
	(divisor-sign divisor-min divisor-max)
	(numeric-range-info divisor-low divisor-high)
      (when (and divisor-max (zerop divisor-max))
	;; We've got a problem: guarenteed division by zero.
	(return-from integer-truncate-derive-type t))
      (when (zerop divisor-min)
	;; We'll assume that they arn't going to divide by zero.
	(incf divisor-min))
      (cond ((and number-sign divisor-sign)
	     ;; We know the sign of both.
	     (if (eq number-sign divisor-sign)
		 ;; Same sign, so the result will be positive.
		 `(integer ,(if divisor-max
				(truncate number-min divisor-max)
				0)
			   ,(if number-max
				(truncate number-max divisor-min)
				'*))
		 ;; Different signs, the result will be negative.
		 `(integer ,(if number-max
				(- (truncate number-max divisor-min))
				'*)
			   ,(if divisor-max
				(- (truncate number-min divisor-max))
				0))))
	    ((eq divisor-sign '+)
	     ;; The divisor is positive.  Therefore, the number will just
	     ;; become closer to zero.
	     `(integer ,(if number-low
			    (truncate number-low divisor-min)
			    '*)
		       ,(if number-high
			    (truncate number-high divisor-min)
			    '*)))
	    ((eq divisor-sign '-)
	     ;; The divisor is negative.  Therefore, the absolute value of
	     ;; the number will become closer to zero, but the sign will also
	     ;; change.
	     `(integer ,(if number-high
			    (- (truncate number-high divisor-min))
			    '*)
		       ,(if number-low
			    (- (truncate number-low divisor-min))
			    '*)))
	    ;; The divisor could be either positive or negative.
	    (number-max
	     ;; The number we are dividing has a bound.  Divide that by the
	     ;; smallest posible divisor.
	     (let ((bound (truncate number-max divisor-min)))
	       `(integer ,(- bound) ,bound)))
	    (t
	     ;; The number we are dividing is unbounded, so we can't tell
	     ;; anything about the result.
	     'integer)))))
	  
(defun integer-rem-derive-type
       (number-low number-high divisor-low divisor-high)
  (if (and divisor-low divisor-high)
      ;; We know the range of the divisor, and the remainder must be smaller
      ;; than the divisor.  We can tell the sign of the remainer if we know
      ;; the sign of the number.
      (let ((divisor-max (1- (max (abs divisor-low) (abs divisor-high)))))
	`(integer ,(if (or (null number-low)
			   (minusp number-low))
		       (- divisor-max)
		       0)
		  ,(if (or (null number-high)
			   (plusp number-high))
		       divisor-max
		       0)))
      ;; The divisor is potentially either very positive or very negative.
      ;; Therefore, the remainer is unbounded, but we might be able to tell
      ;; something about the sign from the number.
      `(integer ,(if (and number-low (not (minusp number-low)))
		     ;; The number we are dividing is positive.  Therefore,
		     ;; the remainder must be positive.
		     0
		     '*)
		,(if (and number-high (not (plusp number-high)))
		     ;; The number we are dividing is negative.  Therefore,
		     ;; the remainder must be negative.
		     0
		     '*))))

(defoptimizer (random derive-type) ((bound &optional state))
  (let ((type (continuation-type bound)))
    (when (numeric-type-p type)
      (let ((class (numeric-type-class type))
	    (high (numeric-type-high type))
	    (format (numeric-type-format type)))
	(make-numeric-type
	 :class class
	 :format format
	 :low (coerce 0 (or format class 'real))
	 :high (cond ((not high) nil)
		     ((eq class 'integer) (max (1- high) 0))
		     ((or (consp high) (zerop high)) high)
		     (t `(,high))))))))


;;;; Logical derive-type methods:


;;; Integer-Type-Length -- Internal
;;;
;;; Return the maximum number of bits an integer of the supplied type can take
;;; up, or NIL if it is unbounded.  The second (third) value is T if the
;;; integer can be positive (negative) and NIL if not.  Zero counts as
;;; positive.
;;;
(defun integer-type-length (type)
  (if (numeric-type-p type)
      (let ((min (numeric-type-low type))
	    (max (numeric-type-high type)))
	(values (and min max (max (integer-length min) (integer-length max)))
		(or (null max) (not (minusp max)))
		(or (null min) (minusp min))))
      (values nil t t)))

(defoptimizer (logand derive-type) ((x y))
  (multiple-value-bind
      (x-len x-pos x-neg)
      (integer-type-length (continuation-type x))
    (declare (ignore x-pos))
    (multiple-value-bind
	(y-len y-pos y-neg)
	(integer-type-length (continuation-type y))
      (declare (ignore y-pos))
      (if (not x-neg)
	  ;; X must be positive.
	  (if (not y-neg)
	      ;; The must both be positive.
	      (cond ((or (null x-len) (null y-len))
		     (specifier-type 'unsigned-byte))
		    ((or (zerop x-len) (zerop y-len))
		     (specifier-type '(integer 0 0)))
		    (t
		     (specifier-type `(unsigned-byte ,(min x-len y-len)))))
	      ;; X is positive, but Y might be negative.
	      (cond ((null x-len)
		     (specifier-type 'unsigned-byte))
		    ((zerop x-len)
		     (specifier-type '(integer 0 0)))
		    (t
		     (specifier-type `(unsigned-byte ,x-len)))))
	  ;; X might be negative.
	  (if (not y-neg)
	      ;; Y must be positive.
	      (cond ((null y-len)
		     (specifier-type 'unsigned-byte))
		    ((zerop y-len)
		     (specifier-type '(integer 0 0)))
		    (t
		     (specifier-type
		      `(unsigned-byte ,y-len))))
	      ;; Either might be negative.
	      (if (and x-len y-len)
		  ;; The result is bounded.
		  (specifier-type `(signed-byte ,(1+ (max x-len y-len))))
		  ;; We can't tell squat about the result.
		  (specifier-type 'integer)))))))

(defoptimizer (logior derive-type) ((x y))
  (multiple-value-bind
      (x-len x-pos x-neg)
      (integer-type-length (continuation-type x))
    (multiple-value-bind
	(y-len y-pos y-neg)
	(integer-type-length (continuation-type y))
      (cond
       ((and (not x-neg) (not y-neg))
	;; Both are positive.
	(specifier-type `(unsigned-byte ,(if (and x-len y-len)
					     (max x-len y-len)
					     '*))))
       ((not x-pos)
	;; X must be negative.
	(if (not y-pos)
	    ;; Both are negative.  The result is going to be negative and be
	    ;; the same length or shorter than the smaller.
	    (if (and x-len y-len)
		;; It's bounded.
		(specifier-type `(integer ,(ash -1 (min x-len y-len)) -1))
		;; It's unbounded.
		(specifier-type '(integer * -1)))
	    ;; X is negative, but we don't know about Y.  The result will be
	    ;; negative, but no more negative than X.
	    (specifier-type
	     `(integer ,(or (numeric-type-low (continuation-type x)) '*)
		       -1))))
       (t
	;; X might be either positive or negative.
	(if (not y-pos)
	    ;; But Y is negative.  The result will be negative.
	    (specifier-type
	     `(integer ,(or (numeric-type-low (continuation-type y)) '*)
		       -1))
	    ;; We don't know squat about either.  It won't get any bigger.
	    (if (and x-len y-len)
		;; Bounded.
		(specifier-type `(signed-byte ,(1+ (max x-len y-len))))
		;; Unbounded.
		(specifier-type 'integer))))))))

(defoptimizer (logxor derive-type) ((x y))
  (multiple-value-bind
      (x-len x-pos x-neg)
      (integer-type-length (continuation-type x))
    (multiple-value-bind
	(y-len y-pos y-neg)
	(integer-type-length (continuation-type y))
      (cond
       ((or (and (not x-neg) (not y-neg))
	    (and (not x-pos) (not y-pos)))
	;; Either both are negative or both are positive.  The result will be
	;; positive, and as long as the longer.
	(specifier-type `(unsigned-byte ,(if (and x-len y-len)
					     (max x-len y-len)
					     '*))))
       ((or (and (not x-pos) (not y-neg))
	    (and (not y-neg) (not y-pos)))
	;; Either X is negative and Y is positive of vice-verca.  The result
	;; will be negative.
	(specifier-type `(integer ,(if (and x-len y-len)
				       (ash -1 (max x-len y-len))
				       '*)
				  -1)))
       ;; We can't tell what the sign of the result is going to be.  All we
       ;; know is that we don't create new bits.
       ((and x-len y-len)
	(specifier-type `(signed-byte ,(1+ (max x-len y-len)))))
       (t
	(specifier-type 'integer))))))



;;;; Miscellaneous derive-type methods:


(defoptimizer (code-char derive-type) ((code))
  (specifier-type 'base-char))


(defoptimizer (values derive-type) ((&rest values))
  (values-specifier-type
   `(values ,@(mapcar #'(lambda (x)
			  (type-specifier (continuation-type x)))
		      values))))



;;;; Byte operations:
;;;
;;;    We try to turn byte operations into simple logical operations.  First,
;;; we convert byte specifiers into separate size and position arguments passed
;;; to internal %FOO functions.  We then attempt to transform the %FOO
;;; functions into boolean operations when the size and position are constant
;;; and the operands are fixnums.


;;; With-Byte-Specifier  --  Internal
;;;
;;;    Evaluate body with Size-Var and Pos-Var bound to expressions that
;;; evaluate to the Size and Position of the byte-specifier form Spec.  We may
;;; wrap a let around the result of the body to bind some variables.
;;;
;;;    If the spec is a Byte form, then bind the vars to the subforms.
;;; otherwise, evaluate Spec and use the Byte-Size and Byte-Position.  The goal
;;; of this transformation is to avoid consing up byte specifiers and then
;;; immediately throwing them away.
;;;
(defmacro with-byte-specifier ((size-var pos-var spec) &body body)
  (once-only ((spec `(macroexpand ,spec))
	      (temp '(gensym)))
    `(if (and (consp ,spec)
	      (eq (car ,spec) 'byte)
	      (= (length ,spec) 3))
	 (let ((,size-var (second ,spec))
	       (,pos-var (third ,spec)))
	   ,@body)
	 (let ((,size-var `(byte-size ,,temp))
	       (,pos-var `(byte-position ,,temp)))
	   `(let ((,,temp ,,spec))
	      ,,@body)))))

(def-source-transform ldb (spec int)
  (with-byte-specifier (size pos spec)
    `(%ldb ,size ,pos ,int)))

(def-source-transform dpb (newbyte spec int)
  (with-byte-specifier (size pos spec)
    `(%dpb ,newbyte ,size ,pos ,int)))

(def-source-transform mask-field (spec int)
  (with-byte-specifier (size pos spec)
    `(%mask-field ,size ,pos ,int)))

(def-source-transform deposit-field (newbyte spec int)
  (with-byte-specifier (size pos spec)
    `(%deposit-field ,newbyte ,size ,pos ,int)))


(defoptimizer (%ldb derive-type) ((size posn num))
  (let ((size (continuation-type size)))
    (if (and (numeric-type-p size)
	     (csubtypep size (specifier-type 'integer)))
	(let ((size-high (numeric-type-high size)))
	  (if (and size-high (<= size-high vm:word-bits))
	      (specifier-type `(unsigned-byte ,size-high))
	      (specifier-type 'unsigned-byte)))
	*universal-type*)))

(defoptimizer (%mask-field derive-type) ((size posn num))
  (let ((size (continuation-type size))
	(posn (continuation-type posn)))
    (if (and (numeric-type-p size)
	     (csubtypep size (specifier-type 'integer))
	     (numeric-type-p posn)
	     (csubtypep posn (specifier-type 'integer)))
	(let ((size-high (numeric-type-high size))
	      (posn-high (numeric-type-high posn)))
	  (if (and size-high posn-high
		   (<= (+ size-high posn-high) vm:word-bits))
	      (specifier-type `(unsigned-byte ,(+ size-high posn-high)))
	      (specifier-type 'unsigned-byte)))
	*universal-type*)))

(defoptimizer (%dpb derive-type) ((newbyte size posn int))
  (let ((size (continuation-type size))
	(posn (continuation-type posn))
	(int (continuation-type int)))
    (if (and (numeric-type-p size)
	     (csubtypep size (specifier-type 'integer))
	     (numeric-type-p posn)
	     (csubtypep posn (specifier-type 'integer))
	     (numeric-type-p int)
	     (csubtypep int (specifier-type 'integer)))
	(let ((size-high (numeric-type-high size))
	      (posn-high (numeric-type-high posn))
	      (high (numeric-type-high int))
	      (low (numeric-type-low int)))
	  (if (and size-high posn-high high low
		   (<= (+ size-high posn-high) vm:word-bits))
	      (specifier-type
	       (list (if (minusp low) 'signed-byte 'unsigned-byte)
		     (max (integer-length high)
			  (integer-length low)
			  (+ size-high posn-high))))
	      *universal-type*))
	*universal-type*)))

(defoptimizer (%deposit-field derive-type) ((newbyte size posn int))
  (let ((size (continuation-type size))
	(posn (continuation-type posn))
	(int (continuation-type int)))
    (if (and (numeric-type-p size)
	     (csubtypep size (specifier-type 'integer))
	     (numeric-type-p posn)
	     (csubtypep posn (specifier-type 'integer))
	     (numeric-type-p int)
	     (csubtypep int (specifier-type 'integer)))
	(let ((size-high (numeric-type-high size))
	      (posn-high (numeric-type-high posn))
	      (high (numeric-type-high int))
	      (low (numeric-type-low int)))
	  (if (and size-high posn-high high low
		   (<= (+ size-high posn-high) vm:word-bits))
	      (specifier-type
	       (list (if (minusp low) 'signed-byte 'unsigned-byte)
		     (max (integer-length high)
			  (integer-length low)
			  (+ size-high posn-high))))
	      *universal-type*))
	*universal-type*)))



(deftransform %ldb ((size posn int)
		    (fixnum fixnum integer)
		    (unsigned-byte #.vm:word-bits))
  "convert to inline logical ops"
  `(logand (ash int (- posn))
	   (ash ,(1- (ash 1 vm:word-bits))
		(- size ,vm:word-bits))))

(deftransform %mask-field ((size posn int)
			   (fixnum fixnum integer)
			   (unsigned-byte #.vm:word-bits))
  "convert to inline logical ops"
  `(logand int
	   (ash (ash ,(1- (ash 1 vm:word-bits))
		     (- size ,vm:word-bits))
		posn)))

;;; Note: for %dpb and %deposit-field, we can't use (or (signed-byte n)
;;; (unsigned-byte n)) as the result type, as that would allow result types
;;; that cover the range -2^(n-1) .. 1-2^n, instead of allowing result types
;;; of (unsigned-byte n) and result types of (signed-byte n).

(deftransform %dpb ((new size posn int)
		    *
		    (unsigned-byte #.vm:word-bits))
  "convert to inline logical ops"
  `(let ((mask (ldb (byte size 0) -1)))
     (logior (ash (logand new mask) posn)
	     (logand int (lognot (ash mask posn))))))

(deftransform %dpb ((new size posn int)
		    *
		    (signed-byte #.vm:word-bits))
  "convert to inline logical ops"
  `(let ((mask (ldb (byte size 0) -1)))
     (logior (ash (logand new mask) posn)
	     (logand int (lognot (ash mask posn))))))

(deftransform %deposit-field ((new size posn int)
			      *
			      (unsigned-byte #.vm:word-bits))
  "convert to inline logical ops"
  `(let ((mask (ash (ldb (byte size 0) -1) posn)))
     (logior (logand new mask)
	     (logand int (lognot mask)))))

(deftransform %deposit-field ((new size posn int)
			      *
			      (signed-byte #.vm:word-bits))
  "convert to inline logical ops"
  `(let ((mask (ash (ldb (byte size 0) -1) posn)))
     (logior (logand new mask)
	     (logand int (lognot mask)))))


;;; Miscellanous numeric transforms:


;;; COMMUTATIVE-ARG-SWAP  --  Internal
;;;
;;;    If a constant appears as the first arg, swap the args.
;;;
(deftransform commutative-arg-swap ((x y) * * :defun-only t :node node)
  (if (and (constant-continuation-p x)
	   (not (constant-continuation-p y)))
      `(,(continuation-function-name (basic-combination-fun node))
	y
	,(continuation-value x))
      (give-up)))

(dolist (x '(= char= + * logior logand logxor))
  (%deftransform x '(function * *) #'commutative-arg-swap
		 "place constant arg last."))

;;; Handle the case of a constant boole-code.
;;;
(deftransform boole ((op x y) * * :when :both)
  "convert to inline logical ops"
  (unless (constant-continuation-p op)
    (give-up "BOOLE code is not a constant."))
  (let ((control (continuation-value op)))
    (case control
      (#.boole-clr 0)
      (#.boole-set -1)
      (#.boole-1 'x)
      (#.boole-2 'y)
      (#.boole-c1 '(lognot x))
      (#.boole-c2 '(lognot y))
      (#.boole-and '(logand x y))
      (#.boole-ior '(logior x y))
      (#.boole-xor '(logxor x y))
      (#.boole-eqv '(logeqv x y))
      (#.boole-nand '(lognand x y))
      (#.boole-nor '(lognor x y))
      (#.boole-andc1 '(logandc1 x y))
      (#.boole-andc2 '(logandc2 x y))
      (#.boole-orc1 '(logorc1 x y))
      (#.boole-orc2 '(logorc2 x y))
      (t
       (abort-transform "~S illegal control arg to BOOLE." control)))))


;;;; Convert multiply/divide to shifts.

;;; If arg is a constant power of two, turn * into a shift.
;;;
(deftransform * ((x y) (integer integer) * :when :both)
  "convert x*2^k to shift"
  (unless (constant-continuation-p y) (give-up))
  (let* ((y (continuation-value y))
	 (y-abs (abs y))
	 (len (1- (integer-length y-abs))))
    (unless (= y-abs (ash 1 len)) (give-up))
    (if (minusp y)
	`(- (ash x ,len))
	`(ash x ,len))))

;;; If both arguments and the result are (unsigned-byte 32), try to come up
;;; with a ``better'' multiplication using multiplier recoding.  There are two
;;; different ways the multiplier can be recoded.  The more obvious is to shift
;;; X by the correct amount for each bit set in Y and to sum the results.  But
;;; if there is a string of bits that are all set, you can add X shifted by
;;; one more then the bit position of the first set bit and subtract X shifted
;;; by the bit position of the last set bit.  We can't use this second method
;;; when the high order bit is bit 31 because shifting by 32 doesn't work
;;; too well.
;;; 
(deftransform * ((x y)
		 ((unsigned-byte 32) (unsigned-byte 32))
		 (unsigned-byte 32))
  "recode as shift and add"
  (unless (constant-continuation-p y)
    (give-up))
  (let ((y (continuation-value y))
	(result nil)
	(first-one nil))
    (labels ((tub32 (x) `(truly-the (unsigned-byte 32) ,x))
	     (add (next-factor)
	       (setf result
		     (tub32
		      (if result
			  `(+ ,result ,(tub32 next-factor))
			  next-factor)))))
      (declare (inline add))
      (dotimes (bitpos 32)
	(if first-one
	    (when (not (logbitp bitpos y))
	      (add (if (= (1+ first-one) bitpos)
		       ;; There is only a single bit in the string.
		       `(ash x ,first-one)
		       ;; There are at least two.
		       `(- ,(tub32 `(ash x ,bitpos))
			   ,(tub32 `(ash x ,first-one)))))
	      (setf first-one nil))
	    (when (logbitp bitpos y)
	      (setf first-one bitpos))))
      (when first-one
	(cond ((= first-one 31))
	      ((= first-one 30)
	       (add '(ash x 30)))
	      (t
	       (add `(- ,(tub32 '(ash x 31)) ,(tub32 `(ash x ,first-one))))))
	(add '(ash x 31))))
    (or result 0)))

;;; If arg is a constant power of two, turn floor into a shift and mask.
;;; If ceiling, add in (1- (abs y)) and then do floor.
;;;
(flet ((frob (y ceil-p)
	 (unless (constant-continuation-p y) (give-up))
	 (let* ((y (continuation-value y))
		(y-abs (abs y))
		(len (1- (integer-length y-abs))))
	   (unless (= y-abs (ash 1 len)) (give-up))
	   (let ((shift (- len))
		 (mask (1- y-abs)))
	     `(let ,(when ceil-p `((x (+ x ,(1- y-abs)))))
		,(if (minusp y)
		     `(values (ash (- x) ,shift)
			      (- (logand (- x) ,mask)))
		     `(values (ash x ,shift)
			      (logand x ,mask))))))))
  (deftransform floor ((x y) (integer integer) *)
    "convert division by 2^k to shift"
    (frob y nil))
  (deftransform ceiling ((x y) (integer integer) *)
    "convert division by 2^k to shift"
    (frob y t)))


;;; Do the same for mod.
;;;
(deftransform mod ((x y) (integer integer) * :when :both)
  "convert remainder mod 2^k to LOGAND"
  (unless (constant-continuation-p y) (give-up))
  (let* ((y (continuation-value y))
	 (y-abs (abs y))
	 (len (1- (integer-length y-abs))))
    (unless (= y-abs (ash 1 len)) (give-up))
    (let ((mask (1- y-abs)))
      (if (minusp y)
	  `(- (logand (- x) ,mask))
	  `(logand x ,mask)))))


;;; If arg is a constant power of two, turn truncate into a shift and mask.
;;;
(deftransform truncate ((x y) (integer integer))
  "convert division by 2^k to shift"
  (unless (constant-continuation-p y) (give-up))
  (let* ((y (continuation-value y))
	 (y-abs (abs y))
	 (len (1- (integer-length y-abs))))
    (unless (= y-abs (ash 1 len)) (give-up))
    (let* ((shift (- len))
	   (mask (1- y-abs)))
      `(if (minusp x)
	   (values ,(if (minusp y)
			`(ash (- x) ,shift)
			`(- (ash (- x) ,shift)))
		   (- (logand (- x) ,mask)))
	   (values ,(if (minusp y)
			`(- (ash (- x) ,shift))
			`(ash x ,shift))
		   (logand x ,mask))))))

;;; And the same for rem.
;;;
(deftransform rem ((x y) (integer integer) * :when :both)
  "convert remainder mod 2^k to LOGAND"
  (unless (constant-continuation-p y) (give-up))
  (let* ((y (continuation-value y))
	 (y-abs (abs y))
	 (len (1- (integer-length y-abs))))
    (unless (= y-abs (ash 1 len)) (give-up))
    (let ((mask (1- y-abs)))
      `(if (minusp x)
	   (- (logand (- x) ,mask))
	   (logand x ,mask)))))


;;;; Arithmetic and logical identity operation elimination:
;;;
;;; Flush calls to random arith functions that convert to the identity
;;; function or a constant.


(dolist (stuff '((ash 0 x)
		 (logand -1 x)
		 (logand 0 0)
		 (logior 0 x)
		 (logior -1 -1)
		 (logxor -1 (lognot x))
		 (logxor 0 x)))
  (destructuring-bind (name identity result) stuff
    (deftransform name ((x y) `(* (constant-argument (member ,identity))) '*
			:eval-name t :when :both)
      "fold identity operations"
      result)))

;;; These are restricted to rationals, because (- 0 0.0) is 0.0, not -0.0, and
;;; (* 0 -4.0) is -0.0.
;;;
(deftransform - ((x y) ((constant-argument (member 0)) rational) *
		 :when :both)
  "convert (- 0 x) to negate"
  '(%negate y))
;;;
(deftransform * ((x y) (rational (constant-argument (member 0))) *
		 :when :both)
  "convert (* x 0) to 0."
  0)


;;; NOT-MORE-CONTAGIOUS  --  Interface
;;;
;;;    Return T if in an arithmetic op including continuations X and Y, the
;;; result type is not affected by the type of X.  That is, Y is at least as
;;; contagious as X.
;;;
(defun not-more-contagious (x y)
  (declare (type continuation x y))
  (let ((x (continuation-type x))
	(y (continuation-type y)))
    (values (type= (numeric-contagion x y)
		   (numeric-contagion y y)))))


;;; Fold (OP x 0).
;;;
;;;    If y is not constant, not zerop, or is contagious, then give up.
;;;
(dolist (stuff '((+ x)
		 (- x)
		 (expt 1)))
  (destructuring-bind (name result) stuff
    (deftransform name ((x y) '(t (constant-argument t)) '* :eval-name t
			:when :both)
      "fold zero arg"
      (let ((val (continuation-value y)))
	(unless (and (zerop val)
		     (not (and (floatp val) (minusp (float-sign val))))
		     (not-more-contagious y x))
	  (give-up)))
      result)))

;;; Fold (OP x +/-1)
;;;
(dolist (stuff '((* x (%negate x))
		 (/ x (%negate x))
		 (expt x (/ 1 x))))
  (destructuring-bind (name result minus-result) stuff
    (deftransform name ((x y) '(t (constant-argument real)) '* :eval-name t
			:when :both)
      "fold identity operations"
      (let ((val (continuation-value y)))
	(unless (and (= (abs val) 1)
		     (not-more-contagious y x))
	  (give-up))
	(if (minusp val) minus-result result)))))



(dolist (name '(ash /))
  (deftransform name ((x y) '((constant-argument (integer 0 0)) integer) '*
		      :eval-name t :when :both)
    "fold zero arg"
    0))

(dolist (name '(truncate round floor ceiling))
  (deftransform name ((x y) '((constant-argument (integer 0 0)) integer) '*
		      :eval-name t :when :both)
    "fold zero arg"
    '(values 0 0)))

    

;;;; Character operations:

(deftransform char-equal ((a b) (base-char base-char))
  "open code"
  '(let* ((ac (char-code a))
	  (bc (char-code b))
	  (sum (logxor ac bc)))
     (or (zerop sum)
	 (when (eql sum #x20)
	   (let ((sum (+ ac bc)))
	     (and (> sum 161) (< sum 213)))))))

(deftransform char-upcase ((x) (base-char))
  "open code"
  '(let ((n-code (char-code x)))
     (if (and (> n-code #o140)	; Octal 141 is #\a.
	      (< n-code #o173))	; Octal 172 is #\z.
	 (code-char (logxor #x20 n-code))
	 x)))

(deftransform char-downcase ((x) (base-char))
  "open code"
  '(let ((n-code (char-code x)))
     (if (and (> n-code 64)	; 65 is #\A.
	      (< n-code 91))	; 90 is #\Z.
	 (code-char (logxor #x20 n-code))
	 x)))


;;;; Equality predicate transforms:


;;; SAME-LEAF-REF-P  --  Internal
;;;
;;;    Return true if X and Y are continuations whose only use is a reference
;;; to the same leaf, and the value of the leaf cannot change.
;;;
(defun same-leaf-ref-p (x y)
  (declare (type continuation x y))
  (let ((x-use (continuation-use x))
	(y-use (continuation-use y)))
    (and (ref-p x-use)
	 (ref-p y-use)
	 (eq (ref-leaf x-use) (ref-leaf y-use))
	 (constant-reference-p x-use))))


;;; SIMPLE-EQUALITY-TRANSFORM  --  Internal
;;;
;;;    If X and Y are the same leaf, then the result is true.  Otherwise, if
;;; there is no intersection between the types of the arguments, then the
;;; result is definitely false.
;;;
(deftransform simple-equality-transform ((x y) * * :defun-only t
					 :when :both)
  (cond ((same-leaf-ref-p x y)
	 't)
	((not (types-intersect (continuation-type x) (continuation-type y)))
	 'nil)
	(t
	 (give-up))))

(dolist (x '(eq char= equal))
  (%deftransform x '(function * *) #'simple-equality-transform))


;;; EQL IR1 Transform  --  Internal
;;;
;;;    Similar to SIMPLE-EQUALITY-PREDICATE, except that we also try to convert
;;; to a type-specific predicate or EQ:
;;; -- If both args are characters, convert to CHAR=.  This is better than just
;;;    converting to EQ, since CHAR= may have special compilation strategies
;;;    for non-standard representations, etc.
;;; -- If either arg is definitely not a number, then we can compare with EQ.
;;; -- Otherwise, we try to put the arg we know more about second.  If X is
;;;    constant then we put it second.  If X is a subtype of Y, we put it
;;;    second.  These rules make it easier for the back end to match these
;;;    interesting cases.
;;; -- If Y is a fixnum, then we quietly pass because the back end can handle
;;;    that case, otherwise give an efficency note.
;;;
(deftransform eql ((x y) * * :when :both)
  "convert to simpler equality predicate"
  (let ((x-type (continuation-type x))
	(y-type (continuation-type y))
	(char-type (specifier-type 'character))
	(number-type (specifier-type 'number)))
    (cond ((same-leaf-ref-p x y)
	   't)
	  ((not (types-intersect x-type y-type))
	   'nil)
	  ((and (csubtypep x-type char-type)
		(csubtypep y-type char-type))
	   '(char= x y))
	  ((or (not (types-intersect x-type number-type))
	       (not (types-intersect y-type number-type)))
	   '(eq x y))
	  ((and (not (constant-continuation-p y))
		(or (constant-continuation-p x)
		    (and (csubtypep x-type y-type)
			 (not (csubtypep y-type x-type)))))
	   '(eql y x))
	  (t
	   (give-up)))))


;;; = IR1 Transform  --  Internal
;;;
;;;    Convert to EQL if both args are rational and complexp is specified
;;; and the same for both.
;;; 
(deftransform = ((x y) * * :when :both)
  "open code"
  (let ((x-type (continuation-type x))
	(y-type (continuation-type y)))
    (if (and (numeric-type-p x-type) (numeric-type-p y-type))
	(let ((x-class (numeric-type-class x-type))
	      (y-class (numeric-type-class y-type)))
	  (cond ((and (eq x-class 'float) (eq y-class 'float))
		 ;; They are both floats.  Leave as = so that -0.0 is
		 ;; handled correctly.
		 (give-up))
		((and (member x-class '(rational integer))
		      (member y-class '(rational integer))
		      (let ((x-complexp (numeric-type-complexp x-type)))
			(and x-complexp
			     (eq x-complexp (numeric-type-complexp y-type)))))
		 ;; They are both rationals and complexp is the same.  Convert
		 ;; to EQL.
		 '(eql x y))
		(t
		 (give-up "Operands might not be the same type."))))
	(give-up "Operands might not be the same type."))))


;;; Numeric-Type-Or-Lose  --  Interface
;;;
;;;    If Cont's type is a numeric type, then return the type, otherwise
;;; GIVE-UP.
;;;
(defun numeric-type-or-lose (cont)
  (declare (type continuation cont))
  (let ((res (continuation-type cont)))
    (unless (numeric-type-p res) (give-up))
    res))


;;; IR1-TRANSFORM-<  --  Internal
;;;
;;;    See if we can statically determine (< X Y) using type information.  If
;;; X's high bound is < Y's low, then X < Y.  Similarly, if X's low is >= to
;;; Y's high, the X >= Y (so return NIL).  If not, at least make sure any
;;; constant arg is second.
;;;
(defun ir1-transform-< (x y first second inverse)
  (if (same-leaf-ref-p x y)
      'nil
      (let* ((x-type (numeric-type-or-lose x))
	     (x-lo (numeric-type-low x-type))
	     (x-hi (numeric-type-high x-type))
	     (y-type (numeric-type-or-lose y))
	     (y-lo (numeric-type-low y-type))
	     (y-hi (numeric-type-high y-type)))
	(cond ((and x-hi y-lo (< x-hi y-lo))
	       't)
	      ((and y-hi x-lo (>= x-lo y-hi))
	       'nil)
	      ((and (constant-continuation-p first)
		    (not (constant-continuation-p second)))
	       `(,inverse y x))
	      (t
	       (give-up))))))
	      

(deftransform < ((x y) (integer integer) * :when :both)
  (ir1-transform-< x y x y '>))

(deftransform > ((x y) (integer integer) * :when :both)
  (ir1-transform-< y x x y '<))


;;;; Converting N-arg comparisons:
;;;
;;;    We convert calls to N-arg comparison functions such as < into two-arg
;;; calls.  This transformation is enabled for all such comparisons in this
;;; file.  If any of these predicates are not open-coded, then the
;;; transformation should be removed at some point to avoid pessimization.

;;; Multi-Compare  --  Internal
;;;
;;;    This function is used for source transformation of N-arg comparison
;;; functions other than inequality.  We deal both with converting to two-arg
;;; calls and inverting the sense of the test, if necessary.  If the call has
;;; two args, then we pass or return a negated test as appropriate.  If it is a
;;; degenerate one-arg call, then we transform to code that returns true.
;;; Otherwise, we bind all the arguments and expand into a bunch of IFs.
;;;
(proclaim '(function multi-compare (symbol list boolean)))
(defun multi-compare (predicate args not-p)
  (let ((nargs (length args)))
    (cond ((< nargs 1) (values nil t))
	  ((= nargs 1) `(progn ,@args t))
	  ((= nargs 2)
	   (if not-p
	       `(if (,predicate ,(first args) ,(second args)) nil t)
	       (values nil t)))
	  (t
	   (do* ((i (1- nargs) (1- i))
		 (last nil current)
		 (current (gensym) (gensym))
		 (vars (list current) (cons current vars))
		 (result 't (if not-p
				`(if (,predicate ,current ,last)
				     nil ,result)
				`(if (,predicate ,current ,last)
				     ,result nil))))
	       ((zerop i)
		`((lambda ,vars ,result) . ,args)))))))


(def-source-transform = (&rest args) (multi-compare '= args nil))
(def-source-transform < (&rest args) (multi-compare '< args nil))
(def-source-transform > (&rest args) (multi-compare '> args nil))
(def-source-transform <= (&rest args) (multi-compare '> args t))
(def-source-transform >= (&rest args) (multi-compare '< args t))

(def-source-transform char= (&rest args) (multi-compare 'char= args nil))
(def-source-transform char< (&rest args) (multi-compare 'char< args nil))
(def-source-transform char> (&rest args) (multi-compare 'char> args nil))
(def-source-transform char<= (&rest args) (multi-compare 'char> args t))
(def-source-transform char>= (&rest args) (multi-compare 'char< args t))

(def-source-transform char-equal (&rest args) (multi-compare 'char-equal args nil))
(def-source-transform char-lessp (&rest args) (multi-compare 'char-lessp args nil))
(def-source-transform char-greaterp (&rest args) (multi-compare 'char-greaterp args nil))
(def-source-transform char-not-greaterp (&rest args) (multi-compare 'char-greaterp args t))
(def-source-transform char-not-lessp (&rest args) (multi-compare 'char-lessp args t))


;;; Multi-Not-Equal  --  Internal
;;;
;;;    This function does source transformation of N-arg inequality functions
;;; such as /=.  This is similar to Multi-Compare in the <3 arg cases.  If
;;; there are more than two args, then we expand into the appropriate n^2
;;; comparisons only when speed is important.
;;;
(proclaim '(function multi-not-equal (symbol list)))
(defun multi-not-equal (predicate args)
  (let ((nargs (length args)))
    (cond ((< nargs 1) (values nil t))
	  ((= nargs 1) `(progn ,@args t))
	  ((= nargs 2)
	   `(if (,predicate ,(first args) ,(second args)) nil t))
	  ((not (policy nil (>= speed space) (>= speed cspeed)))
	   (values nil t))
	  (t
	   (collect ((vars))
	     (dotimes (i nargs) (vars (gensym)))
	     (do ((var (vars) next)
		  (next (cdr (vars)) (cdr next))
		  (result 't))
		 ((null next)
		  `((lambda ,(vars) ,result) . ,args))
	       (let ((v1 (first var)))
		 (dolist (v2 next)
		   (setq result `(if (,predicate ,v1 ,v2) nil ,result))))))))))

(def-source-transform /= (&rest args) (multi-not-equal '= args))
(def-source-transform char/= (&rest args) (multi-not-equal 'char= args))
(def-source-transform char-not-equal (&rest args) (multi-not-equal 'char-equal args))


;;; Expand Max and Min into the obvious comparisons.
(def-source-transform max (arg &rest more-args)
  (if (null more-args)
      `(values ,arg)
      (once-only ((arg1 arg)
		  (arg2 `(max ,@more-args)))
	`(if (> ,arg1 ,arg2)
	     ,arg1 ,arg2))))
;;;
(def-source-transform min (arg &rest more-args)
  (if (null more-args)
      `(values ,arg)
      (once-only ((arg1 arg)
		  (arg2 `(min ,@more-args)))
	`(if (< ,arg1 ,arg2)
	     ,arg1 ,arg2))))


;;;; Converting N-arg arithmetic functions:
;;;
;;;    N-arg arithmetic and logic functions are associated into two-arg
;;; versions, and degenerate cases are flushed.

;;; Associate-Arguments  --  Internal
;;;
;;;    Left-associate First-Arg and More-Args using Function.
;;;
(proclaim '(function associate-arguments (symbol t list) list))
(defun associate-arguments (function first-arg more-args)
  (let ((next (rest more-args))
	(arg (first more-args)))
    (if (null next)
	`(,function ,first-arg ,arg)
	(associate-arguments function `(,function ,first-arg ,arg) next))))

;;; Source-Transform-Transitive  --  Internal
;;;
;;;    Do source transformations for transitive functions such as +.  One-arg
;;; cases are replaced with the arg and zero arg cases with the identity.  If
;;; Leaf-Fun is true, then replace two-arg calls with a call to that function. 
;;;
(defun source-transform-transitive (fun args identity &optional leaf-fun)
  (declare (symbol fun leaf-fun) (list args))
  (case (length args)
    (0 identity)
    (1 `(values ,(first args)))
    (2 (if leaf-fun
	   `(,leaf-fun ,(first args) ,(second args))
	   (values nil t)))
    (t
     (associate-arguments fun (first args) (rest args)))))

(def-source-transform + (&rest args) (source-transform-transitive '+ args 0))
(def-source-transform * (&rest args) (source-transform-transitive '* args 1))
(def-source-transform logior (&rest args) (source-transform-transitive 'logior args 0))
(def-source-transform logxor (&rest args) (source-transform-transitive 'logxor args 0))
(def-source-transform logand (&rest args) (source-transform-transitive 'logand args -1))

(def-source-transform logeqv (&rest args)
  (if (evenp (length args))
      `(lognot (logxor ,@args))
      `(logxor ,@args)))

;;; Note: we can't use source-transform-transitive for GCD and LCM because when
;;; they are given one argument, they return it's absolute value.

(def-source-transform gcd (&rest args)
  (case (length args)
    (0 0)
    (1 `(abs (the integer ,(first args))))
    (2 (values nil t))
    (t (associate-arguments 'gcd (first args) (rest args)))))

(def-source-transform lcm (&rest args)
  (case (length args)
    (0 1)
    (1 `(abs (the integer ,(first args))))
    (2 (values nil t))
    (t (associate-arguments 'lcm (first args) (rest args)))))


;;; Source-Transform-Intransitive  --  Internal
;;;
;;;    Do source transformations for intransitive n-arg functions such as /.
;;; With one arg, we form the inverse.  With two args we pass.  Otherwise we
;;; associate into two-arg calls.
;;;
(proclaim '(function source-transform-intransitive (symbol list t) list))
(defun source-transform-intransitive (function args inverse)
  (case (length args)
    ((0 2) (values nil t))
    (1 `(,@inverse ,(first args)))
    (t
     (associate-arguments function (first args) (rest args)))))

(def-source-transform - (&rest args)
  (source-transform-intransitive '- args '(%negate)))
(def-source-transform / (&rest args)
  (source-transform-intransitive '/ args '(/ 1)))


;;;; Apply:
;;;
;;;    We convert Apply into Multiple-Value-Call so that the compiler only
;;; needs to understand one kind of variable-argument call.  It is more
;;; efficient to convert Apply to MV-Call than MV-Call to Apply.

(def-source-transform apply (fun arg &rest more-args)
  (let ((args (cons arg more-args)))
    `(multiple-value-call ,fun
       ,@(mapcar #'(lambda (x)
		     `(values ,x))
		 (butlast args))
       (values-list ,(car (last args))))))


;;;; FORMAT transform:
;;;
;;; If the control string is a compile-time constant, then replace it with
;;; a use of the FORMATTER macro so that the control string is ``compiled.''
;;; Furthermore, if the destination is either a stream or T and the control
;;; string is a function (i.e. formatter), then convert the call to format to
;;; just a funcall of that function.
;;; 
(deftransform format ((dest control &rest args) (t simple-string &rest t) *
		      :policy (> speed space))
  (unless (constant-continuation-p control)
    (give-up "Control string is not a constant."))
  (let ((arg-names (mapcar #'(lambda (x) (declare (ignore x)) (gensym)) args)))
    `(lambda (dest control ,@arg-names)
       (declare (ignore control))
       (format dest (formatter ,(continuation-value control)) ,@arg-names))))
;;;
(deftransform format ((stream control &rest args) (stream function &rest t) *
		      :policy (> speed space))
  (let ((arg-names (mapcar #'(lambda (x) (declare (ignore x)) (gensym)) args)))
    `(lambda (stream control ,@arg-names)
       (funcall control stream ,@arg-names)
       nil)))
;;;
(deftransform format ((tee control &rest args) ((member t) function &rest t) *
		      :policy (> speed space))
  (let ((arg-names (mapcar #'(lambda (x) (declare (ignore x)) (gensym)) args)))
    `(lambda (tee control ,@arg-names)
       (declare (ignore tee))
       (funcall control *standard-output* ,@arg-names)
       nil)))
