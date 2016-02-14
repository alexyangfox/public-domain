README.TXT file for MUSCLE v4.0
-------------------------------

This is a preliminary, experimental version of MUSCLE v4.0.

It is recommended for experienced, advanced users only.
Previous versions of MUSCLE may be more stable and more
portable. They certainly have much better documentation,
though I hope to improve on this in the near future.

This README.TXT file is the only documentation currently
available for v4. This program has many capabilities, only
of which are described here.

In previous versions of MUSCLE I made an effort to maintain
backwards compatibility. However, I consider this to be a
prototype -- everything is subject to change; please consider
this before building any infrastructure around this code,
e.g. output parsers, because they will almost certainly need
to be changed for future versions.

Robert C. Edgar.
Contact: bob@drive5.com.


Compiling and linking
---------------------

On Unix and Linux systems, the mk script is used to build
MUSCLE. The binary file is named mus4. The bash shell is
required, if bash is not available you will have to
edit the script to conform to your shell. It is short and
simple so this should not be too difficult.

A Microsoft Visual Studio project file is provided for use
on Windows systems: mus4.vcproj.

Not very helpful help
---------------------

You can try:

	mus4 --help

This will give a complete list of command-line options, but
is unlikely to be enough for you to figure out how to use many
of them.


Multiple protein or nucleotide alignment
----------------------------------------

	mus4 --input seqs.fasta --log muscle.log

The log file will contain several analyses of the sequences,
including a multiple alignment in a proprietary but hopefully
easily understandable format.

Better names for the --log option would be --out or --output,
and I will probably change it in future versions.

To suppress the additional analysis such as repeat detection:

	mus4 --input seqs.fasta --msa_only --log muscle.log

This will save execution time and may reduce distracting information
you don't need in the output file.

To get output in FASTA format:

	mus4 --input seqs.fasta --output alignment.fasta [--msa_only]
	

Frame-shift detection
---------------------

	mus4 --input seqs.fasta --frameshift_only --cons 0 --log muscle.log


Multiple local alignment with re-arrangements
---------------------------------------------

	mus4 --input seqs.fasta --maf output.maf

The MAF format is documented here:

	http://genome.ucsc.edu/FAQ/FAQformat#format5


Misc options
------------

    --quiet                Suppress progress messages written to stderr.
    --version              Report version number.
    --cons <n>             Number of consistency reps (default 1).
    --refine <n>           Number of refinement passes (default 10).
    --minlocalprob <p>     Posterior threshold for local alignment (default 0.85).
    --minsparseprob <p>    Posterior threshold for sparse matrices (default 0.3).
    --minlocallen <n>      Minimum length of a local alignment (default 8).
    --model <name>         Graphical model, for example:
                              localaff
                              localaffnuc
                              global
                              globalaff
                              globaltermaff
                              globalnuc
                              globalaffnuc
                              <model1>+<model2> (dual model)
                              global+localaff (default for amino acids)
                              globalnuc+localaffnuc (default for nucleotides)
 